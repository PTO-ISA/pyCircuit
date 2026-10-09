#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <source_location>

void require(bool passed, std::source_location at = std::source_location::current()) {
  if (!passed) {
    std::cerr << "historical feature oracle line " << at.line() << '\n';
    std::abort();
  }
}
template<unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
using Byte = gfsim::wire<gfsim::Bits<8>>;
auto initial() {
  pyc_dut::Inputs input;
  input.pyc_7079635f636c6b = known<1>(0);
  input.pyc_7079635f727374 = known<1>(1);
#if FEATURE_KIND == 0
  input.en = known<1>(0);
#elif FEATURE_KIND == 1
  input.in_x = known<8>(0);
#else
  input.in_a = known<8>(0);
#endif
  return input;
}
auto outputs(pyc_dut &dut) {
  const auto out = dut.sample();
#if FEATURE_KIND == 1
  return std::array<Byte, 2>{out.y0, out.y1};
#else
  return std::array<Byte, 2>{out.y, out.y};
#endif
}
void drive_data(pyc_dut::Inputs &input, Byte data) {
#if FEATURE_KIND == 0
  require(data.isFullyKnown());
  input.en = known<1>(data.value().value());
#elif FEATURE_KIND == 1
  input.in_x = data;
#else
  input.in_a = data;
#endif
}
void equal(Byte actual, Byte wanted) {
  require(actual.packed().invariantHolds());
  require(actual.packed().value() == wanted.packed().value());
  require(actual.packed().knownMask() == wanted.packed().knownMask());
  require(actual.packed().zMask() == wanted.packed().zMask());
}
int main(int argc, char **argv) {
  require(argc == 3);
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
  pyc_dut dut(workers);
  auto input = initial();
  dut.drive(input);
  dut.system().Build();
  dut.system().Reset();
  auto step = [&](unsigned clock) {
    input.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(input);
    require(dut.system().Step() == gfsim::SimStepResult::Running);
    return outputs(dut);
  };
  // Retain the original driver's two asserted-reset clock cycles.
  for (unsigned cycle = 0; cycle < 2; ++cycle) {
    step(0); step(1);
    for (auto value : step(0)) equal(value, known<8>(0));
  }
  input.pyc_7079635f727374 = known<1>(0);
  std::ifstream stream(argv[2]); require(stream.good());
  unsigned stimulus, pre0, post0, pre1, post1, cycles = 0;
  while (stream >> stimulus >> pre0 >> post0 >> pre1 >> post1) {
    drive_data(input, known<8>(stimulus));
    auto pre = step(0);
    equal(pre[0], known<8>(pre0)); equal(pre[1], known<8>(pre1));
    auto edge = step(1);
    equal(edge[0], known<8>(pre0)); equal(edge[1], known<8>(pre1));
    auto post = step(0);
    equal(post[0], known<8>(post0)); equal(post[1], known<8>(post1));
    std::cout << "CYCLE " << cycles++ << ' ' << pre0 << ' ' << post0
              << ' ' << pre1 << ' ' << post1 << '\n';
  }
  require(stream.eof() && cycles == FEATURE_CYCLES);
#if FEATURE_KIND == 2
  // Additional host four-state I/O evidence; the historical Python driver
  // itself supplied only known values and no source-level X/Z constructor.
  const std::array<Byte, 3> patterns{
      Byte::unknown(),
      Byte::fromPacked(gfsim::FourState<8>::fromMasks(
          gfsim::Bits<8>{0}, gfsim::Bits<8>{0}, gfsim::Bits<8>{255})),
      Byte::fromPacked(gfsim::FourState<8>::fromMasks(
          gfsim::Bits<8>{66}, gfsim::Bits<8>{90}, gfsim::Bits<8>{128}))};
  for (const auto &pattern : patterns) {
    drive_data(input, pattern); step(0); step(1);
    for (auto actual : step(0)) equal(actual, pattern);
  }
  std::cout << "HOST X Z mixed-mask capture\n";
#endif
  // Reset must win over the next write/increment and restore both children.
  drive_data(input, known<8>(1));
  input.pyc_7079635f727374 = known<1>(1);
  step(0); step(1);
  for (auto value : step(0)) equal(value, known<8>(0));
  std::cout << "PASS\n";
}
