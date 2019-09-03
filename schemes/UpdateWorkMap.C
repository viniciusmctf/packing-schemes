#include "UpdateWorkMap.h"

double WorkGroup::sum_work() {
  return 0.0;
}

/**
 *
 * End of WorkGroup definitions.
 * Start of WorkMap definitions.
 **/


WorkMap::WorkMap(int n_objs, int* objs, double* loads): my_total_load(0.0) {
  recent = std::set<WorkObj>();
  create_obj_naive_map(n_objs, objs, loads);
}

WorkMap::WorkMap(int n_objs, double* loads): my_total_load(0.0) {
  recent = std::set<WorkObj>();
  create_obj_naive_map(n_objs, loads);
}

WorkMap::WorkMap(const std::vector<std::pair<int,double> >& map) : my_total_load(0.0) {
  recent = std::set<WorkObj>();
  create_obj_naive_map(map);
}

void WorkMap::insert(WorkObj obj) {
  all_objs[obj.sys_index] = obj;
  ordered_objs.insert(obj);
  my_total_load += obj.load;
  return; // Please override this in affinity-oriented algorithms
  // if (obj.neighbor >= 0) {
  //   // Frontier handling code here.
  // }
}

// This should go to recent
void WorkMap::insert_new(WorkObj obj) {
  my_total_load += obj.load;
  recent.insert(obj);
}

void WorkMap::emplace(int index, double value) {
  WorkObj node {index, value};
  insert(node);
}

void WorkMap::emplace_new(int index, double value) {
  WorkObj node {index, value};
  insert_new(node);
}

bool WorkMap::exists(int index) {
    return all_objs.count(index);
}

// Throws exception if not found
const WorkObj& WorkMap::find(int index) {
    try {
        auto& obj = all_objs.at(index);
        return obj;
    } catch (const std::out_of_range& oor) {
        for (auto& entry : recent) {
            if (entry.sys_index == index) {
                return entry;
            }
        }
    }
}

std::vector<WorkObj> WorkMap::remove_batch(std::vector<int> indexes) {
    std::vector<WorkObj> retval = {};
    int i = 0;
    while (indexes.size() > 0) {
        i = indexes.back();
        if (!all_objs.count(i)) {
            indexes.pop_back();
            continue;
        }
        auto obj = remove(i);
        retval.push_back(obj);
        indexes.pop_back();
    }
    return retval;
}

std::vector<WorkObj> WorkMap::remove_batch_of_load(double& expected_load) {
    double l_loadt = expected_load*(0.96); //lower_load_threshold
    double h_loadt = expected_load*(1.08); //higher_load_threshold
    double current_batch_load = 0.0;

    std::vector<WorkObj> work_batch = {};
    while (current_batch_load < l_loadt && ordered_objs.size() > 0) {
        auto& task = (WorkObj&) *ordered_objs.begin();
        current_batch_load += task.load;

        if (current_batch_load > h_loadt) {
            current_batch_load -= task.load;
            break;
        }
        work_batch.push_back(remove(task.sys_index));
    }

    expected_load = current_batch_load;
    return work_batch;
}

double WorkMap::calculate_total_load() {
  if (my_total_load < 0.001) {
    double tmp = 0.0;
    if (all_objs.size() > 0) {
      for (auto entry : all_objs) {
        tmp += entry.second.load;
      }
    }
    if (recent.size() > 0) {
      for (auto entry : recent) {
        tmp += entry.load;
      }
    }
    my_total_load = tmp;
  }
  return my_total_load;
}

// Throws exception if obj doesnt exist (from all_objs.at())
WorkObj WorkMap::remove(int index) {
  auto obj = all_objs.at(index);
  ordered_objs.erase(obj);
  all_objs.erase(index);
  my_total_load -= obj.load;
  return obj;
}

 // false if not removed, true otherwise
 void WorkMap::create_obj_naive_map(int n_objs, int* objs, double* loads) {
     for (int i = 0; i < n_objs; i++) {
         auto obj = WorkObj{objs[i], loads[i]};
         insert(obj);
     }
 }

void WorkMap::create_obj_naive_map(int n_objs, double* loads) {
  for (int i = 0; i < n_objs; i++) {
    auto obj = WorkObj{i, loads[i]};
    insert(obj);
  }
}

void WorkMap::create_obj_naive_map(const std::vector<std::pair<int,double>>& map) {
  for (const auto& element : map) {
    emplace(element.first, element.second);
  }
}

void WorkMap::remove_from_all(const WorkObj& obj) {
    return;
} // Should be overrided by specialized classes

void WorkMap::clear() {
  all_objs.clear();
  ordered_objs.clear();
  recent.clear();
  my_total_load = 0.0;
}
