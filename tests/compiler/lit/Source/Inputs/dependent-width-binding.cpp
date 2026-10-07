// Instance-identity oracle. Stimulus and expected values are independent of
// the DUT: the golden model below re-derives each counter from the raw driven
// epoch sequence, never from a DUT sample.
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "instance-identity oracle line " << at.line() << '\n'; std::abort(); }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
#ifdef CLOSED_WIDTH_IDENTITY
int main(int argc, char **argv) {
  require(argc == 2);
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
  gfsim::WorkExecutor pool(workers);
  pyc_root root("width-identity", &pool);
  root.clk = root.rst = known<1>(0); root.d = known<8>(0);
  root.Reset(); root.Xfer();
  const auto pattern = gfsim::FourState<8>::fromMasks(
      gfsim::Bits<8>{0xa5}, gfsim::Bits<8>{0xf0}, gfsim::Bits<8>{0x03});
  for (const auto value : {gfsim::FourState<8>::known(gfsim::Bits<8>{0x96}), pattern}) {
    root.d = decltype(root.d)::fromPacked(value);
    root.clk = known<1>(0); root.Work(); root.Xfer();
    root.clk = known<1>(1); root.Work(); root.Xfer();
    root.clk = known<1>(0); root.Work();
    const auto actual = root.q.packed();
    require(actual.invariantHolds());
    require(actual.knownMask() == value.knownMask());
    require(actual.zMask() == value.zMask());
    require((actual.value() & actual.knownMask()) == (value.value() & value.knownMask()));
    root.Xfer();
  }
  std::cout << "PASS width identity known/X/Z\n";
}
#else
struct Frame { unsigned clock, reset, en_a, en_b, da, db; };
// reset, only-A, only-B, both, both-hold, reset again. Each entry is one
// explicit Work/Xfer epoch; the observed value at Work() is the state
// committed by the preceding Xfer().
constexpr Frame frames[] = {
    {0, 1, 0, 0, 0, 0},   {1, 1, 0, 0, 0, 0},  {0, 0, 0, 0, 0, 0},
    {1, 0, 1, 0, 3, 0},   {0, 0, 1, 0, 0, 0},  {1, 0, 1, 0, 5, 0},
    {0, 0, 1, 0, 0, 0},   {1, 0, 1, 0, 1, 0},  {0, 0, 1, 0, 0, 0},
    {1, 0, 0, 1, 0, 7},   {0, 0, 0, 1, 0, 0},  {1, 0, 0, 1, 0, 9},
    {0, 0, 0, 1, 0, 0},   {1, 0, 1, 1, 11, 13}, {0, 0, 1, 1, 0, 0},
    {1, 0, 1, 1, 2, 4},   {0, 0, 1, 1, 0, 0},  {1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},   {1, 0, 0, 0, 99, 99}, {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 255, 255}, {0, 0, 0, 0, 0, 0},
};

int main(int argc, char **argv) {
  require(argc == 2);
  unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
  gfsim::WorkExecutor pool(workers);
  pyc_root root("instance-identity", &pool);
  root.en_a = root.en_b = known<1>(0);
  root.da = root.db = known<8>(0);
  root.pyc_7079635f636c6b = root.pyc_7079635f727374 = known<1>(0);
  root.Reset();
  root.Xfer();
  unsigned committed_a = 0, committed_b = 0, next_a = 0, next_b = 0;
  bool last_clock = false;
  unsigned epoch = 0;
  for (const Frame &frame : frames) {
    root.pyc_7079635f636c6b = known<1>(frame.clock);
    root.pyc_7079635f727374 = known<1>(frame.reset);
    root.en_a = known<1>(frame.en_a);
    root.en_b = known<1>(frame.en_b);
    root.da = known<8>(frame.da);
    root.db = known<8>(frame.db);
    root.Work();
    const auto packed = root.result.packed();
    require(packed.isFullyKnown());
    // First declared field occupies the most significant bits of the record.
    const auto actual_a = gfsim::extract<8>(packed, 8).value().value();
    const auto actual_b = gfsim::extract<8>(packed, 0).value().value();
    require(actual_a == committed_a && actual_b == committed_b);
    std::cout << "WORK " << epoch << ' ' << actual_a << ' ' << actual_b
              << " EN " << frame.en_a << frame.en_b << '\n';
    root.Xfer();
    next_a = committed_a;
    next_b = committed_b;
    if (frame.clock && !last_clock) {
      if (frame.reset) { next_a = 0; next_b = 0; }
      else {
        if (frame.en_a) next_a = (committed_a + frame.da) & 0xffu;
        if (frame.en_b) next_b = (committed_b + frame.db) & 0xffu;
      }
    }
    last_clock = frame.clock != 0;
    committed_a = next_a;
    committed_b = next_b;
    ++epoch;
  }
  std::cout << "PASS\n";
  return 0;
}

#endif
