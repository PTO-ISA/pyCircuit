#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>

void require(bool condition) { if (!condition) std::abort(); }
auto bit(unsigned value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}
unsigned value(const gfsim::wire<gfsim::Bits<3>> &wire) {
  require(wire.isFullyKnown());
  for (unsigned i = 0; i < 8; ++i)
    if (wire.value() == gfsim::Bits<3>{i}) return i;
  std::abort();
}

template <class DUT> bool validate(DUT &dut) {
  gfsim::SimFailureInfo failure;
  if constexpr (requires { dut.pyc_validate_checks(failure); })
    return dut.pyc_validate_checks(failure);
  return true;
}

// Work/check/Xfer is the existing backend protocol. Each cycle below supplies
// one rising edge; failed preparation is discarded before any owner commits.
int main() {
  gfsim::WorkExecutor executor(2);
  pyc_root dut("transactional", &executor);
  dut.Build();
  dut.pyc_7079635f636c6b = bit(0);
  dut.pyc_7079635f727374 = bit(0);
  dut.Reset(); dut.Xfer();
  auto sample = [&](unsigned clk, unsigned rst, bool accepted = true) {
    dut.pyc_7079635f636c6b = bit(clk);
    dut.pyc_7079635f727374 = bit(rst);
    dut.Work();
    require(validate(dut) == accepted);
    if (accepted) dut.Xfer(); else dut.DiscardNext();
    dut.Work(); dut.DiscardNext();
  };
  auto cycle = [&](unsigned rst = 0, bool accepted = true) {
    sample(0, rst);
    sample(1, rst, accepted);
  };
#ifdef FAILURE
  auto check = [&](unsigned a, unsigned b) {
    require(value(dut.a) == a && value(dut.b) == b);
    std::cout << a << ' ' << b << '\n';
  };
  sample(0, 0); check(2, 5);
  cycle(); check(3, 6);
  cycle(0, false); check(3, 6);
  // Retry the same rising sample: the failed clock/check history cannot advance.
  sample(1, 0, false); check(3, 6);
  // Same rising sample with reset must override the failing assertion.
  sample(1, 1); check(2, 5);
  cycle(); check(3, 6);
#else
  auto check = [&](unsigned a, unsigned b, unsigned counter, unsigned shared,
                   bool copied) {
    require(value(dut.a) == a && value(dut.b) == b);
    require(value(dut.alias_a) == a && value(dut.counter) == counter);
    require(value(dut.shared) == shared);
    const auto expected = gfsim::FourState<4>::fromMasks(
        gfsim::Bits<4>{8}, gfsim::Bits<4>{12}, gfsim::Bits<4>{1});
    require(dut.payload.packed().value() == expected.value());
    require(dut.payload.packed().knownMask() == expected.knownMask());
    require(dut.payload.packed().zMask() == expected.zMask());
    if (copied) {
      require(dut.copy.packed().value() == expected.value());
      require(dut.copy.packed().knownMask() == expected.knownMask());
      require(dut.copy.packed().zMask() == expected.zMask());
    } else require(dut.copy.isFullyKnown() && dut.copy.value() == gfsim::Bits<4>{0});
    std::cout << a << ' ' << b << ' ' << counter << ' ' << shared << ' '
              << copied << '\n';
  };
  sample(0, 0); check(6, 3, 5, 3, false);
  cycle(); check(7, 6, 6, 1, true);
  cycle(); check(7, 6, 7, 2, true);
  cycle(); check(0, 7, 0, 1, true);
  cycle(); check(0, 7, 1, 2, true);
  cycle(1); check(6, 3, 5, 3, false);
  cycle(); check(7, 6, 6, 1, true);
#endif
}
