#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>

// The adapter owns clock/reset and can observe state after rejected Work.
int main(int argc, char **argv) {
  gfsim::WorkExecutor executor(argc > 1 ? std::atoi(argv[1]) : 1);
  pyc_root dut("methods", &executor);
  auto bit = [](unsigned value) {
    return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
  };
  dut.Build();
  dut.pyc_7079635f636c6b = bit(0);
  dut.pyc_7079635f727374 = bit(0);
  dut.Reset();
  dut.Xfer();
  auto sample = [&](unsigned clock, bool accepted = true) {
    dut.pyc_7079635f636c6b = bit(clock);
    dut.Work();
    gfsim::SimFailureInfo failure;
    if (dut.pyc_validate_checks(failure) != accepted)
      std::abort();
    if (accepted)
      dut.Xfer();
    else
      dut.DiscardNext();
  };
  auto check = [&](unsigned expected) {
    if (!dut.out0.isFullyKnown() || !dut.out1.isFullyKnown() ||
        dut.out0.value() != gfsim::Bits<8>{expected} ||
        dut.out1.value() != gfsim::Bits<8>{expected})
      std::abort();
  };
  for (unsigned expected = 0; expected < 3; ++expected) {
    sample(0);
    check(expected);
    std::cout << expected << ' ' << expected << '\n';
    sample(1, expected < 2);
  }
  // Both the failing Method and independent Rule proposed 2 -> 3.
  sample(0);
  check(2);
}
