#include "DiscreteSelfishLB.h"
#include <random>

void _discrete_this_iteration_load(int it, const char* string, double makespan) {
  if (CkMyPe() == 0 && _lb_args.debug() > 1) {
    CkPrintf("[%d] %s %lf\n", it, string, makespan);
  }
}

double calc_pack_load(double avg_m, double max_j) {
  if (2*max_j > 0.025*avg_m) {
    return 2*max_j;
  } else {
    return 0.025*avg_m;
  }
}

CkReductionMsg* sum_max_double(int n_msg, CkReductionMsg** msgs) {
  //Sum starts off at zero
  double ret[2]= {0,0};
  for (int i = 0; i < n_msg; i++) {
    //Sanity check:
    CkAssert(msgs[i]->getSize()==2*sizeof(double));
    //Extract this message's data
    double *m=(double *)msgs[i]->getData();
    ret[0]+=m[0];
    ret[1]=((ret[1] > m[1]) ? ret[1] : m[1]);
  }
  return CkReductionMsg::buildNew(2*sizeof(double),ret);
}

/*global*/ CkReduction::reducerType sum_max_doubleType;
/*initnode*/ void registerSum_max_double(void)
{
  sum_max_doubleType = CkReduction::addReducer(sum_max_double);
}

CreateLBFunc_Def(DiscreteSelfishLB, "Greedy Selfish Task Reallocation");

DiscreteSelfishLB::DiscreteSelfishLB(CkMigrateMessage *m) : CBase_DiscreteSelfishLB(m) {
}

DiscreteSelfishLB::DiscreteSelfishLB(const CkLBOptions &opt) : CBase_DiscreteSelfishLB(opt) {
  lbname = "DiscreteSelfishLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] DiscreteSelfishLB created\n",CkMyPe());
  InitLB(opt);
}

void DiscreteSelfishLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_DiscreteSelfishLB(thisgroup);
}

void DiscreteSelfishLB::Strategy(const DistBaseLB::LDStats* const stats) {
  if (CkMyPe() == 0) {
    CkPrintf("Starting DiscreteSelfishLB...\n");
  }
  if (_lb_args.lbpacksize() > 0 && _lb_args.lbpacksize() < CkNumPes()) {
    kIterations = _lb_args.lbpacksize();
  } else {
    kIterations = CkNumPes();
  }
  cur_iteration = 1;
  mig_counts.clear();
  mig_counts = std::vector<int> (CkNumPes(), 0); // Fill mig_counts with 0s
  waiting_messages = 0; migrates_expected = 0;
  my_load = 0.0; leaving = 0;
  all_tasks.clear(); migrateInfo.clear();
  my_stats = stats;
  int nobjs = my_stats->n_objs;
  all_tasks.reserve(2*nobjs);
  ordered_tasks.reserve(nobjs);
  task_to_destination_map.clear();
  int my_id = CkMyPe();
  double max_task = 0.0;

  for (int i = 0; i < nobjs; ++i) {
    double task_load = my_stats->objData[i].wallTime;
    if (task_load > 0.0001 && my_stats->objData[i].migratable) {
      // task_to_destination_map.emplace({my_id, my_id, i}); // Final destination map
      all_tasks.push_back(std::make_tuple(my_id, my_id, i, task_load)); // Vector of temporary migration tokens
      ordered_tasks.push_back(NaiveWorkUnit(i, task_load));
      max_task = (max_task > task_load) ? max_task : task_load;
      my_load += my_stats->objData[i].wallTime;
    }
  }


  if (_lb_args.debug() > 1) {
    CkPrintf("%d, Load at Start: %lf\n", CkMyPe(), my_load);
  }

  // CalculateMigrations();
  double data = my_load;
  CkCallback cb = CkCallback(CkReductionTarget(DiscreteSelfishLB, LoadReduction), thisProxy);
  contribute(sizeof(double), &data, CkReduction::sum_double, cb);
}

void DiscreteSelfishLB::LoadReduction(double x) {
  avg_load = x/(double)CkNumPes();
  double pack_load = 0.025*avg_load; // calc_pack_load(avg_load, max_task);
  CkPrintf("Pau no meu cu memo, feio %lf, %lf, %lf\n", avg_load, pack_load);
  double pack_count = my_load/pack_load + 1;
  CreatePacks(pack_count);
  CalculateMigrations();
}

void DiscreteSelfishLB::CreatePacks(int n_packs) {
  local_packs = {};
  // CkPrintf("[%d] %d packs\n", CkMyPe(), n_packs);
  for (int i = 0; i < n_packs; i++) {
    local_packs.push_back(NaivePack());
  }

  // Make ordered_tasks into a heap
  std::make_heap(local_packs.begin(), local_packs.end(), std::greater<NaivePack>{});
  std::make_heap(ordered_tasks.begin(), ordered_tasks.end());

  while (ordered_tasks.size() > 0) {
    std::pop_heap(local_packs.begin(), local_packs.end(),
      [](const NaivePack a, const NaivePack b){ return a.load() > b.load(); }); // Min Heap on Packs
    NaivePack pack = local_packs.back();
    local_packs.pop_back();
    std::pop_heap(ordered_tasks.begin(), ordered_tasks.end(),
      [](const NaiveWorkUnit a, const NaiveWorkUnit b){ return a.get_load() < b.get_load(); }); // Max Heap on Tasks
    auto task = ordered_tasks.back();
    pack.add(task);
    local_packs.push_back(pack);
    std::push_heap(local_packs.begin(), local_packs.end(),
      [](const NaivePack a, const NaivePack b){ return a.load() > b.load(); }); // Min Heap on Packs
    ordered_tasks.pop_back();
  }
  if ( _lb_args.debug() > 2 ) {
    for (auto pack : local_packs) {
      CkPrintf("[%d] I have a pack of load %lf\n", CkMyPe(), pack.load());
    }
  }
}

void DiscreteSelfishLB::CalculateMigrations() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, CkNumPes()-1);

  for (int i = 0; i < all_tasks.size(); i++) {
    auto task = all_tasks[i];
    auto random_target = dis(gen);
    if (random_target != CkMyPe() && std::get<1>(task) == CkMyPe()) {
      thisProxy[random_target].ReceiveTask(CkMyPe(), my_load,
        std::get<2>(task), std::get<3>(task), i);
      waiting_messages++;
    }
  }

  if (!waiting_messages) {
    cur_iteration++;
    double tmp = my_load;
    CkCallback cb = CkCallback(CkReductionTarget(DiscreteSelfishLB, NextIteration), thisProxy);
    contribute(sizeof(double), &tmp, CkReduction::max_double, cb);
  }
}

void DiscreteSelfishLB::ReceiveTask(int host_id, double host_load, int task_id, double task_load,
    int arr_position) {
  bool receive = DetermineMigrationSuccess(host_load, task_load);
  if (receive) {
    my_load += task_load;
    all_tasks.push_back(std::make_tuple(host_id, CkMyPe(), task_id, task_load));
  }
  thisProxy[host_id].ConfirmRecv(receive, CkMyPe(), arr_position);
}

void DiscreteSelfishLB::ConfirmRecv(bool recv, int who, int arr_position) {
  waiting_messages--;
  if (recv) {
    auto task = all_tasks[arr_position]; // Modifying this should modify the one in the array
    all_tasks[arr_position] = std::make_tuple(std::get<0>(task),
      who, std::get<2>(task), std::get<3>(task));
    my_load -= std::get<3>(task);
  }
  if (!waiting_messages) {
    cur_iteration++;
    double tmp = my_load;
    CkCallback cb = CkCallback(CkReductionTarget(DiscreteSelfishLB, NextIteration), thisProxy);
    contribute(sizeof(double), &tmp, CkReduction::max_double, cb);
  }
}

void DiscreteSelfishLB::NextIteration(double makespan) {
  _discrete_this_iteration_load(cur_iteration, "Makespan", makespan);
  if (cur_iteration >= kIterations) {
    // Prepare to inform moves of tasks in remote peers
    std::map< int, std::map<int, int> > remote_task_map; // PE_ID -> Task -> Target
    for (auto task : all_tasks) {
      if (std::get<0>(task) != CkMyPe()) {
        remote_task_map[std::get<0>(task)][std::get<2>(task)] = std::get<1>(task);
      }
    }
    for (auto entry : remote_task_map) {
      int remote_host = entry.first;
      int msg_size = entry.second.size();
      int* tids = new int[msg_size]; int* targets = new int[msg_size];
      // This could be optimized to one contiguous array, accessing 2 by 2, but the code would be less readable
      auto it = entry.second.begin();
      for (int i = 0; i < msg_size; i++, it++) {
        auto task_target = (std::pair<int,int>)(*it);
        tids[i] = task_target.first; targets[i] = task_target.second;
      }
      thisProxy[remote_host].MoveTasks(CkMyPe(), msg_size, tids, targets);
      waiting_messages++;
    }
    if (!waiting_messages){
      contribute(CkCallback(CkReductionTarget(DiscreteSelfishLB, FinishLoadBalance), thisProxy));
    }
  } else {
    CalculateMigrations();
  }
}

void DiscreteSelfishLB::MoveTasks(int who, int msg_size, int* tids, int* targets) {
  int i = 0;
  int tid = tids[i]; int target = targets[i];
  int _task; int _target; int _src; double _load;
  for (int j = 0; i < msg_size && j < all_tasks.size(); j++) {
    std::tie(_src, _target, _task, _load) = all_tasks[j];
    if (_task == tid && _src == CkMyPe()) {
      all_tasks[j] = std::make_tuple(_src, _target, _task, _load);
    }
  }
  thisProxy[who].ConfirmTasks();
}


void DiscreteSelfishLB::ConfirmTasks() {
  waiting_messages--;
  if (!waiting_messages){
    contribute(CkCallback(CkReductionTarget(DiscreteSelfishLB, FinishLoadBalance), thisProxy));
  }
}

void DiscreteSelfishLB::FinishLoadBalance() {
  migrateInfo.reserve(all_tasks.size());
  for (auto task : all_tasks) {
    if (std::get<0>(task) == CkMyPe() && std::get<1>(task) != CkMyPe()) {
      // This is a leaving task
      // task_to_destination_map.emplace(std::get<1>(task), std::get<2>(task));
      int tid = std::get<2>(task); int destination = std::get<1>(task);
      MigrateInfo* inf = new MigrateInfo();
      inf->obj = my_stats->objData[tid].handle;
      inf->from_pe = CkMyPe();
      inf->to_pe = destination;
      migrateInfo.push_back(inf);
      mig_counts[destination]++;
      leaving++;
    }
  }
  InsertMigrateInfo();
}

bool DiscreteSelfishLB::DetermineMigrationSuccess(double host_load, double task_load) {
  bool ret_val = false;
  if (my_load + task_load < host_load) {
    double chance = 1.0 - (my_load/host_load);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    ret_val = (chance > dis(gen));
  }
  return ret_val;
}

void DiscreteSelfishLB::InsertMigrateInfo() {
  msg = new(leaving, CkNumPes(), CkNumPes(), 0) LBMigrateMsg;
  msg->n_moves = leaving;
  for (size_t i = 0; i < leaving; ++i) {
      MigrateInfo* inf = (MigrateInfo*) migrateInfo.at(i);
      msg->moves[i] = *inf;
      delete inf;
  }
  migrateInfo.clear();
  for (int dest = 0; dest < CkNumPes(); dest++) {
    thisProxy[dest].InformMigrations(mig_counts[dest]);
  }

  if (_lb_args.debug() > 3) CkPrintf("[%d] Confirm final barrier commit with load %lf\n", CkMyPe(), my_load);
  contribute(CkCallback(CkReductionTarget(DiscreteSelfishLB, FinalBarrier), thisProxy));
}

void DiscreteSelfishLB::FinalBarrier() {
  if (_lb_args.debug() > 2)
    CkPrintf("[%d] I have received a total of %d tasks\n", CkMyPe(), migrates_expected);
  ProcessMigrationDecision(msg);
}

void DiscreteSelfishLB::InformMigrations(int count) {
  migrates_expected += count;
}

#include "DiscreteSelfishLB.def.h"
