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
