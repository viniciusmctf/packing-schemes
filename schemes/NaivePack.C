#include "NaivePack.h"

bool NaiveWorkUnit::operator>(const NaiveWorkUnit& rhs) {
  return load > rhs.load;
}

bool NaiveWorkUnit::operator<(const NaiveWorkUnit& rhs) {
  return load < rhs.load;
}

bool NaiveWorkUnit::operator==(const NaiveWorkUnit& rhs) {
  return id == rhs.id;
}

bool NaiveWorkUnit::operator!=(const NaiveWorkUnit& rhs) {
  return id != rhs.id;
}

double NaiveWorkUnit::operator+(const NaiveWorkUnit& rhs) {
  return get_load() + rhs.get_load();
}

std::vector<NaiveWorkUnit> NaivePack::copy_and_destroy() {
    std::vector<NaiveWorkUnit> retval = {};
    retval.reserve(tasks.size());
    std::copy(tasks.begin(), tasks.end(), std::back_inserter(retval));
    tasks.clear();
    return retval;
}

bool NaivePack::operator>(const double& rhs){
    return load() > rhs;
}

bool NaivePack::operator<(const double& rhs){
    return load() < rhs;
}

NaivePack::NaivePack(std::initializer_list<NaiveWorkUnit> tasks) : tasks(tasks) {
    total_load = std::accumulate(tasks.begin(), tasks.end(), 0.0);
}

std::vector<int> const NaivePack::get_ids() {
    std::vector<int> ids = {};
    for (const auto& t : tasks) {
        ids.push_back(t.get_id());
    }
    return ids;
}

void NaivePack::add(NaiveWorkUnit t) {
    if (!tasks.count(t)) {
        tasks.insert(t);
        total_load += t.get_load();
    }
}
