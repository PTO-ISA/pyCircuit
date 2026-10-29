#include "pycircuit_system.hpp"
#include <cstdlib>

template <class DUT> bool validate(DUT &dut) {
  gfsim::SimFailureInfo failure;
  if constexpr (requires { dut.pyc_validate_checks(failure); })
    return dut.pyc_validate_checks(failure);
  return true;
}

int main(int argc, char **argv) {
  gfsim::WorkExecutor executor(argc > 1 ? std::atoi(argv[1]) : 1);
  pyc_root dut("interfaces", &executor);
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
  auto check = [&](unsigned incremented, unsigned first, unsigned second) {
    sample(0, 0);
    auto matches = [](const auto &wire, unsigned expected) {
      return wire.isFullyKnown() && wire.value() == gfsim::Bits<8>{expected};
    };
    if (!matches(dut.out0, incremented) || !matches(dut.out1, first) ||
        !matches(dut.out2, second) || !matches(dut.out3, first)) std::abort();
  };
  check(0, 20, 30);
  sample(1, 0); check(4, 0, 10);
  sample(1, 0); check(4, 5, 11);
  sample(1, 0); check(4, 5, 11);
  sample(1, 1); check(0, 20, 30);
  sample(1, 0); check(4, 0, 10);
  sample(1, 0); check(4, 5, 11);
}
