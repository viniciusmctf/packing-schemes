#ifndef _PACK_SELFISH_LB_
#define _PACK_SELFISH_LB_

#include "DistBaseLB.h"
#include "DistributedGreedyPackingLB.decl.h"

#include <vector>
#include <tuple>
#include <queue>
#include <unordered_map>
#include <random>

void CreateDistributedGreedyPackingLB();

class DistributedGreedyPackingLB : public CBase_DistributedGreedyPackingLB {
  using task_global_id_t = std::tuple<int, int, int, double>; // source, current, src_id, load
public:
    DistributedGreedyPackingLB(const CkLBOptions&);
    DistributedGreedyPackingLB(CkMigrateMessage *m);
    void StartTaskTrade(int host_id, double host_load);
    void RespondTrade(bool ok, int remote_id, double remote_load);
    void SendTasks(int host_id, double host_load, int info_count, int task_ids[], double task_loads[], int arr_positions[]);
    void MoveTasks(int who, int msg_size,int task_id[], int destination[]);
    void ConfirmTasks();
    void InformMigrations(int count);
    void NextIteration(double); // Reduction target, for iterations
    void FinishLoadBalance();
    void FinalBarrier();

private:
   // Private functions
   void InitLB(const CkLBOptions &);
   void Strategy(const DistBaseLB::LDStats* const stats);
   void InsertMigrateInfo(); // Fool proof migrateInfo creation

   // Random Decision Process
   void CalculateRequests(); // Decide which tasks will go where and issue the "ReceiveTask" calls
   void ToNextIteration();
   bool DetermineMigrationSuccess(double host_load, double task_load); // my_load is available in context
   std::tuple< std::vector<double>, std::vector<int>, std::vector<int> > BuildPack(double max_load, int to);

   // Atributes
   std::vector<MigrateInfo*> migrateInfo;
   std::map<int, int> task_to_destination_map;
   std::vector< task_global_id_t > all_tasks;
   std::vector<int> mig_counts;
   double my_load;
   int waiting_messages, leaving, cur_iteration;

  // Random Attributes

   // Magical constants
   int kIterations;
   double kMigProb = 0.5;

   // Charm Attributes
   LBMigrateMsg* msg;
   const DistBaseLB::LDStats* my_stats;
   CProxy_DistributedGreedyPackingLB thisProxy;
};



#endif /* _PACK_SELFISH_LB_ */
