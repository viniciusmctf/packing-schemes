#include <algorithm>
#include <cstdlib>
#include "UpdateProcMap.h"

int ProcMap::insert_update(MapNode node) {
    bool found = false;
    auto it = map.begin();
    int i = 0;
    while (!found && it != map.end()) {
        found = node._index == map[i]._index;
        ++it;
        ++i;
    }
    if (found) {
        auto replace = most_recent(node, map[--i]);
        map[i] = replace;
        if (node._timestamp > replace._timestamp) { //The value was not replaced
            return 0;
        } else { // The value was replaced
            std::make_heap(map.begin(), map.end(), [](MapNode lhs, MapNode rhs) {
                return lhs._data < rhs._data;
            } ) ; // Use greater in order to create a min_heap)
            return 1;
        }
    } else {
        // Create new node;
        map.push_back(node);
        std::push_heap(map.begin(), map.end(), [](MapNode lhs, MapNode rhs) {
            return lhs._data < rhs._data;
        } ) ; // Use greater in order to create a min_heap
        return 2;
    }
    return 0;
}

int ProcMap::emplace_update(int id, double load, double time_) {
    return insert_update(MapNode{id, load, time_});
}

const MapNode& ProcMap::most_recent(const MapNode& a, const MapNode& b) {
    return (a._timestamp > b._timestamp ? a : b);
}

int ProcMap::size() {
    return map.size();
}

std::pair<int,double> ProcMap::first() const {
    if (map.size() < 1) {
        return std::move(std::make_pair(-1,-1.0));
    }

    std::vector<MapNode> map_cpy = map; // Create a local cpy
    std::sort_heap(map_cpy.begin(), map_cpy.end(), [](MapNode lhs, MapNode rhs) {
        return lhs._data < rhs._data;
    });

    auto node = map_cpy.front();
    return std::make_pair(node._index, node._data);
}

std::pair<int, double> ProcMap::last() const {
    if (map.size() < 1) {
        return std::make_pair(-1,-1.0);
    }

    std::vector<MapNode> map_cpy = map; // Create a local cpy
    std::sort_heap(map_cpy.begin(), map_cpy.end(), [](MapNode lhs, MapNode rhs) {
        return lhs._data < rhs._data;
    });

    auto node = map_cpy.back();
    return std::make_pair(node._index, node._data);
}

void ProcMap::vectorize_data(int *indexes, double *loads, double *timestamps) {
    std::vector<MapNode> map_cpy = map; // Create a local cpy
    std::sort_heap(map_cpy.begin(), map_cpy.end(), [](MapNode lhs, MapNode rhs) {
        return lhs._data < rhs._data;
    });
    int i = 0;
    for (auto node : map_cpy) {
        indexes[i] = node._index;
        loads[i] = node._data;
        timestamps[i] = node._timestamp;
        i++;
    }
}

int ProcMap::get_random_id() {
    if (map.size() < 2) {
        return (pe_id+1)%num_pes;
    } else {
      if (map.size() < (float)CkNumPes()/4.0f) {
        return first().first;
      }
    }

    std::vector<MapNode> map_cpy = map; // Create a local cpy
    std::vector<int> possible_indexes;
    int total_out = (map.size() < 4) ? map.size() : map.size()/10+4;

    std::sort_heap(map_cpy.begin(), map_cpy.end(), [](MapNode lhs, MapNode rhs) {
        return lhs._data < rhs._data;
    });

    for (int i = 0; i < total_out; i++) {
        const MapNode& smallest = map_cpy.at(i);
        possible_indexes.push_back(smallest._index);
    }

    std::srand(pe_id*num_pes/32);
    int victim = 0;
    do {
        victim = std::rand()%possible_indexes.size();
        victim = possible_indexes[victim];
    } while (victim == pe_id);

    return victim;
}
