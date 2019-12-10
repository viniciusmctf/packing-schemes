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
    double operator+(double rhs) {
      return load + rhs;
    }
protected:
    int id;
    double load;
};
