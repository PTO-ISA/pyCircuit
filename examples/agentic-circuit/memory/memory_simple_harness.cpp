#include "memory_simple.generated.cpp"

#include <array>
#include <cstddef>
#include <iostream>

int main() {
  ac_generated::MemorySimple model;
  const std::array<ac_generated::MemoryRequest, 2> writes{{
      ac_generated::MemoryRequest{3, 42, 1},
      ac_generated::MemoryRequest{3, 99, 2},
  }};
  const ac_generated::MemoryRequest read{3, 0, 3};

  // Both logical endpoints are valid before the first simulated epoch. Fixed
  // priority drains both writer requests before accepting the reader request.
  for (const auto &request : writes)
    if (!model.writes().proposePush(request))
      return 1;
  if (!model.reads().proposePush(read))
    return 1;

  auto rows = model.dispatch_rows();
  std::size_t cycles = 0;
  for (std::size_t tick = 0; tick < 32; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    if (model.sink_0_values().size() == writes.size() &&
        model.sink_1_values().size() == 1) {
      cycles = tick + 1;
      break;
    }
  }

  const auto &write_responses = model.sink_0_values();
  const auto &read_responses = model.sink_1_values();
  if (write_responses.size() != 2 || write_responses[0].data != 0 ||
      write_responses[0].tag != 1 || write_responses[1].data != 42 ||
      write_responses[1].tag != 2 || read_responses.size() != 1 ||
      read_responses[0].data != 99 || read_responses[0].tag != 3)
    return 2;

  std::cout << "cycles=" << cycles
            << " write_old_values=" << write_responses[0].data << ","
            << write_responses[1].data
            << " read_after_priority=" << read_responses[0].data << "\n";
  return 0;
}
