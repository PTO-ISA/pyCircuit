#ifndef GENERATED_SOURCE
#error "GENERATED_SOURCE must name the generated QueueGraph C++ source"
#endif
#include GENERATED_SOURCE

#include <array>
#include <iostream>

int main() {
  ac_generated::LaneBundle model;
  auto rows = model.dispatch_rows();
  auto tick = [&](uint64_t number) {
    const gfsim::Epoch epoch{number, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };

  model.bundle().proposePush(gfsim::UInt<8>{10});
  model.bundle().proposePush(gfsim::UInt<8>{11});
  model.bundle().doXfer({0, 0});

  model.bundle().proposePush(gfsim::UInt<8>{20});
  model.bundle().proposePush(gfsim::UInt<8>{21});
  tick(1);
  model.bundle().proposePush(gfsim::UInt<8>{30});
  tick(2);
  tick(3);

  const auto &values = model.sink_0_values();
  const std::array<uint64_t, 5> expected = {10, 11, 20, 21, 30};
  if (values.size() != expected.size()) {
    std::cerr << "unexpected sink count " << values.size() << '\n';
    return 1;
  }
  for (size_t index = 0; index < values.size(); ++index)
    if (values[index] != gfsim::UInt<8>{expected[index]})
      return 2;

  model.bundle().proposePush(gfsim::UInt<8>{40});
  model.bundle().doXfer({4, 0});
  model.reset();
  tick(5);
  if (!model.sink_0_values().empty() || model.bundle().committedSize() != 0)
    return 3;
  std::cout << "PASS gfsim lane_bundle behavior\n";
  return 0;
}
