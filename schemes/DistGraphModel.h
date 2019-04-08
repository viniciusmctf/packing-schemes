/** \file DistGraphModel.h
 *  Author: Vinicius M C T de Freitas
 *  Date Created: April 3rd, 2019
 *  E-mail: vinicius.mct.freitas@gmail.com
 *          or vinicius.mctf@posgrad.ufsc.br
 *
 */

#ifndef DGM_BASE_DEFS
#define DGM_BASE_DEFS

#include <unordered_map>


#include "def_DGM_charm.h"

namespace DGM {

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
using ResourceMap = ::std::unordered_map<DGM::CkResId, DGM::ResourceInfo>;


class Vertex {
public:
    Vertex(CkVrtId i) : _id(i), _weight(0) {}
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
    Edge(CkVrtId from_, CkVrtId to_) : from(Vertex(from_)), to(Vertex(to_)), 
	distance(1) {}
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
    ExpectedMigration(Vertex& vertex, CkResId resource) :
        migrating_vertex(vertex), target_resource(resource) {}

    ExpectedMigration() = delete; // Do not allow empty constructor

    bool operator<(const ExpectedMigration& rhs) {
        return migrating_vertex.id() < rhs.migrating_vertex.id();
    }

private:
    Vertex& migrating_vertex;
    CkResId target_resource;
};


}

#endif /* DGM_BASE_DEFS */
