#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "composite family oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  top dut("pipes");
  dut.Build();
  dut.clk = known<1>(0);
  dut.rst = known<1>(0);
  constexpr std::size_t lanes = std::remove_cvref_t<decltype(dut.d)>::size;
  for (std::size_t i = 0; i < lanes; ++i) {
    dut.en.element(i) = known<1>(1);
    dut.init.element(i) = known<8>(3);
    dut.d.element(i) = known<8>(17);
  }
  auto expect = [&](unsigned first, unsigned second) {
    for (std::size_t i = 0; i < lanes; ++i) {
      require(dut.first_q.element(i).isFullyKnown() &&
              dut.second_q.element(i).isFullyKnown());
      require(dut.first_q.element(i).value() == gfsim::Bits<8>{first});
      require(dut.second_q.element(i).value() == gfsim::Bits<8>{second});
    }
  };
  dut.Reset();
  dut.Xfer();
  dut.Work();
  expect(3, 3);
  dut.clk = known<1>(1);
  dut.Work();
  expect(3, 3);
  dut.Xfer();
  dut.Work();
  expect(17, 3); // each descendant observes its own old Q.
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  dut.clk = known<1>(1);
  dut.Work();
  dut.Xfer();
  dut.Work();
  expect(17, 17);
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  for (std::size_t i = 0; i < lanes; ++i)
    dut.d.element(i) = known<8>(42);
  dut.clk = known<1>(1);
  dut.en.element(lanes - 1) = gfsim::wire<gfsim::Bits<1>>::unknown();
  bool rejected = false;
  try {
    dut.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  dut.Xfer();
  dut.en.element(lanes - 1) = known<1>(1);
  dut.Work();
  expect(17, 17);
  dut.Xfer();
  dut.Work();
  expect(42, 17);
}
