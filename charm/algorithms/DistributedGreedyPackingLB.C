#include "DistributedGreedyPackingLB.h"
#include <random>

void __this_iteration_load(int it, const char* string, double makespan) {
  if (CkMyPe() == 0 && _lb_args.debug() > 1) {
    CkPrintf("[%d] %s %lf\n", it, string, makespan);
  }
}

CreateLBFunc_Def(DistributedGreedyPackingLB, "Greedy Selfish Task Reallocation");

DistributedGreedyPackingLB::DistributedGreedyPackingLB(CkMigrateMessage *m) : CBase_DistributedGreedyPackingLB(m) {
}

DistributedGreedyPackingLB::DistributedGreedyPackingLB(const CkLBOptions &opt) : CBase_DistributedGreedyPackingLB(opt) {
  lbname = "DistributedGreedyPackingLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] DistributedGreedyPackingLB created\n",CkMyPe());
  InitLB(opt);
}

void DistributedGreedyPackingLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_DistributedGreedyPackingLB(thisgroup);
}

void DistributedGreedyPackingLB::Strategy(const DistBaseLB::LDStats* const stats) {
  if (CkMyPe() == 0) {
    CkPrintf("Starting DistributedGreedyPackingLB...\n");
  }
  if (_lb_args.lbpacksize() > 0 && _lb_args.lbpacksize() < CkNumPes()) {
    kIterations = _lb_args.lbpacksize();
  } else {
    kIterations = CkNumPes();
  }
  kMigProb = 0.8;
  cur_iteration = 1;
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

  if (_lb_args.debug() > 1) {
    CkPrintf("%d, Load at Start: %lf\n", CkMyPe(), my_load);
  }

  CalculateRequests();
}

void DistributedGreedyPackingLB::CalculateRequests() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, CkNumPes()-1);
  auto random_target = dis(gen);
  while (random_target == CkMyPe()) {
    random_target = dis(gen);
  }
  thisProxy[random_target].StartTaskTrade(CkMyPe(), my_load);
}

void DistributedGreedyPackingLB::StartTaskTrade(int host_id, double host_load) {
  bool response = DetermineMigrationSuccess(host_load);
  thisProxy[host_id].RespondTrade(response, CkMyPe(), my_load);
}

void DistributedGreedyPackingLB::RespondTrade(bool ok, int remote_id, double remote_load) {
  if (ok) {
    double delta = my_load - remote_load;
    auto pack = BuildPack(delta, remote_id);
    auto loads = std::get<0>(pack);
    auto ids = std::get<1>(pack);
    auto arr_ids = std::get<2>(pack);
    int size = ids.size();
    int* _ids = new int[size];
    int* _arr_ids = new int[size];
    double* _loads = new double[size];
    for (int i = 0; i < size; i++) {
      _ids[i] = ids[i];
      _arr_ids[i] = arr_ids[i];
      _loads[i] = loads[i];
    }
    thisProxy[remote_id].SendTasks(CkMyPe(), my_load, size, _ids, _loads, _arr_ids);
  }
  ToNextIteration();
}

std::tuple< std::vector<double>, std::vector<int>, std::vector<int> >
  DistributedGreedyPackingLB::BuildPack(double max_load, int to) {
    using std::get;
  std::vector<int> ids;
  std::vector<int> arr_positions;
  std::vector<double loads;
  double total_load = 0.0;
  int limit = all_tasks.size();
  for(int i = 0; i < limit; ++i) {
    auto& task = all_tasks[i];
    if (get<1>(task) != CkMyPe()) continue; // Task not here
    if (total_load + get<3>(task) > max_load) continue; // Task overloads pack

    loads.push_back(get<3>(task)); // Add everything to pack
    ids.push_back(get<2>(task));
    arr_positions.push_back(i);

    total_load += get<3>(task); // Add to total_load of pack

    all_tasks[i] =std::make_tuple(get<0>(task), to, get<2>(task), get<3>(task));
  }
  my_load -= total_load; // Subtracts pack load from machine load
  return std::make_tuple(loads, ids, arr_positions);
}

void DistributedGreedyPackingLB::SendTasks(int host_id, double host_load,
  int info_count, int task_ids[], double task_loads[], int arr_positions[]) {

  for (int i = 0; i < info_count; i++) {
    all_tasks.push_back(std::make_tuple(host_id, CkMyPe(), task_ids[i], task_loads[i]));
    my_load += task_loads[i];
  }
}

void DistributedGreedyPackingLB::ToNextIteration() {
  cur_iteration++;
  double tmp = my_load;
  CkCallback cb = CkCallback(CkReductionTarget(DistributedGreedyPackingLB, NextIteration), thisProxy);
  contribute(sizeof(double), &tmp, CkReduction::max_double, cb);
}

void DistributedGreedyPackingLB::NextIteration(double makespan) {
  __this_iteration_load(cur_iteration, "Makespan", makespan);
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
      contribute(CkCallback(CkReductionTarget(DistributedGreedyPackingLB, FinishLoadBalance), thisProxy));
    }
  } else {
    CalculateRequests();
  }
}

void DistributedGreedyPackingLB::MoveTasks(int who, int msg_size, int* tids, int* targets) {
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


void DistributedGreedyPackingLB::ConfirmTasks() {
  waiting_messages--;
  if (!waiting_messages){
    contribute(CkCallback(CkReductionTarget(DistributedGreedyPackingLB, FinishLoadBalance), thisProxy));
  }
}

void DistributedGreedyPackingLB::FinishLoadBalance() {
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

bool DistributedGreedyPackingLB::DetermineMigrationSuccess(double host_load) {
  bool ret_val = false;
  if (my_load < host_load) {
    double chance = 1.0 - (my_load/host_load);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    ret_val = (kMigProb*chance > dis(gen));
  }
  return ret_val;
}

void DistributedGreedyPackingLB::InsertMigrateInfo() {
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
  contribute(CkCallback(CkReductionTarget(DistributedGreedyPackingLB, FinalBarrier), thisProxy));
}

void DistributedGreedyPackingLB::FinalBarrier() {
  if (_lb_args.debug() > 2)
    CkPrintf("[%d] I have received a total of %d tasks\n", CkMyPe(), migrates_expected);
  ProcessMigrationDecision(msg);
}

void DistributedGreedyPackingLB::InformMigrations(int count) {
  migrates_expected += count;
}

#include "DistributedGreedyPackingLB.def.h"
