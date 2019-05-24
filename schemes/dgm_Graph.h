
#include "DistGraphModel.h"
#include <vector>
#include <map>
#include <set>

namespace DGM {
    using VertList = ::std::initializer_list<DGM::Vertex>;
    using EdgeList = ::std::initializer_list<DGM::Edge>;
    using VertMap = ::std::unordered_map<DGM::CkVrtId, DGM::Vertex>;
    using EdgeMap = ::std::map<DGM::CkVrtId, ::std::map<DGM::CkVrtId, DGM::CkVrtDis> >;


    class Graph {
    public:
        Graph();
        Graph(int n_vertices, int n_edges);
        Graph(VertList vertices, EdgeList edges);
        ~Graph();

    protected:
        VertMap vertices;
        EdgeMap edges;
    };

    using Partition = ::std::set<DGM::Vertex>;
    struct Partitions {
        Partitions() {}
        Partitions(int expected); // Reserve expected+1 on both structs
        std::vector<CkVrtId> add_partition(Partition p); // \retval: already allocated vertices

        std::set<CkVrtId> included_ids;
        std::vector<Partition> elements;
    };

    class GGP_Graph : public Graph {
    public:
        GGP_Graph() : Graph() {}
        GGP_Graph(int n_vertices, int n_edges); //: Graph(n_vertices, n_edges);
        GGP_Graph(std::initializer_list<Edge> edges);
        ~GGP_Graph();
        Partitions work(CkVrtWg part_size);

    private:
        Partitions parts;
        std::unordered_map<CkVrtId, int> partition_table; // a vertex -> its partition
        int part_count;


    };
};
