#include "SelfishLB.h"
#include <random>

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

  gen(rd()); // Set random device
  kIterations = 10;
  waiting_messages = 0;
  my_load = 0.0;
  all_tasks.clear(); migrateInfo.clear();
  my_stats = stats;
  int nobjs = my_stats->n_objs;
  all_tasks.reserve(2*nobjs);
  task_to_destination_map.clear();
  task_to_destination_map.reserve(nobjs);
  int my_id = CkMyPe();

  for (int i = 0; i < nobjs; ++i) {
    if (my_stats->objs[i].wallTime > 0.0001 && my_stats->objs[i].migratable) {
      task_to_destination_map.emplace({my_id, my_id, i}); // Final destination map
      all_tasks.emplace({my_id, my_id, i, my_stats->objs[i].wallTime}); // Vector of temporary migration tokens
    }
  }

  CalculateMigrations();
}

void SelfishLB::CalculateMigrations() {
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
  my_load += task_load;
  bool receive = DetermineMigrationSuccess(host_load);
  if (receive) {
    all_tasks.emplace({host_id, CkMyPe(), task_id, task_load});
  } else {
    my_load -= task_load;
  }
  thisProxy[host_id].ConfirmRecv(receive, CkMyPe(), arr_position);
}

void SelfishLB::ConfirmRecv(bool recv, int who, int arr_position) {
  waiting_messages--;
  if (recv) {
    auto task = all_tasks[arr_position]; // Modifying this should modify the one in the array
    all_tasks[arr_position] = std::make_tuple(std::get<0>(task),
      who, std::get<2>(task), std::get<3>(task));
    my_load -= task_load;
  }
  if (!waiting_messages) {
    // Contribute for Iteration end
  }
}
