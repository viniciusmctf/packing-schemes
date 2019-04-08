#include "CkDistGraphProxy.h"

#define _DIST_GRAPH_DEBUG_MODE_ 1

using ckutil::HashMap;
using DGM::LocalGraph;

LocalGraph::~LocalGraph() {
    migrations.clear();
    vertices.clear();
    for (auto& f : frontiers) {
        f.second.clear();
    }
    frontiers.clear();
    edges.clear();
    built = false;
}

void LocalGraph::init() {
    using namespace DGM;
    if (!built) {
        //hash_map = HashMap(st);
        hash_map.init();

        // Build list of vertices
        auto n = hash_map.st->n_objs;
        auto objs = hash_map.st->objData;
        total_load = 0.0;
        for (int i = 0; i < n; ++i) {
            auto load = objs[i].wallTime;
            total_load += load;
            auto hash = hash_map.getHash(objs[i].objID(), objs[i].omID());
            if (objs[i].migratable) {
                Vertex v = Vertex(i, load);
                vertices.emplace(hash, v); // GetHash should be i. Check this.
            }
            #if _DIST_GRAPH_DEBUG_MODE_
            printf("Checking hash values... Hash: %d, Id: %d\n", hash, i);
            #endif
            edges.emplace(hash, std::set<Edge>());
        }

        // Parse through communication
        auto m = hash_map.st->n_comm;
        auto comms = hash_map.st->commData;
        int mcasts = 0;
        for (int j = 0; j < m; ++j) {
            auto& ckedge = comms[j];

            if (!ckedge.from_proc()) { // If the message is from an obj ...
                if (ckedge.recv_type() == LD_OBJ_MSG) { // Obj -> Obj
                    auto to_info = ckedge.receiver;
                    auto to_pe = to_info.dest.destObj.destObjProc;
                    auto from = hash_map.getHash(ckedge.sender);
                    auto to   = hash_map.getHash(ckedge.receiver.get_destObj());

                    if (to_pe == my_id) { // Local communication
                        if (from < 0 || to < 0) continue; // Case non-registered
                        try {
                            if (edges.count(from)) {
                                edges[from].emplace(from, to);
                            } else { // Incoming Remote communication -> to
                                // This should not be registered. Issue suspiscious print.
                                printf("Weird things are happening in comm from %d to %d\n", from, to);
                            }
                        } catch (const std::out_of_range& except) {}

                    } else { // Outgoing Remote communication
                        if (!frontiers.count(to_pe)) {
                            frontiers.emplace(to_pe, std::set<int>());
                        }
                        try {
                            frontiers.at(to_pe).insert(from);
                        } catch (const std::out_of_range& except) {}
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
