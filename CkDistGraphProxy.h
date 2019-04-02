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

using CkResId = int; // Resource Id
using CkResWg = double; // Resource summed weight
using CkVrtId = int; // Vertex Id
using CkVrtWg = double; // Vertex Weight
using CkResDis = int; // Distance between 2 resources
using CkVrtDis = int; // Distance between 2 vertices

// #include "DistGraphModel.h"
// #include "BaseLB.h"
#include <unordered_map>
#include <set>

class ResourceInfo {
public:
    ResourceInfo(CkResId id, CkResWg wg) : _id(id), _load(wg) {}
    ResourceInfo() = delete;
    CkResWg load() const { return _load; }
    bool operator<(const ResourceInfo& rhs) { return load() < rhs.load(); }

protected:
    CkResId _id;
    CkResWg _load;
};
using ResourceMap = std::unordered_map<CkResId, ResourceInfo>;


class Vertex {
public:
    Vertex(CkVrtId i, CkVrtWg l) : _id(i), _weight(l) {}
    Vertex() = delete;
    CkVrtId id() const { return _id; }
    CkVrtWg weight() const { return _weight; }

    bool operator<(const Vertex& rhs) { return weight() < rhs.weight(); }
    bool operator>(const Vertex& rhs) { return weight() > rhs.weight(); }

protected:
    CkVrtId _id;
    CkVrtWg _weight;
};

class RemoteVertex : public Vertex {
public:
    RemoteVertex(CkVrtId _id, CkResId _src) : Vertex(_id, 0.0), _src(_src) {}
    CkResId resource() { return _src; }

private:
    CkResId _src;
};

class Edge {
public:
    Edge(const Vertex& from, const Vertex& to, CkVrtDis dist = 1) : from(from),
        to(to), distance(dist) {}

    CkVrtDis dist() const { return distance; }

    // Not ideal to implement in the header, but operator
    bool operator<(const Edge& rhs) const {
        bool res = false;
        if (dist() < rhs.dist()) {
            res = true;
        } else if (from.id() < rhs.from.id()) {
            res = true;
        } else if (to.id() < rhs.to.id()) {
            res = true;
        }
        return res;
    }

    void update_distance(CkVrtDis new_dis) { distance = new_dis; }
protected:
    const Vertex& from;
    const Vertex& to;
    CkVrtDis distance; // Consider this as 1 for vertices in the same resource

};

// Overload < operator to use sets of expected migrations.
// From: https://en.cppreference.com/w/cpp/container/set
// We assume the Compare requirement is fulfilled overloading <
// This way, we guarantee each Vertex will be migrated only once.
class ExpectedMigration {
public:
    ExpectedMigration(CkVrtId vertex, CkResId resource) :
        migrating_vertex(vertex), target_resource(resource) {}

    ExpectedMigration() = delete; // Do not allow empty constructor

    bool operator<(const ExpectedMigration& rhs) {
        return migrating_vertex < rhs.migrating_vertex;
    }

private:
    CkVrtId migrating_vertex;
    CkResId target_resource;
};

class LocalGraph {
protected:
    std::set<ExpectedMigration> migrations;


};



}


#endif /* CK_DIST_LDB_GRAPH_H */
/*@}*/
