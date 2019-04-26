/***
 * Author: Vinicius Freitas (vinicius.mct.freitas@gmail.com)
 * Produced at the Federal University of Santa Catarina, Brazil
 * (UFSC - BR - vinicius.mctf@posgrad.ufsc.br)
 * Base class for the implementation of Task Packing Schemes
 ***/
#pragma once
#include <vector>
#include <set>
#include <numeric>
#include <initializer_list>

class NaiveWorkUnit {
public:
    NaiveWorkUnit() : id(-1), load(0.0) {}
    NaiveWorkUnit(int id, double load) : id(id), load(load) {}
    ~NaiveWorkUnit() = default;
    double get_load() const { return load; }
    int get_id() const { return id; }
    bool operator>(const NaiveWorkUnit& rhs) {
        return load > rhs.load;
    } // Used to sort, compares load
    bool operator<(const NaiveWorkUnit& rhs) {
        return load < rhs.load;
    } // Used to sort, compares load
    bool operator==(const NaiveWorkUnit& rhs) {
        return id == rhs.id;
    } // Used to check, compares id
    bool operator!=(const NaiveWorkUnit& rhs) {
        return id != rhs.id;
    } // Used to check, compares id
    double operator+(const NaiveWorkUnit& rhs) {
        return load + rhs.load;
    }
protected:
    int id;
    double load;
};

double operator+(const double& lhs, const NaiveWorkUnit& rhs) {
    return lhs + rhs.get_load();
};

bool operator<(const NaiveWorkUnit lhs, const NaiveWorkUnit rhs) {
    return lhs.get_load() < rhs.get_load();
}

bool operator>(const NaiveWorkUnit lhs, const NaiveWorkUnit rhs) {
    return lhs.get_load() > rhs.get_load();
}

template<typename TaskType>
class NaivePack {
public:
    NaivePack() = default;
    NaivePack(std::initializer_list<TaskType> tasks);
    ~NaivePack() = default;
    void add(TaskType task);

private:
    std::set<TaskType> tasks;
    double total_load;
    // template <class T> friend class BasePackingScheme; // T should be TaskType. Fix later

public:
    std::vector<int> const get_ids();
    int ntasks() { return tasks.size(); }
    double load() const { return total_load; }
    std::vector<TaskType> copy_and_destroy(); // Excludes need of friend class
    bool operator>(const double&);
    bool operator<(const double&);
    bool operator>(const NaivePack<TaskType>& rhs) {return load() > rhs.load();}
    bool operator<(const NaivePack<TaskType>& rhs) {return load() < rhs.load();}
};

template <typename TaskType> std::vector<TaskType> NaivePack<TaskType>::
    copy_and_destroy() {
    std::vector<TaskType> retval = {};
    retval.reserve(tasks.size());
    std::copy(tasks.begin(), tasks.end(), std::back_inserter(retval));
    tasks.clear();
    return retval;
}

template <> bool NaivePack<NaiveWorkUnit>::
    operator>(const double& rhs){
    return load() > rhs;
}

template <> bool NaivePack<NaiveWorkUnit>::
    operator<(const double& rhs){
    return load() < rhs;
}

template <typename TaskType> NaivePack<TaskType>::
    NaivePack(std::initializer_list<TaskType> tasks) : tasks(tasks) {
    total_load = std::accumulate(tasks.begin(), tasks.end(), 0.0);
}

template <> std::vector<int> const NaivePack<NaiveWorkUnit>::get_ids() {
    std::vector<int> ids = {};
    for (const auto& t : tasks) {
        ids.push_back(t.get_id());
    }
    return ids;
}

template <> void NaivePack<NaiveWorkUnit>::add(NaiveWorkUnit t) {
    if (!tasks.count(t)) {
        tasks.insert(t);
        total_load += t.get_load();
    }
}
