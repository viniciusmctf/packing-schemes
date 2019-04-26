/***
 *
 * Interface for an updatable Work Information.
 *
 *
 **/


#ifndef UPDATABLE_WORK_MAP_H
#define UPDATABLE_WORK_MAP_H

#include <vector>
#include <unordered_map>
#include <set>
#include <list>

struct WorkObj {
    int sys_index;
    double load;
};

inline bool operator<(const WorkObj& lhs, const WorkObj& rhs) {
    return lhs.load < rhs.load;
}

inline bool operator>(const WorkObj& lhs, const WorkObj& rhs) {
    return lhs.load > rhs.load;
}

inline bool operator==(const WorkObj& lhs, const WorkObj& rhs) {
    return lhs.sys_index == rhs.sys_index;
}

inline double operator+(const WorkObj& lhs, const WorkObj& rhs) {
    return lhs.load + rhs.load;
}

inline double operator+(double lhs, const WorkObj& rhs) {
    return lhs + rhs.load;
}

class WorkGroup {
 public:
    double sum_work();

 protected:
    std::list<const WorkObj*> my_work; // Use iterators for easy
    double summed_work; // -1 before first call of sum_work. Should be redefined to -1 everytime some load is removed.
};

class WorkMap {
 public:
    WorkMap() = default;
    WorkMap(int n_objs, int* objs, double* loads);
    WorkMap(int n_objs, double* loads);
    WorkMap(const std::vector<std::pair<int,double> >&);
    void emplace(int, double);
    void emplace_new(int, double);
    void insert(WorkObj);
    void insert_new(WorkObj);
    bool exists(int index);
    int size() { return ordered_objs.size() + recent.size(); }
    const WorkObj& find(int index); // Throws exception if not found
    std::vector<WorkObj> remove_batch(std::vector<int> indexes);
    std::vector<WorkObj> remove_batch_of_load(double& expected_load);
    double calculate_total_load();

 protected:
    std::unordered_map<int, WorkObj> all_objs;
    std::vector<WorkGroup> frontiers;
    std::set<WorkObj> ordered_objs;
    std::set<WorkObj> recent;
    double my_total_load; // should be updated on remove
    WorkObj remove(int index); // false if not removed, true otherwise

 private:
    void create_obj_naive_map(int n_objs, double* loads);
    void create_obj_naive_map(int n_objs, int* objs, double* loads);
    void create_obj_naive_map(const std::vector<std::pair<int,double>>&);
    void remove_from_all(const WorkObj& obj); // Should be overrided by specialized classes

};

#endif // UPDATABLE_WORK_MAP_H
