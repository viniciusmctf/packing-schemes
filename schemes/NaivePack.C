#include "NaivePack.h"
#include <cstdio>

std::vector<NaiveWorkUnit> NaivePack::copy_and_destroy() {
    std::vector<NaiveWorkUnit> retval = {};
    retval.reserve(tasks.size());
    std::copy(tasks.begin(), tasks.end(), std::back_inserter(retval));
    tasks.clear();
    return retval;
}

bool NaivePack::operator>(double rhs) const {
    return load() > rhs;
}

bool NaivePack::operator<(double rhs) const {
    return load() < rhs;
}

bool NaivePack::operator>(NaivePack rhs) const {
  return load() > rhs.load();
}

bool NaivePack::operator<(NaivePack rhs) const {
  return load() < rhs.load();
}

NaivePack::NaivePack(std::initializer_list<NaiveWorkUnit> tasks) : tasks(tasks) {
    total_load = 0.0;
    for (auto task : tasks) {
      total_load += task.get_load();
    }
}

std::vector<int> const NaivePack::get_ids() {
    std::vector<int> ids = {};
    for (const auto& t : tasks) {
        ids.push_back(t.get_id());
    }
    return ids;
}

void NaivePack::add(NaiveWorkUnit t) {
  printf("I say goodbye\n");
  tasks.push_back(t);
  printf("You say\n");
  total_load += t.get_load();
  printf("Hello\n");
  return;
}
