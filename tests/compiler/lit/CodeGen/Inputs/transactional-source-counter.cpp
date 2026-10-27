#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>

// Physical ports belong to this test adapter, never to the authored Module.
int main() {
  gfsim::WorkExecutor executor(1);
  pyc_root dut("counter", &executor);
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
  for (unsigned expected = 0; expected < 3; ++expected) {
    sample(0);
    if (!dut.out0.isFullyKnown() || dut.out0.value() != gfsim::Bits<8>{expected})
      std::abort();
    std::cout << expected << '\n';
    sample(1, expected < 2);
  }
  // The failed high sample proposed 2 -> 3. Committed State must remain two.
  sample(0);
  if (!dut.out0.isFullyKnown() || dut.out0.value() != gfsim::Bits<8>{2})
    std::abort();
}
