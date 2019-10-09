#ifndef _PACK_DROP_LB_
#define _PACK_DROP_LB_

#include "DistBaseLB.h"
#include "PackDropLB.decl.h"

#include "OrderedElement.h"
#include "UpdateProcMap.h"
#include "UpdateWorkMap.h"
#include <vector>
#include <queue>
#include <unordered_map>

void CreatePackDropLB();

class PackDropLB : public CBase_PackDropLB {
public:
    PackDropLB(const CkLBOptions&);
    PackDropLB(CkMigrateMessage *m);
    void LoadSetup(CkReductionMsg* m); // Reduction Target
    void ChareSetup(int count);
    void PackAck(int pack_id, int from, int psize, double pload, bool force);
    void RecvAck(int pack_id, int to, double pload, bool success);
    void EndStep();
    void First_Barrier();
    void GossipLoadInfo(int, int, int, int[], double[]);
    void DoneGossip();
    void DetailsRedux(int migrations);
    void Final_Barrier(double ml);

private:
   // Private functions
   void InitLB(const CkLBOptions &);
   void Setup();
   void PackSizeDef();
   void LoadBalance();
   void SendLoadInfo();
   void ShowMigrationDetails();
   void ForcedPackSend(int pack_id, double pload, bool force);
   void PackSend(int pack_id = 0, int one_time = 0);
   void Strategy(const DistBaseLB::LDStats* const stats);
   bool QueryBalanceNow(int step) { return true; };
   void CalculateReceivers();
   void CalculateCumulateDistribution();
   int FindReceiver();

   // Gossip Algorithm information


   // Load balancing information
   ProcMap remote_pe_info;
   WorkMap local_work_info;

   // Atributes
   bool lb_end;
   bool lb_started;
   int acks_needed;
   int gossip_msg_count;
   int kMaxGossipMsgCount;
   int kPartialInfoCount;
   double avg_load;
   int chare_count;
   double tries;
   size_t total_migrates; // as in dist_lb
   int my_chares;
   double pack_load;
   double my_load;
   double threshold;
   bool is_receiving;
   int done;
   int pack_count;
   int req_hop;
   int underloaded_pe_count;
   int info_send_count;
   int rec_count;
   std::vector<MigrateInfo*> migrateInfo;
   std::list<int> non_receiving_chares;
   std::vector<int> receivers;
   std::vector<int> pe_no;
   std::vector<double> distribution;
   std::vector<double> loads;
   std::priority_queue<Element, std::deque<Element>> local_tasks;
   std::map<int, std::vector<int>> packs;

   // Charm Attributes
   LBMigrateMsg* msg;
   const DistBaseLB::LDStats* my_stats;
   CProxy_PackDropLB thisProxy;
};



#endif /* _PACK_DROP_LB_ */
