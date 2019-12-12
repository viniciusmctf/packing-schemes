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

class NaivePack {
public:
    NaivePack() = default;
    NaivePack(std::initializer_list<NaiveWorkUnit> tasks);
    ~NaivePack() = default;
    void add(TaskType task);

private:
    std::set<NaiveWorkUnit> tasks;
    double total_load;
    // template <class T> friend class BasePackingScheme; // T should be TaskType. Fix later

public:
    std::vector<int> const get_ids();
    int ntasks() { return tasks.size(); }
    double load() const { return total_load; }
    std::vector<NaiveWorkUnit> copy_and_destroy(); // Excludes need of friend class
    bool operator>(double);
    bool operator<(double);
    bool operator>(NaivePack& rhs) {return load() > rhs.load();}
    bool operator<(NaivePack& rhs) {return load() < rhs.load();}
};
