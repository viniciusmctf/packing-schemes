/***
  * Author: Vinicius Freitas
  * contact: vinicius.mct.freitas@gmail.com OR vinicius.mctf@grad.ufsc.br
  * Produced @ ECL - UFSC
  * Newly developed strategy based on Harshita Menon's implementation of GrapevineLB
  * Updated at Apr 2019
  */

#include "PackDropLB.h"
#include "elements.h"
#include <stdlib.h>
#include <cstring>
#include <algorithm>

CreateLBFunc_Def(PackDropLB, "Pack-based distributed load balancer");

CkReductionMsg* sumTwoIndependentDoubles(int n_msg, CkReductionMsg** msgs) {
  //Sum starts off at zero
  double ret[2]= {0,0};
  for (int i=0;i<n_msg;i++) {
    //Sanity check:
    CkAssert(msgs[i]->getSize()==2*sizeof(double));
    //Extract this message's data
    double *m=(double *)msgs[i]->getData();
    ret[0]+=m[0];
    ret[1]+=m[1];
  }
  return CkReductionMsg::buildNew(2*sizeof(double),ret);
}

/*global*/ CkReduction::reducerType sumTwoIndependentDoublesType;
/*initnode*/ void registerSumTwoIndependentDoubles(void)
{
  sumTwoIndependentDoublesType = CkReduction::addReducer(sumTwoIndependentDoubles);
}

PackDropLB::PackDropLB(CkMigrateMessage *m) : CBase_PackDropLB(m) {
}

PackDropLB::PackDropLB(const CkLBOptions &opt) : CBase_PackDropLB(opt) {
  lbname = "PackDropLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] PackDropLB created\n",CkMyPe());
  InitLB(opt);
}

void PackDropLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_PackDropLB(thisgroup);
}

void PackDropLB::Strategy(const DistBaseLB::LDStats* const stats) {
    if (CkMyPe() == 0) {
        CkPrintf("In PackDrop Strategy\n");
    }

    lb_started = false;
    my_stats = stats;
    threshold = 0.05;
    lb_end = false;
    tries = 0;
    info_send_count = 0;

    packs.clear();
    pe_no.clear();
    loads.clear();
    receivers.clear();
    pack_count = 0;
    total_migrates = 0;
    acks_needed = 0;

    packs = std::map<int, std::vector<int> >();

    kMaxGossipMsgCount = 2 * CmiLog2(CkNumPes());
    kPartialInfoCount = -1;

    distribution.clear();
    srand((unsigned)CmiWallTimer()*CkMyPe()/CkNumPes());

    // Parse through LDStats to initialize work map
    int nobjs = my_stats->n_objs;
    std::vector<std::pair<int,double>> map;
    map.reserve(nobjs);
    migrateInfo = {};
    for (int i = 0; i < nobjs; ++i) {
      if (my_stats->objData[i].migratable && my_stats->objData[i].wallTime > 0.0001) {
        map.emplace_back(i, my_stats->objData[i].wallTime);
      }
    }
    local_work_info = WorkMap(map);
    migrateInfo.reserve(nobjs);

    my_load = local_work_info.calculate_total_load();
    double l = my_load;
    double chares = (double) my_stats->n_objs;
    double data[2] = {l, chares};

    CkCallback cb(CkReductionTarget(PackDropLB, LoadSetup), thisProxy);
    contribute(2*sizeof(double), (void*) data, sumTwoIndependentDoublesType, cb);
}

void PackDropLB::LoadSetup(CkReductionMsg* loadAndChares) {
  double* res = (double*) loadAndChares->getData();
  avg_load = res[0]/CkNumPes();
  int chares = (int) res[1];
  if (_lb_args.debug() > 2)
    CkPrintf("[%d] Sanity check, avg load: %lf, chares in lf: %lf, chares int: %d \n",
      CkMyPe(), avg_load, res[1], chares);
  ChareSetup(chares);
}


void PackDropLB::ChareSetup(int count) {
    if (_lb_args.debug() > 2) CkPrintf("[%d] Start ChareSetup\n", CkMyPe());
    chare_count = count;
    if (_lb_args.lbpacksize() > 0) {
      pack_load = _lb_args.lbpacksize()*avg_load;
    } else {
      double avg_task_size = (CkNumPes()*avg_load)/chare_count;
      pack_load = avg_task_size*(2 - CkNumPes()/chare_count);
    }

    double ceil = avg_load*(1+threshold);
    double pack_floor = pack_load*(1-threshold);
    double pack_load_now = 0;
    if (my_load > ceil) {
      double rem_load = ceil - avg_load;
      auto removed_objs = local_work_info.remove_batch_of_load(rem_load);
      int pack_id = 0;
      packs.emplace(std::make_pair(pack_id, std::vector<int>()));
      // From UpdateWorkMap to our local packing scheme
      for (auto obj : removed_objs) {
        if (_lb_args.debug() > 3) CkPrintf("[%d] Creating Pack(%d), adding task of id <%d>, and load <%.5lf> \n", CkMyPe(), pack_id, obj.sys_index, obj.load);
        pack_load_now += obj.load;
        packs[pack_id].push_back(obj.sys_index);
        if (pack_load_now > pack_floor) {
          packs.emplace(std::make_pair(++pack_id, std::vector<int>()));
          my_load -= pack_load_now;
          pack_load_now = 0.0;
        }
      }
      pack_count = packs.size();
    } else {
        double r_loads[1];
        int r_pe_no[1];
        r_loads[0] = my_load;
        r_pe_no[0] = CkMyPe();
        req_hop = 0;
        GossipLoadInfo(req_hop, CkMyPe(), 1, r_pe_no, r_loads);
    }
    if (CkMyPe() == 0) {
        CkCallback cb(CkIndex_PackDropLB::First_Barrier(), thisProxy);
        CkStartQD(cb);
    }
    if (_lb_args.debug() > 2) CkPrintf("[%d] End ChareSetup\n", CkMyPe());
    //CkPrintf("[%d] Ending ChareSetup step\n", CkMyPe());
}

void PackDropLB::First_Barrier() {
    if (_lb_args.debug() > 2) CkPrintf("[%d] Start Load Balance\n", CkMyPe());
    LoadBalance();
}

void PackDropLB::LoadBalance() {
    lb_started = true;
    if (packs.size() == 0) {
        if (_lb_args.debug() > 2)
          // CkPrintf("[%d] Am underloaded, it's ok ...\n", CkMyPe());
        msg = new(total_migrates,CkNumPes(),CkNumPes(),0) LBMigrateMsg;
        msg->n_moves = total_migrates;
        CkCallback cb(CkReductionTarget(PackDropLB, Final_Barrier), thisProxy);
        contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
        return;
    } else {
        if (_lb_args.debug() > 2) CkPrintf("[%d] Am overloaded, time to migrate.\n", CkMyPe());
    }
    underloaded_pe_count = pe_no.size();
    // CalculateCumulateDistribution();
    CalculateReceivers();
    PackSend(0,0);
    //CkPrintf("[%d] In LoadBalance Step\n", CkMyPe());
}

void PackDropLB::CalculateReceivers() {
    double pack_ceil = pack_load*(1+threshold);
    double ceil = avg_load*(1+threshold);
    for (size_t i = 0; i < pe_no.size(); ++i) {
        if (loads[i] + pack_ceil < ceil) {
            receivers.push_back(pe_no[i]);
        }
    }
}

int PackDropLB::FindReceiver() {
  int rec = 0;
  //if (_lb_args.debug() > 2) CkPrintf("[%d] Looking for receivers...\n", CkMyPe());
  if (receivers.size() < CkNumPes()/4) {
      rec = rand()%CkNumPes();
      while (rec == CkMyPe()) {
          rec = rand()%CkNumPes();
      }
  } else {
      rec = receivers[rand()%receivers.size()];
      while (rec == CkMyPe()) {
          rec = receivers[rand()%receivers.size()];
      }
  }
  return rec;
}

void PackDropLB::PackSend(int pack_id, int one_time) {
    if (_lb_args.debug() > 2) CkPrintf("[%d] Start PackSend\n", CkMyPe());
    tries++;
    if (tries >= 4) {
        //if (_lb_args.debug()) CkPrintf("[%d] No receivers found\n", CkMyPe());
        EndStep();
        return;
    }
    int idp = pack_id;
    //if (_lb_args.debug() > 2) CkPrintf("[%d] Trying to send %d packs (%d) to random receivers, total of %d tasks..\n", CkMyPe(), packs.size(), idp, packs[idp].size());
    while (idp < packs.size()) {
        if (packs[idp].size() == 0) {
            ++idp;
            continue;
        }
        int rand_rec = FindReceiver();
        acks_needed++;
        thisProxy[rand_rec].PackAck(idp, CkMyPe(), packs[idp].size(), pack_load, false);
        if (one_time) {
            break;
        }
        ++idp;
    }
    // if (_lb_args.debug() > 2) CkPrintf("[%d] End PackSend\n", CkMyPe());
}

void PackDropLB::PackAck(int id, int from, int psize, double pload, bool force) {
    bool ack = ((my_load + pack_load < avg_load*(1+threshold)) || force);
    if (ack) {
        migrates_expected += psize;
        my_load += pload;
    }
    thisProxy[from].RecvAck(id, CkMyPe(), pload, ack);
}

void PackDropLB::RecvAck(int id, int to, double pload, bool success) {
    if (success) {
        const std::vector<int> this_pack = packs.at(id);
        for (size_t i = 0; i < this_pack.size(); ++i) {
            int task = this_pack.at(i);
            MigrateInfo* inf = new MigrateInfo();
            inf->obj = my_stats->objData[task].handle;
            inf->from_pe = CkMyPe();
            inf->to_pe = to;
            migrateInfo.push_back(inf);
        }
        packs[id] = std::vector<int>();
        total_migrates++;
        acks_needed--;
        pack_count--;
        if (acks_needed == 0) {
            msg = new(total_migrates, CkNumPes(), CkNumPes(), 0) LBMigrateMsg;
            msg->n_moves = total_migrates;
            for (size_t i = 0; i < total_migrates; ++i) {
                MigrateInfo* inf = (MigrateInfo*) migrateInfo[i];
                msg->moves[i] = *inf;
                delete inf;
            }
            migrateInfo.clear();
            lb_end = true;
            CkCallback cb(CkReductionTarget(PackDropLB, Final_Barrier), thisProxy);
            contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
        }
    } else {
        acks_needed--;
        if (tries >= 2) {
            ForcedPackSend(id, pload, true);
        } else {
            ForcedPackSend(id, pload, false);
        }
    }
}

void PackDropLB::ForcedPackSend(int id, double pload, bool force) {
    int rand_rec = FindReceiver();
    tries++;
    acks_needed++;
    thisProxy[rand_rec].PackAck(id, CkMyPe(), (int) packs.at(id).size(), pload, force);
}

void PackDropLB::EndStep() {
    if (total_migrates < pack_count && tries < 8) {
        CkPrintf("[%d] Gotta migrate more: %d\n", CkMyPe(), tries);
        PackSend();
    } else {
        msg = new(total_migrates, CkNumPes(), CkNumPes(), 0) LBMigrateMsg;
        msg->n_moves = total_migrates;
        for (size_t i = 0; i < total_migrates; ++i) {
            MigrateInfo* inf = (MigrateInfo*) migrateInfo[i];
            msg->moves[i] = *inf;
            delete inf;
        }
        migrateInfo.clear();
        lb_end = true;
        CkCallback cb(CkReductionTarget(PackDropLB, Final_Barrier), thisProxy);
        contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
    }
}

void PackDropLB::Final_Barrier(double max_load) {
    if (CkMyPe() == 0 && _lb_args.debug()) {
      CkPrintf("Final makespan: %lf\n", max_load);
    }
    ProcessMigrationDecision(msg);
}

// TODO
void PackDropLB::ShowMigrationDetails() {
  if (total_migrates > 0)
    CkPrintf("[%d] migrating %d elements\n", CkMyPe(), total_migrates);
  if (migrates_expected > 0)
    CkPrintf("[%d] receiving %d elements\n", CkMyPe(), migrates_expected);

  CkCallback cb (CkReductionTarget(PackDropLB, DetailsRedux), thisProxy);
  contribute(sizeof(int), &total_migrates, CkReduction::sum_int, cb);
}

void PackDropLB::DetailsRedux(int migs) {
  if (CkMyPe() <= 0) CkPrintf("[%d] Total number of migrations is %d\n", CkMyPe(), migs);
}

/*
* Gossip load information between peers. Receive the gossip message.
*/
void PackDropLB::GossipLoadInfo(int req_h, int from_pe, int n,
    int remote_pe_no[], double remote_loads[]) {
  std::vector<int> p_no;
  std::vector<double> l;

  int i = 0;
  int j = 0;
  int m = pe_no.size();

  while (i < m && j < n) {
    if (pe_no[i] < remote_pe_no[j]) {
      p_no.push_back(pe_no[i]);
      l.push_back(loads[i]);
      i++;
    } else {
      p_no.push_back(remote_pe_no[j]);
      l.push_back(remote_loads[j]);
      if (pe_no[i] == remote_pe_no[j]) {
        i++;
      }
      j++;
    }
  }

  if (i == m && j != n) {
    while (j < n) {
      p_no.push_back(remote_pe_no[j]);
      l.push_back(remote_loads[j]);
      j++;
    }
  } else if (j == n && i != m) {
    while (i < m) {
      p_no.push_back(pe_no[i]);
      l.push_back(loads[i]);
      i++;
    }
  }

  pe_no.swap(p_no);
  loads.swap(l);
  req_hop = req_h + 1;

  SendLoadInfo();
}

/*
* Construct the gossip message and send to peers
*/
void PackDropLB::SendLoadInfo() {
  if (gossip_msg_count > kMaxGossipMsgCount) {
    return;
  }

  int rand_nbor1;
  int rand_nbor2 = -1;
  do {
    rand_nbor1 = rand() % CkNumPes();
  } while (rand_nbor1 == CkMyPe());

  do {
    rand_nbor2 = rand() % CkNumPes();
  } while ((rand_nbor2 == CkMyPe()) || (rand_nbor2 == rand_nbor1));


  int info_count = (kPartialInfoCount >= 0) ? kPartialInfoCount : pe_no.size();
  int* p = new int[info_count];
  double* l = new double[info_count];
  for (int i = 0; i < info_count; i++) {
    p[i] = pe_no[i];
    l[i] = loads[i];
  }

  thisProxy[rand_nbor1].GossipLoadInfo(req_hop, CkMyPe(), info_count, p, l);
  thisProxy[rand_nbor2].GossipLoadInfo(req_hop, CkMyPe(), info_count, p, l);

  gossip_msg_count++;

  delete[] p;
  delete[] l;
}

/*
* The PEs have probabilities inversely proportional to their load. Construct a
* CDF based on this. from DistributedLB.C
*/
void PackDropLB::CalculateCumulateDistribution() {
  // The min loaded PEs have probabilities inversely proportional to their load.
  double cumulative = 0.0;
  double ub_load = 1.05*avg_load;
  for (int i = 0; i < underloaded_pe_count; i++) {
    cumulative += (ub_load - loads[i])/ub_load;
    distribution.push_back(cumulative);
  }

  for (int i = 0; i < underloaded_pe_count; i++) {
    distribution[i] = distribution[i]/cumulative;
  }
}

#include "PackDropLB.def.h"
