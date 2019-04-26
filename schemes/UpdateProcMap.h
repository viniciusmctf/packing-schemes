// Description for an updatable proc map, a vector-based hash-ordered structure
// for storing PE information

#ifndef UPDATABLE_PE_MAP_H
#define UPDATABLE_PE_MAP_H

#include <vector>
#include <map>
#include <ckgraph.h>

struct MapNode {
    int _index;
    double _data;
    double _timestamp;
};

class ProcMap {
 public:
    ProcMap() : pe_id(CkMyPe()), num_pes(CkNumPes()) { map.reserve(CkNumPes()); }
    ProcMap(int pe_id, int num_pes) : pe_id(pe_id), num_pes(num_pes) { map.reserve(CkNumPes()); }
    int insert_update(MapNode); // returns 1 if node was updated, 0 otherwise
    int emplace_update(int, double, double); // returns 1 if node was updated, 0 otherwise
    int size();
    int get_random_id();
    std::pair<int, double> first() const;
    std::pair<int, double> last() const;
    void vectorize_data(int*,double*,double*);

 private:
    const MapNode& most_recent(const MapNode& a, const MapNode& b);
    std::vector< MapNode > map; // Heapfied
    int pe_id, num_pes;
};

#endif // UPDATABLE_PE_MAP_H
