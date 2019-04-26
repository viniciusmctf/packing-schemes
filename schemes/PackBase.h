/***
 * Author: Vinicius Freitas (vinicius.mct.freitas@gmail.com)
 * Produced at the Federal University of Santa Catarina, Brazil
 * (UFSC - BR - vinicius.mctf@posgrad.ufsc.br)
 * Base class for the implementation of Task Packing Schemes
 ***/
#pragma once
#include <vector>
#include <algorithm>
#include "NaivePack.h"

template <typename TaskType>
class BasePackingScheme {
public:
    BasePackingScheme() {}
    BasePackingScheme(std::vector<TaskType> wus, double pack_size, double pack_var, bool build = false);
    ~BasePackingScheme();
    void reserve(int tasks);
    void add_wu(TaskType task);
    virtual void build_packs(double target_load, double variance_in_percent);
    virtual std::vector<TaskType> extract_pack();

protected:
    std::vector< NaivePack<TaskType> > all_packs;
    std::vector<TaskType> all_wus;

private:
    bool built;
    int total_pack_count;
    double total_accumulated_load;
    double target_pack_load_low_threshold;
    double target_pack_load_high_threshold;
};

template <> BasePackingScheme<NaiveWorkUnit>::
    BasePackingScheme(std::vector<NaiveWorkUnit> wus, double pack_size,
        double pack_var, bool build) : all_wus(wus),
        total_pack_count(wus.size()) {
    if (build) {
        total_accumulated_load = std::accumulate(wus.begin(), wus.end(), 0.0);
        target_pack_load_low_threshold = (1-pack_var)*pack_size;
        target_pack_load_high_threshold = (1+pack_var)*pack_size;
        build_packs(pack_size, pack_var);
    }
};


// This is the naive packing scheme, sorting tasks from smaller to bigger
template <typename TaskType> void BasePackingScheme<TaskType>::
    build_packs(double target_load, double variance_in_percent) {
    if (built) {
        return;
    }
    all_packs.reserve(all_wus.size());
    std::vector<TaskType> pack = {};
    std::sort(all_wus.begin(), all_wus.end());
    double pack_load = 0.0;
    for (const auto& task : all_wus) {
        if (pack_load > target_pack_load_low_threshold) {
            all_packs.emplace_back(pack);
            pack.clear(); pack_load = 0.0;
        }
        pack.push_back(task);
        pack_load += task;
    }
    built = true;
}

template <typename TaskType> std::vector<TaskType>
    BasePackingScheme<TaskType>::extract_pack() {
    auto mig_pack = all_packs.back();
    all_packs.pop_back();
    total_accumulated_load -= mig_pack.load();
    return mig_pack.copy_and_destroy();
}
