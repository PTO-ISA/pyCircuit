#ifndef GENERATED_SOURCE
#error "GENERATED_SOURCE must name the generated QueueGraph C++ source"
#endif
#include GENERATED_SOURCE

#include <array>
#include <iostream>

int main() {
  ac_generated::LaneTransform model;
  auto rows = model.dispatch_rows();
  auto tick = [&](uint64_t number) {
    const gfsim::Epoch epoch{number, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  model.input().proposePush(gfsim::UInt<8>{10});
  model.input().proposePush(gfsim::UInt<8>{11});
  model.input().doXfer({0, 0});
  tick(1);
  tick(2);
  const auto &values = model.sink_0_values();
  if (values.size() != 2 || values[0] != gfsim::UInt<8>{11} ||
      values[1] != gfsim::UInt<8>{12})
    return 1;
  model.reset();
  std::cout << "PASS gfsim lane_transform behavior\n";
  return 0;
}
