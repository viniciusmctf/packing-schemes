#include "CkDistGraphProxy.h"

using ckutil::HashMap;

LocalGraph::~LocalGraph() {
    migrations.clear();
    vertices.clear();
    for (auto& f : frontiers) {
        f.clear();
    }
    frontiers.clear();
    edges.clear();
    hash_map.the_map.clear();
    hash_map.hash_size = 0;
    built = false;
}

virtual void LocalGraph::init(const DistBaseLB::stats* const st) {
    using namespace DGM;
    if (!built) {
        hash_map = HashMap(st);
        hash_map.init();

        // Build list of vertices
        auto n = st->n_objs;
        auto objs = st->objData;
        total_load = 0.0;
        for (int i = 0; i < n; ++i) {
            auto load = objs[i].wallTime;
            total_load += load;
            if (objs[i].migratable) {
                Vertex v = Vertex(i, load);
                vertices.emplace(hash_map.getHash(objs[i]), v);
            }
            edges.emplace(i, {});
        }

        // Parse through communication
        auto m = st->n_comm;
        auto comms = st->commData;
        int mcasts = 0;
        for (int j = 0; j < m; ++j) {
            auto& ckedge = comms[j];

            if (!ckedge.from_proc()) { // If the message is from an obj ...
                if (ckedge.recv_type() == LD_OBJ_MSG) { // Obj -> Obj
                    auto to_info = ckedge.receiver;
                    auto to_pe = to_info.dest.destObj.destObjProc;

                    if (to_pe == my_id) { // Local communication
                        auto from = hash_map.getHash(ckedge.sender);
                        auto to   = hash_map.getHash(ckedge.receiver.get_destObj());
                        if (from < 0 || to_h < 0) continue; // Case non-registered
                        try {
                            if (edges.count(from)) {
                                edges[from].emplace(Edge(from, to))
                            } else { // Incoming Remote communication -> to
                                // Add to frontier
                            }
                        } catch (const auto& except) {}

                    } else { // Outgoing Remote communication
                        // Add to Frontier
                    }
                } else if (ckedge.recv_type() == LD_OBJLIST_MSG) { // Multicast
                    // Disconsider for now. count the overall number of messages.
                    mcasts += ckedge.messages;
                }
            }
        }
        built = true;
    }
}
