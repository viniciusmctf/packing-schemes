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
    NaivePack() {
      tasks = {};
      total_load = 0.0;
    }
    NaivePack(std::initializer_list<NaiveWorkUnit> tasks);
    ~NaivePack() = default;
    void add(NaiveWorkUnit task);

private:
    std::vector<NaiveWorkUnit> tasks;
    double total_load;
    // template <class T> friend class BasePackingScheme; // T should be NaiveWorkUnit. Fix later

public:
    std::vector<int> const get_ids();
    int ntasks() { return tasks.size(); }
    double load() const { return total_load; }
    std::vector<NaiveWorkUnit> copy_and_destroy(); // Excludes need of friend class
    bool operator>(const double);
    bool operator<(const double);
    bool operator>(const NaivePack rhs);
    bool operator<(const NaivePack rhs);
  };

std::vector<NaiveWorkUnit> NaivePack::copy_and_destroy() {
    std::vector<NaiveWorkUnit> retval = {};
    retval.reserve(tasks.size());
    std::copy(tasks.begin(), tasks.end(), std::back_inserter(retval));
    tasks.clear();
    return retval;
}

struct NaivePack_Greater {
  inline bool operator() (const NaivePack a, const NaivePack b) {
      return (a.load() > b.load());
    }
  };

bool NaivePack::operator>(const double rhs){
    return load() > rhs;
}

bool NaivePack::operator<(const double rhs){
    return load() < rhs;
}

bool operator>(const NaivePack lhs, const NaivePack rhs){
    return lhs.load() > rhs.load();
}

bool operator<(const NaivePack lhs, const NaivePack rhs){
    return lhs.load() < rhs.load();
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
    tasks.push_back(t);
    total_load = total_load + t;
}
