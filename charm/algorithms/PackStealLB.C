#include "PackStealLB.h"

CreateLBFunc_Def(PackStealLB, "Master-backpack-thief load balancer")

PackStealLB::PackStealLB(CkMigrateMessage *m) : CBase_PackStealLB(m) {}

PackStealLB::PackStealLB(const CkLBOptions &opt) : CBase_PackStealLB(opt) {
  lbname = "PackStealLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] PackStealLB created!!\n",CkMyPe());
  InitLB(opt);
}

void PackStealLB::turnOn() {
 #if CMK_LBDB_ON
  theLbdb->getLBDB()->
    TurnOnBarrierReceiver(receiver);
  theLbdb->getLBDB()->
    TurnOnNotifyMigrated(notifier);
  theLbdb->getLBDB()->
    TurnOnStartLBFn(startLbFnHdl);
 #endif
}

void PackStealLB::turnOff() {
 #if CMK_LBDB_ON
  theLbdb->getLBDB()->
    TurnOffBarrierReceiver(receiver);
  theLbdb->getLBDB()->
    TurnOffNotifyMigrated(notifier);
  theLbdb->getLBDB()->
    TurnOffStartLBFn(startLbFnHdl);
 #endif
}

void PackStealLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_PackStealLB(thisgroup);
  if (opt.getSeqNo() > 0) turnOff();
}

/***
  * Here begin the auxiliary methods
 **/

void PackStealLB::InitWorkMap() {
  // Parse through LDStats to initialize work map
  int nobjs = my_stats->n_objs;
  std::vector<std::pair<int,double>> map;
  map.reserve(nobjs);
  migrateInfo = {};
  for (int i = 0; i < nobjs; ++i) {
    if (my_stats->objData[i].migratable && my_stats->objData[i].wallTime > 0.0001) {
      map.emplace_back(i, my_stats->objData[i].wallTime);
      // CkPrintf("[%d] Obj <%d> processed, <%lf>\n", CkMyPe(), i, my_stats->objData[i].wallTime);
    }
  }
  // CkPrintf("[%d] Done parsing\n", CkMyPe());
  //CkPrintf("[%d] I have a total of %d clients registered (nobjs + 2)\n", CkMyPe(), nobjs+2);
  local_work_info = WorkMap(); local_work_info.clear();
  local_work_info = WorkMap(map);
  migrateInfo.reserve(nobjs);
  // CkPrintf("[%d] WorkMapInit done\n", CkMyPe());
}

inline int mod_int(int a) {
  return (a >= 0 ? a : -a);
}


const std::pair<int, double> PackStealLB::CalculateStealingLoad() {
  // Calculate the batch size for this particular application scenario
  // (0.01)*avg_load (1% of the average load)
  // Based on this stealers will know how many packs they can afford to steal.
  static double size_of_one_pack = 0.02*avg_load;
  static int original_num_packs = mod_int((my_load-lb_load)/size_of_one_pack);
  int ret_num_packs = num_packs == 0 ? mod_int((my_load-lb_load)/size_of_one_pack) : num_packs;
  num_packs = ret_num_packs;
  return {ret_num_packs, size_of_one_pack};
}

int PackStealLB::ChooseRandomReceiver() {
  int id;
  if (failed_attempts >= CkNumPes()/4) {
    id = std::rand()%CkNumPes();
  } else {
    id = remote_pe_info.get_random_id()%CkNumPes();
  }
  if (id == CkMyPe() || failed_steals.count(id)) {
    do {
      id = std::rand()%CkNumPes();
    } while(id == CkMyPe() || failed_steals.count(id));
  }
  // CkPrintf("[%d] Chosen id: %d\n", CkMyPe(), id);
  return id;
}

int PackStealLB::VectorizeMap(int*& id_arr, double*& load_arr, double*& times_arr) {
  int info_count = remote_pe_info.size();
  id_arr = new int[info_count];
  load_arr = new double[info_count];
  times_arr = new double[info_count];
  remote_pe_info.vectorize_data(id_arr, load_arr, times_arr);
  //for (int i = 0; i < info_count; i++) {
    //CkPrintf("[%d] Map index <%d>: %d, %lf, %lf\n", CkMyPe(), i, id_arr[i], load_arr[i], times_arr[i]);
  //}
  return info_count;
}  //retval: size of ProcMap // Allocs 2 arrays // Remember to delete on flow!

void PackStealLB::AddToMigrationMessage(int to, int task_id) {
  if (_lb_args.debug() > 4) {
    CkPrintf("Migration log: %d, %d, %d\n", task_id, CkMyPe(), to);
  }
  MigrateInfo* migrating = new MigrateInfo;
  migrating->obj = my_stats->objData[task_id].handle;
  migrating->from_pe = CkMyPe();
  migrating->to_pe = to;
  migrateInfo.push_back(migrating);
}

void PackStealLB::DetermineMigratingWork() {
  // Define in a DOD oriented fashion the tasks this PE wants to migrate
  //CkPrintf("[%d] Defining migrate_ids and loads...\n", CkMyPe());
  pack_count = 0;
  migrate_ids = {};
  migrate_loads = {};
  batch_delimiters = {0};

  double total_removed_load = local_work_info.calculate_total_load() - lb_load;
  auto removed_objs = local_work_info.remove_batch_of_load(total_removed_load);
  auto steal_factor = CalculateStealingLoad();
  int n_steals = steal_factor.first;
  double max_bload = steal_factor.second;
  double curr_bload = 0.0;

  for (int i = 0; i < removed_objs.size(); i++) {
    auto& obj = removed_objs.at(i);
    migrate_ids.push_back(obj.sys_index);
    migrate_loads.push_back(obj.load);
    curr_bload += obj.load;
    if (curr_bload >= max_bload) {
      curr_bload = 0.0;
      batch_delimiters.push_back(i+1);
    }
  }
  pack_count = batch_delimiters.size();
  batch_delimiters.push_back(removed_objs.size());
}

void PackStealLB::UpdateTotalLoad(int n_tasks, double increased) {
  int temp = n_tasks - 1;
  local_work_info.insert_new({--temp_neg_tasks, increased});
  while (temp-- > 0) {
    local_work_info.insert_new({--temp_neg_tasks, 0});
  }
  my_load += increased;
}

void PackStealLB::StealAttempt(int suggestion) {
  auto steal_factor = CalculateStealingLoad();
  int steals = 2;// (current_attempts > 0) ? steal_factor.first : 1;
  //CkPrintf("[%d] I will attempt %d new steals!\n", CkMyPe(), steal_factor.first);
  if (suggestion == CkMyPe()) {
    suggestion++; suggestion %= CkNumPes();
  }
  for (int i = 0; i < steals; i++) {
    int n_info = remote_pe_info.size();
    int* id_arr; double* load_arr;
    double* times_arr;
    VectorizeMap(id_arr, load_arr, times_arr);
    // CkPrintf("[%d] I will attempt a steal on <%d> of load %lf! %d\n", CkMyPe(), victim, steal_factor.second, CkNumPes());
    thisProxy[suggestion].StealLoad(CkMyPe(), steal_factor.second, n_info, id_arr, load_arr, times_arr, current_attempts);
    current_attempts++;
  }
  // CkPrintf("[%d] I am underloaded: %lf!\n", CkMyPe(), my_load);
}

void PackStealLB::IsDone(int n_info, int done_pes[]) {
}

 /***
   * Here end the auxiliary methods
  **/

/***
 * START OF ALGORITHM EXECUTION FLOW
 **/
void PackStealLB::Strategy(const DistBaseLB::LDStats* const stats) {
  // Initialize local variables
  // CkPrintf("[%d] Here my legend starts!\n", CkMyPe());
  current_attempts = 0, pack_count = 0; failed_attempts = 0;
  total_migrates = 0, num_packs = 0;
  migrates_expected = 0;
  lb_started = false;
  done_pe_list.clear();
  failed_steals.clear();
  finished = false, really_finished = false;
  int mult_fact = 1;
  for (int i = 2; i < CkNumPes(); i *= i) {
    mult_fact++;
  }
  max_steal_attempts = CkNumPes() > 7 ? mult_fact : CkNumPes()*2;
  my_stats = stats;
  // Initialize work map
  InitWorkMap(); //local_work_info will have data from this point on.
  my_load = local_work_info.calculate_total_load();
  // Start reduction on average load
  CkCallback cb(CkReductionTarget(PackStealLB, AvgLoadReduction), thisProxy);
  double l = my_load;
  PrintFinalSteps();
  lb_started = true;
  contribute(sizeof(double), &l, CkReduction::sum_double, cb);

}

void PackStealLB::AvgLoadReduction(double total_load) {
  // Calculate average system load
  avg_load = total_load/CkNumPes();
  double vlb_load = 0.85*avg_load;
  lb_load = 0.95*avg_load;
  // CkPrintf("[%d] Avg sys load: %lf!\n", CkMyPe(), avg_load);
  ub_load = 1.05*avg_load;

  remote_pe_info = ProcMap{CkMyPe(), CkNumPes()};
  remote_pe_info.emplace_update(CkMyPe(), my_load, CmiWallTimer());
  if (my_load > ub_load) {
    // CkPrintf("[%d] I am overloaded: %lf!\n", CkMyPe(), my_load);
    DetermineMigratingWork();
    current_attempts = 0;
    has_packs = true;
    thisProxy[(CkMyPe()+1)%CkNumPes()].SuggestSteal(CkMyPe(), my_load, CmiWallTimer(), CkMyPe(), 0);
    // CkPrintf("[%d] I have produced %d packs!\n", CkMyPe(), pack_count);

  } else if (my_load < vlb_load) {
    has_packs = false;
    StealAttempt(ChooseRandomReceiver()); // Increments current_attempts
  } else {
    has_packs = false;
    finished = true;
  }

  // Start QD
  if (CkMyPe() == 0) {
    CkCallback cb(CkIndex_PackStealLB::EndBarrier(), thisProxy);
    CkStartQD(cb);
  }

  // Determine if I am overloaded or underloaded
  // Overloaded PEs will propagate their ID.
  // Underloaded PEs will try to steal load from their first neighbor (ID+1%numpes)
  // Increment "current_attempts" for each steal attempt. (decrement when receiving load)
  // If current_attempts reaches 0, contribute to finish.
  // Keep overloaded PEs current_attempts at -(num_created_batches).
  // // Increment when load is donated.
}

void PackStealLB::StealLoad(int thief_id, double stolen_load, int n_info, int ids[], double loads[], double times[], int n_tries) {
  // CkPrintf("[%d] They <%d> are stealing!!\n", CkMyPe(), thief_id);
  for (int i = 0; i < n_info; i++) {
    // CkPrintf("<%d, %.4lf, %.4lf>, ", ids[i], loads[i], times[i]);
    remote_pe_info.emplace_update(ids[i], loads[i], times[i]);
  }
  // CkPrintf("[%d] Table updated: comparing %lf X %lf\n", CkMyPe(), my_load, ub_load);

  if (my_load > ub_load && current_attempts < (batch_delimiters.size()-1) ) {
    if (!has_packs) { // DEFINE HAS_PACKS
      DetermineMigratingWork();
    }
    bool done_donating = false;
    double sent_load = 0.0;
    int count = 0;
    while (!done_donating) {
      double tmp_sent_load = 0.0;
      int begin_id = batch_delimiters[current_attempts];
      int end_id = batch_delimiters[++current_attempts]; // Update current_attempts
      for (int i = begin_id; i < end_id; i++) {
        AddToMigrationMessage(thief_id, migrate_ids[i]);
      }
      for (int i = begin_id; i < end_id; i++) {
        tmp_sent_load += migrate_loads[i];
        count++;
      }
      my_load -= tmp_sent_load;
      sent_load += tmp_sent_load;
      // If a reasonable increment would make sent_load >= than stolen load, stop
      if ((my_load <= ub_load && sent_load*1.3 >= stolen_load) || current_attempts >= (batch_delimiters.size()-1)) {
        break; // In practice, the same thing. The boolean is simply more elegant
        done_donating = true;
      }
    }
    int n_info = remote_pe_info.size();
    int* id_arr; double* load_arr;
    double* times_arr;
    VectorizeMap(id_arr, load_arr, times_arr);
    thisProxy[thief_id].GiveLoad(count, sent_load, 0, n_info, id_arr, load_arr, times_arr, CkMyPe());
    total_migrates += count;
    if (current_attempts >= batch_delimiters.size()-1) {
      has_packs = false;
    }
    if (max_steal_attempts-current_attempts <= 0 && !finished) {
      finished = true;
    }

  } else {
    if ((n_tries > max_steal_attempts || my_load >= ub_load) && local_work_info.size() > 1) {
      double donating = stolen_load/2.0;
      double steal_load = stolen_load;
      auto obj_vec = local_work_info.remove_batch_of_load(donating);
      if (obj_vec.size() < 1) {
        return;
      }

      double accum = 0.0; int donations = 0;
      // CkPrintf("[%d] Thief in commit message: %d\n", CkMyPe(), thief_id);
      //CkPrintf("[%d] Creating pack in forced donation; total of %d objs\n", CkMyPe(), obj_vec.size());
      for (auto obj : obj_vec) {
        AddToMigrationMessage(thief_id, obj.sys_index);
        accum += obj.load;
        donations++;
        //CkPrintf("[%d] Donating Object of load %lf\n", CkMyPe(), obj.load);
      }
      // CkPrintf("[%d] I am able to donate an extra %lf, but not more :(\n", CkMyPe(), accum);
      int n_info = remote_pe_info.size();
      int* id_arr; double* load_arr;
      double* times_arr;
      VectorizeMap(id_arr, load_arr, times_arr); //CkPrintf("[%d] After Vectorize call:\n", CkMyPe());
      if (donations < 1) {
        return;
        int victim = remote_pe_info.last().first;
        if (victim == CkMyPe()) {
          remote_pe_info.last().first;
        }
        thisProxy[thief_id].SuggestSteal(CkMyPe(), my_load, CmiWallTimer(), victim, 2);
        return;
      }
      thisProxy[thief_id].GiveLoad(donations, accum, 1, n_info, id_arr, load_arr, times_arr, CkMyPe());
      // steal_load -= accum;
      my_load -= accum;
      total_migrates += donations;
      remote_pe_info.emplace_update(CkMyPe(), my_load, CmiWallTimer());
    } else {
      int victim = remote_pe_info.last().first;
      if (victim == CkMyPe()) {
        victim = (victim+1)%CkNumPes();
      }
      //CkPrintf("[%d] Underloaded Victim Case <%.3f>, cannot donate any to %d. I suggest %d\n", CkMyPe(), my_load, thief_id, victim);
      thisProxy[thief_id].SuggestSteal(CkMyPe(), my_load, CmiWallTimer(), victim, 1);
      if (local_work_info.size() <= 1 || my_load <= 1.05*ub_load) {
        if (!finished) {
          finished = true;
        }
      }
    }

  }

  if (!finished && my_load <= ub_load*1.05) {
    // CkPrintf("[%d] I have finished!\n", CkMyPe());
    finished = true;
    // done_pe_list.insert(CkMyPe());
  }
  // Update remote pe info
  // Verify if able to donate load
  // If yes, donate via GIVE LOAD
  // If not, replicate thief id with renewed record data
}

void PackStealLB::SuggestSteal(int from_id, double from_load, double from_time, int suggested_victim, int count) {
  //CkPrintf("[%d] Educated Steal hint from %d to %d\n", CkMyPe(), from_id, suggested_victim);
  remote_pe_info.emplace_update(from_id, from_load, from_time);
  if (finished || count > CkNumPes()/4 || from_id == CkMyPe()) {
    return;
  } else if (my_load < lb_load) {
    StealAttempt(suggested_victim);
  } else {
    int thief = remote_pe_info.first().first;
    if (thief == CkMyPe()) {
      return;
    }
    thisProxy[thief].SuggestSteal(CkMyPe(), my_load, CmiWallTimer(), suggested_victim, ++count);
  }
}

void PackStealLB::GiveLoad(int n_tasks, double total_load, int seek, int n_info, int ids[], double loads[], double times[], int from) {
  //CkPrintf("[%d] I am receiving load %lf from %d New info: %d, Ntasks: %d\n", CkMyPe(), total_load, from, n_info, n_tasks);
  UpdateTotalLoad(n_tasks, total_load);
  migrates_expected += n_tasks;
  current_attempts--;
  for (int i = 0; i < n_info; i++) {
    // CkPrintf("<%d, %.4lf, %.4lf>, ", ids[i], loads[i], times[i]);
    remote_pe_info.emplace_update(ids[i], loads[i], times[i]);
  }
  // CkPrintf("Updating my actual load after the fact... \n");
  remote_pe_info.emplace_update(CkMyPe(), my_load, CmiWallTimer());

  if (my_load > lb_load || finished) {
    finished = true;
    return;
  }

  if (seek == 1) {
    failed_attempts++;
  }

  if (my_load < lb_load && seek != 1 && failed_attempts < CkNumPes()/2) {
    // CkPrintf("[%d] Can't stop failing...\n", CkMyPe());
    failed_attempts++;
    failed_steals.insert(from);
    int suggestion = ChooseRandomReceiver();
    StealAttempt(suggestion);
  }

  if (current_attempts <= 0 && !finished) {
    // CkPrintf("[%d] I have finished!\n", CkMyPe());
    finished = true;
    // done_pe_list.insert(CkMyPe());
  }
}

void PackStealLB::EndBarrier() {
  really_finished = true;
  msg = new(total_migrates,CkNumPes(),CkNumPes(),0) LBMigrateMsg;
  msg->n_moves = total_migrates;
  for(int i = 0; i < total_migrates; i++) {
    MigrateInfo* item = (MigrateInfo*) migrateInfo[i];
    msg->moves[i] = *item;
    delete item;
    migrateInfo[i] = 0;
  }
  migrateInfo.clear();
  //CkPrintf("[%d] Finishing! Registered a total of %d migrations, received a total of %d\n", CkMyPe(), total_migrates, migrates_expected);
  PrintFinalSteps();
  ProcessMigrationDecision(msg);

  // contribute(CkCallback(CkReductionTarget(PackStealLB, FinishLoadBalance),thisProxy));
}

void PackStealLB::FinishLoadBalance() {
  // CkPrintf("[%d] I am actually done!\n", CkMyPe());
  ProcessMigrationDecision(msg);
}
/***
 * END OF ALGORITHM EXECUTION FLOW
 **/

void PackStealLB::PrintFinalSteps() {
  if (_lb_args.debug() > 1) {
    const char* step = really_finished ? "finish" : "start";
    CkPrintf("[%d] CSV> pe_load; %s; %d; %lf;\n", CkMyPe(), step, CkMyPe(), my_load); // start/finish; Me; MyLoad
    CkPrintf("[%d] CSV> num_tasks; %d; %d\n", CkMyPe(), CkMyPe(), local_work_info.size()); // Me; MyTasks
    CkPrintf("[%d] CSV> migs; %d; %d; %d\n", migrates_expected, total_migrates); // Me; RecvTasks; MigTasks
  }
}

#include "PackStealLB.def.h"
