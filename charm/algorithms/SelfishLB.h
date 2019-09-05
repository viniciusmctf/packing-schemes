#ifndef _SELFISH_LB_
#define _SELFISH_LB_

#include "DistBaseLB.h"
#include "SelfishLB.decl.h"

#include <vector>
#include <tuple>
#include <queue>
#include <unordered_map>

void CreateSelfishLB();

class SelfishLB : public CBase_SelfishLB {
  using task_global_id_t = std::tuple<int, int, int, double>; // source, current, src_id, load
public:
    SelfishLB(const CkLBOptions&);
    SelfishLB(CkMigrateMessage *m);
    void ReceiveTask(int host_id, double host_load, int task_id, double task_load, int arr_position);
    void ConfirmRecv(bool recv, int who, int arr_position);
    void MoveTasks(int msg_size,int task_id[], int destination[]);
    void NextIteration(); // Reduction target, for iterations

private:
   // Private functions
   void InitLB(const CkLBOptions &);
   void Strategy(const DistBaseLB::LDStats* const stats);
   void InsertMigrateInfo(int task_id, int destination); // Fool proof migrateInfo cretion

   // Random Decision Process
   void CalculateMigrations(); // Decide which tasks will go where and issue the "ReceiveTask" calls
   bool DetermineMigrationSuccess(double host_load); // my_load is available in context

   // Atributes
   std::vector<MigrateInfo*> migrateInfo;
   std::vector< std::pair <int, int> > task_to_destination_map;
   std::vector< task_global_id_t > all_tasks;
   double my_load;
   int waiting_messages;

  // Random Attributes
  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen; //Standard mersenne_twister_engine seeded with rd()

   // Magical constants
   int kIterations;

   // Charm Attributes
   LBMigrateMsg* msg;
   const DistBaseLB::LDStats* my_stats;
   CProxy_SelfishLB thisProxy;
};



#endif /* _SELFISH_LB_ */
