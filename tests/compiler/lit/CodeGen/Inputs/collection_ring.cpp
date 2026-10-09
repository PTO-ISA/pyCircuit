#include "pycircuit_system.hpp"
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <source_location>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "collection ring oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <class Array>
void expect(const Array &array, std::initializer_list<unsigned> values) {
  require(array.size == values.size());
  std::size_t i = 0;
  for (auto value : values) {
    require(array.element(i).isFullyKnown());
    require(array.element(i++).value() == gfsim::Bits<8>{value});
  }
}
int main() {
  top dut("ring");
  dut.Build();
  dut.clk = known<1>(0);
  dut.rst = known<1>(0);
  for (unsigned i = 0; i < 3; ++i) {
    dut.en_left.element(i) = known<1>(1);
    dut.en_right.element(i) = known<1>(1);
    dut.init_left.element(i) = known<8>(i + 1);
    dut.init_right.element(i) = known<8>(10 + i);
    dut.data.element(i) = known<8>(20 + i);
  }
  dut.Reset();
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {1, 2, 3});
  expect(dut.qb, {10, 11, 12});
  dut.clk = known<1>(1);
  dut.Work();
  expect(dut.qa, {1, 2, 3});
  expect(dut.qb, {10, 11, 12});
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {2, 3, 1});
  expect(dut.qb, {20, 21, 22});
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  dut.clk = known<1>(1);
  dut.en_right.element(0) = known<1>(0);
  dut.data.element(0) = known<8>(99);
  dut.Work();
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {3, 1, 2});
  expect(dut.qb, {20, 21, 22});
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  dut.clk = known<1>(1);
  dut.Work();
  dut.DiscardNext();
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {3, 1, 2});
  expect(dut.qb, {20, 21, 22});
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {1, 2, 3});
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  dut.clk = known<1>(1);
  dut.en_left.element(2) = gfsim::wire<gfsim::Bits<1>>::unknown();
  bool rejected = false;
  try {
    dut.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  dut.Xfer();
  dut.en_left.element(2) = known<1>(1);
  dut.Work();
  expect(dut.qa, {1, 2, 3});
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {2, 3, 1}); // failed clock proposal was discarded too.
  dut.clk = known<1>(0);
  dut.Work();
  dut.Xfer();
  dut.clk = known<1>(1);
  dut.rst = known<1>(1);
  dut.Work();
  dut.Xfer();
  dut.Work();
  expect(dut.qa, {1, 2, 3});
  expect(dut.qb, {10, 11, 12});
}
