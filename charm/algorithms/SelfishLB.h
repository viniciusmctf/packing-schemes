#ifndef _SELFISH_LB_
#define _SELFISH_LB_

#include "DistBaseLB.h"
#include "SelfishLB.decl.h"

#include <vector>
#include <tuple>
#include <queue>
#include <unordered_map>
#include <random>

void CreateSelfishLB();

class SelfishLB : public CBase_SelfishLB {
  using task_global_id_t = std::tuple<int, int, int, double>; // source, current, src_id, load
public:
    SelfishLB(const CkLBOptions&);
    SelfishLB(CkMigrateMessage *m);
    void ReceiveTask(int host_id, double host_load, int task_id, double task_load, int arr_position);
    void ConfirmRecv(bool recv, int who, int arr_position);
    void MoveTasks(int who, int msg_size,int task_id[], int destination[]);
    void ConfirmTasks();
    void InformMigrations(int count);
    void NextIteration(); // Reduction target, for iterations
    void FinishLoadBalance();
    void FinalBarrier();

private:
   // Private functions
   void InitLB(const CkLBOptions &);
   void Strategy(const DistBaseLB::LDStats* const stats);
   void InsertMigrateInfo(); // Fool proof migrateInfo creation

   // Random Decision Process
   void CalculateMigrations(); // Decide which tasks will go where and issue the "ReceiveTask" calls
   bool DetermineMigrationSuccess(double host_load, double task_load); // my_load is available in context

   // Atributes
   std::vector<MigrateInfo*> migrateInfo;
   std::map<int, int> task_to_destination_map;
   std::vector< task_global_id_t > all_tasks;
   std::vector<int> mig_counts;
   double my_load;
   int waiting_messages, cur_iteration;

  // Random Attributes

   // Magical constants
   int kIterations;
   double kMigProb;

   // Charm Attributes
   LBMigrateMsg* msg;
   const DistBaseLB::LDStats* my_stats;
   CProxy_SelfishLB thisProxy;
};



#endif /* _SELFISH_LB_ */
