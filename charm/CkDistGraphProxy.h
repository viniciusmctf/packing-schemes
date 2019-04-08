/** \file CkDistGraphProxy.h
 *  Author: Vinicius M C T de Freitas
 *  Date Created: April 2nd, 2019
 *  E-mail: vinicius.mct.freitas@gmail.com
 *          or vinicius.mctf@posgrad.ufsc.br
 *
 *  Note: This class is a Proxy for DistGraphModel, proposed in 2019 by Freitas et. al.
 *  it adapts the information provided by the Charm++ RTS to DGM and vice-versa.
 */

/**
 *  \addtogroup CkLdb
 */

/*@{*/

#ifndef CK_DIST_LDB_GRAPH_H
#define CK_DIST_LDB_GRAPH_H

#include "CkGraphHash.h"
#include "DistGraphModel.h"

#include <set>
#include <string>
#include <vector>


namespace DGM {

class LocalGraph {
public:
    LocalGraph();
    LocalGraph(int id, const DistBaseLB::LDStats* const st) : hash_map(st), my_id(id) {}
    ~LocalGraph();
    virtual void init();

    const std::set<ExpectedMigration>& confirm_migrations() const { return migrations; }

    std::set<ExpectedMigration> migrations;
    std::map< CkResId, std::set <CkVrtHash> > frontiers;
    std::unordered_map<CkVrtHash, Vertex> vertices; //Indexed by hash
    std::unordered_map<CkVrtId, std::set<Edge> > edges;
    ckutil::HashMap hash_map;
    bool built = false;
    double total_load = 0.0;
    int my_id;

    std::string to_string() const;
};

}


#endif /* CK_DIST_LDB_GRAPH_H */
/*@}*/
