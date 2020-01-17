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
    NaiveWorkUnit(const NaiveWorkUnit&) = default;
    double get_load() const { return load; }
    int get_id() const { return id; }
    bool operator==(const NaiveWorkUnit& rhs) const {
        return id == rhs.id;
    } // Used to check, compares id
    bool operator!=(const NaiveWorkUnit& rhs) const {
        return id != rhs.id;
    } // Used to check, compares id
    double operator+(const NaiveWorkUnit& rhs) const {
        return load + rhs.load;
    }
    double operator+(double rhs) const {
        return load + rhs;
    }
    bool operator<(NaiveWorkUnit rhs) const {
        return get_load() < rhs.get_load();
    }

    bool operator>(NaiveWorkUnit rhs) const {
        return get_load() > rhs.get_load();
    }
protected:
    int id;
    double load;
};


class NaivePack {
public:
    NaivePack() = default;
    NaivePack(std::initializer_list<NaiveWorkUnit> tasks);
    void add(NaiveWorkUnit task);

private:
    std::vector<NaiveWorkUnit> tasks {};
    double total_load = 0.0;

public:
    std::vector<int> const get_ids();
    int ntasks() { return tasks.size(); }
    double load() const { return total_load; }
    std::vector<NaiveWorkUnit> copy_and_destroy(); // Excludes need of friend class
    bool operator>(double) const;
    bool operator<(double) const;
    bool operator>(NaivePack rhs) const;
    bool operator<(NaivePack rhs) const;
};
