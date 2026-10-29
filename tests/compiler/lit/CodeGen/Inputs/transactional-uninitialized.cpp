#include "pycircuit_system.hpp"
#include <cstdlib>

template <class DUT> bool validate(DUT &dut) {
  gfsim::SimFailureInfo failure;
  if constexpr (requires { dut.pyc_validate_checks(failure); })
    return dut.pyc_validate_checks(failure);
  return true;
}

template <class Wire> bool all_x(const Wire &value) {
  return value.packed().knownMask() == gfsim::Bits<Wire::width>{0} &&
         value.packed().zMask() == gfsim::Bits<Wire::width>{0};
}

int main(int argc, char **argv) {
  gfsim::WorkExecutor executor(argc > 1 ? std::atoi(argv[1]) : 1);
  pyc_root dut("uninitialized", &executor);
  auto bit = [](unsigned value) {
    return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
  };
  dut.Build();
  dut.pyc_7079635f636c6b = bit(0);
  dut.pyc_7079635f727374 = bit(0);
  dut.Reset();
  dut.Xfer();
  auto sample = [&](unsigned clock, unsigned reset) {
    dut.pyc_7079635f636c6b = bit(clock);
    dut.pyc_7079635f727374 = bit(reset);
    dut.Work();
    if (!validate(dut)) std::abort();
    dut.Xfer();
  };
  auto check = [&](bool initialized) {
    sample(0, 0);
    if (initialized) {
      if (!dut.out0.isFullyKnown() || dut.out0.value() != gfsim::Bits<8>{37} ||
          !dut.out1.isFullyKnown() || !dut.out2.isFullyKnown() ||
          dut.out1.value().code != gfsim::Bits<2>{2} ||
          dut.out2.value().mode.code != gfsim::Bits<2>{2} ||
          dut.out2.value().data != gfsim::Bits<80>{37})
        std::abort();
    } else if (!all_x(dut.out0) || !all_x(dut.out1) || !all_x(dut.out2)) {
      std::abort();
    }
  };
  check(false);
  sample(1, 0);
  check(true);
  sample(1, 0);
  check(true);
  sample(1, 1);
  check(false);
  sample(1, 0);
  check(true);
}
