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

namespace DGM {

#include "../schemes/DistGraphModel.h"
#include "../../../charm/tmp/DistBaseLB.h"
#include "def_DGM_charm.h"

#include <set>
#include <vector>

struct LocalGraph {
    LocalGraph() = default;
    ~LocalGraph();
    void init(const DistBaseLB::stats* const st);

    const std::set<ExpectedMigration> confirm_migrations() const { return migrations; }
    std::set<ExpectedMigration> migrations;
    std::unordered_map<CkVrtHash, Vertex> vertices; //Indexed by hash
    std::map< std::set <Vertex&> > frontiers;
    std::vector<Edge> edges;
    bool built = false;
};

}


#endif /* CK_DIST_LDB_GRAPH_H */
/*@}*/
