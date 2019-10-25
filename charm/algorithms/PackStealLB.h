/***
 * Author: Vinicius Freitas (vinicius.mct.freitas@gmail.com)
 *
 * This is a locality-friendly, work-stealing-based, distributed load balancing
 * strategy for Charm++.
 *
 * It is based on 3 main steps:
 * 1. Aggregating system load information to determine the optimal pack size
 * 2. PEs communicating to exchange migration tokens
 * 3. Confirm and send migrations to be performed by the RTS
 *
 ***/

#ifndef PACK_STEAL_LB_H
#define PACK_STEAL_LB_H

// Charm includes
#include "DistBaseLB.h"
#include "UpdateProcMap.h"
#include "UpdateWorkMap.h"
#include "PackStealLB.decl.h"

// Data-structure includes


// PackSteal class declaration
class PackStealLB : public CBase_PackStealLB {

public:
    PackStealLB(const CkLBOptions &);
    PackStealLB(CkMigrateMessage *m);
    void turnOn();
    void turnOff();
    void AvgLoadReduction(double x);
    void DetermineMigratingWork();
    void StealLoad(int thief_id, double stolen_load, int n_info, int ids[], double loads[], double times[], int n_tries);
    void GiveLoad(int n_tasks, double total_load, int seeking, int n_info, int id_arr[], double load_arr[], double times_arr[], int from);
    void EndBarrier();
    void MakespanReport(double, int);
    void MigrationReport(int, int);
    void SuggestSteal(int, double, double, int, int);
    void FinishLoadBalance();
    void IsDone(int n_info, int done_pes[]);

protected:
    int total_migrates;
    int num_packs;

private:
    void InitLB(const CkLBOptions &opt);
    const std::pair<int, double> CalculateStealingLoad();
    int ChooseRandomReceiver();
    int VectorizeMap(int*& id_arr, double*& load_arr, double*& times_arr); //retval: size of ProcMap // Allocs 3 arrays
    void InitWorkMap();
    void StealAttempt(int suggestion);
    void AddToMigrationMessage(int to, int task_id);
    void UpdateTotalLoad(int n_tasks, double increased);
    void Strategy(const DistBaseLB::LDStats* const);
    void PrintFinalSteps();

    double avg_load, lb_load, ub_load; // Average, lower and upper bound system loads
    double my_load; double pack_load;
    int max_steal_attempts;
    int failed_attempts;
    int current_attempts;
    int next_victim;
    int remaining_steals;
    int temp_neg_tasks = 0;

    bool finished, really_finished, has_packs, lb_started;
    double makespan_report; int migration_report;
    int makespan_report_count, migration_report_count;

    int pack_count;
    // Ex: 0, 4, 6. (Two batches, from 0-3 and 4-6 positions in the other 2 arrays).
    std::vector<int> batch_delimiters;
    std::vector<int> migrate_ids;
    std::vector<double> migrate_loads;
    std::vector<MigrateInfo*> migrateInfo;

    std::set<int> done_pe_list;
    std::set<int> failed_steals;

    LBMigrateMsg* msg;
    const DistBaseLB::LDStats* my_stats;

    ProcMap remote_pe_info;
    WorkMap local_work_info;

    CProxy_PackStealLB thisProxy;

};

#endif // PACK_STEAL_LB_H
