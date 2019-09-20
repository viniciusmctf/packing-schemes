#include "SelfishLB.h"
#include <random>

void __control_print(const char* string) {
  if (CkMyPe() == 0 && _lb_args.debug() > 1) {
    CkPrintf("%s\n", string);
  }
}

CreateLBFunc_Def(SelfishLB, "Greedy Selfish Task Reallocation");

SelfishLB::SelfishLB(CkMigrateMessage *m) : CBase_SelfishLB(m) {
}

SelfishLB::SelfishLB(const CkLBOptions &opt) : CBase_SelfishLB(opt) {
  lbname = "SelfishLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] SelfishLB created\n",CkMyPe());
  InitLB(opt);
}

void SelfishLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_SelfishLB(thisgroup);
}

void SelfishLB::Strategy(const DistBaseLB::LDStats* const stats) {
  if (CkMyPe() == 0) {
    CkPrintf("Starting SelfishLB...\n");
  }

  kIterations = _lb_args.lbpacksize(); cur_iteration = 1;
  mig_counts.clear();
  mig_counts = std::vector<int> (CkNumPes(), 0); // Fill mig_counts with 0s
  waiting_messages = 0; migrates_expected = 0;
  my_load = 0.0; leaving = 0;
  all_tasks.clear(); migrateInfo.clear();
  my_stats = stats;
  int nobjs = my_stats->n_objs;
  all_tasks.reserve(2*nobjs);
  task_to_destination_map.clear();
  int my_id = CkMyPe();

  for (int i = 0; i < nobjs; ++i) {
    if (my_stats->objData[i].wallTime > 0.0001 && my_stats->objData[i].migratable) {
      // task_to_destination_map.emplace({my_id, my_id, i}); // Final destination map
      all_tasks.push_back(std::make_tuple(my_id, my_id, i, my_stats->objData[i].wallTime)); // Vector of temporary migration tokens
      my_load += my_stats->objData[i].wallTime;
    }
  }

  CalculateMigrations();
}

void SelfishLB::CalculateMigrations() {
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
}

void SelfishLB::ReceiveTask(int host_id, double host_load, int task_id, double task_load,
    int arr_position) {
  bool receive = DetermineMigrationSuccess(host_load, task_load);
  if (receive) {
    my_load += task_load;
    all_tasks.push_back(std::make_tuple(host_id, CkMyPe(), task_id, task_load));
  }
  thisProxy[host_id].ConfirmRecv(receive, CkMyPe(), arr_position);
}

void SelfishLB::ConfirmRecv(bool recv, int who, int arr_position) {
  waiting_messages--;
  if (recv) {
    auto task = all_tasks[arr_position]; // Modifying this should modify the one in the array
    all_tasks[arr_position] = std::make_tuple(std::get<0>(task),
      who, std::get<2>(task), std::get<3>(task));
    my_load -= std::get<3>(task);
  }
  if (!waiting_messages) {
    cur_iteration++;
    contribute(CkCallback(CkReductionTarget(SelfishLB, NextIteration), thisProxy));
  }
}

void SelfishLB::NextIteration() {
  __control_print(".");
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
      contribute(CkCallback(CkReductionTarget(SelfishLB, FinishLoadBalance), thisProxy));
    }
  } else {
    CalculateMigrations();
  }
}

void SelfishLB::MoveTasks(int who, int msg_size, int* tids, int* targets) {
  for (int i = 0; i < msg_size; i++) {
    task_to_destination_map.emplace(tids[i], targets[i]);
  }
  thisProxy[who].ConfirmTasks();
}

void SelfishLB::ConfirmTasks() {
  waiting_messages--;
  if (!waiting_messages){
    contribute(CkCallback(CkReductionTarget(SelfishLB, FinishLoadBalance), thisProxy));
  }
}

void SelfishLB::FinishLoadBalance() {
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

bool SelfishLB::DetermineMigrationSuccess(double host_load, double task_load) {
  bool ret_val = false;
  if (my_load + task_load > host_load) {
    double chance = 1.0 - (my_load/host_load);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    ret_val = (chance > dis(gen));
  }
  return ret_val;
}

void SelfishLB::InsertMigrateInfo() {
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
  contribute(CkCallback(CkReductionTarget(SelfishLB, FinalBarrier), thisProxy));
}

void SelfishLB::FinalBarrier() {
  if (_lb_args.debug() > 1)
    CkPrintf("[%d] My load after LB: %lf\n", CkMyPe(), my_load);
  ProcessMigrationDecision(msg);
}

void SelfishLB::InformMigrations(int count) {
  migrates_expected += count;
}

#include "SelfishLB.def.h"
