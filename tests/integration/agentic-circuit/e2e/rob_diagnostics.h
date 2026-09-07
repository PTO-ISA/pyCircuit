#ifndef ROB_DIAGNOSTICS_H
#define ROB_DIAGNOSTICS_H

#include "gfsim/queue_blocks.h"
#include <iostream>
#include <span>

// Read committed state only, including both reusable placements by runtime
// name.
template <typename Event> void dumpRobEvent(const Event &value) {
  std::cerr << "{index=" << static_cast<unsigned long long>(value.index)
            << ",generation="
            << static_cast<unsigned long long>(value.generation)
            << ",epoch=" << static_cast<unsigned long long>(value.epoch)
            << ",value=" << static_cast<unsigned long long>(value.value)
            << ",done=" << static_cast<unsigned long long>(value.done) << "}";
}

template <unsigned Width> bool dumpRobScalar(gfsim::SimObject *object) {
  auto *table = dynamic_cast<gfsim::SimTable<gfsim::UInt<Width>> *>(object);
  if (!table)
    return false;
  std::cerr << object->path() << "="
            << static_cast<unsigned long long>(table->at(0)) << '\n';
  return true;
}

template <typename Event>
void dumpRob(std::span<const gfsim::DispatchRow> rows, gfsim::Tick tick) {
  std::cerr << "tick=" << tick << '\n';
  for (const auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (auto *queue = dynamic_cast<gfsim::SimQueue<Event> *>(object)) {
      std::cerr << object->path() << " queue=[";
      for (const auto &value : queue->committedValues())
        dumpRobEvent(value);
      std::cerr << "]\n";
    } else if (auto *table = dynamic_cast<gfsim::SimTable<Event> *>(object)) {
      std::cerr << object->path() << " entries=[";
      for (size_t index = 0; index < table->size(); ++index)
        dumpRobEvent(table->at(index));
      std::cerr << "]\n";
    } else {
      (void)(dumpRobScalar<2>(object) || dumpRobScalar<3>(object) ||
             dumpRobScalar<16>(object));
    }
  }
}

#endif
