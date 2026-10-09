#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
void require(bool value,
             std::source_location at = std::source_location::current()) {
  if (!value) {
    std::cerr << "independent memory family oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  top dut("rams");
  dut.Build();
  dut.clk = known<1>(0);
  dut.rst = known<1>(0);
  for (unsigned i = 0; i < 2; ++i) {
    dut.ren.element(i) = known<1>(1);
    dut.wvalid.element(i) = known<1>(1);
    dut.raddr.element(i) = known<4>(0);
    dut.waddr.element(i) = known<4>(0);
    dut.wstrb.element(i) = known<2>(3);
  }
  dut.wdata.element(0) = known<13>(0x1555);
  dut.wdata.element(1) = known<13>(0x0666);
  auto expect = [&](unsigned a, unsigned b) {
    require(dut.q.element(0).isFullyKnown() && dut.q.element(1).isFullyKnown());
    require(dut.q.element(0).value() == gfsim::Bits<13>{a});
    require(dut.q.element(1).value() == gfsim::Bits<13>{b});
  };
  auto fall = [&] {
    dut.clk = known<1>(0);
    dut.Work();
    dut.Xfer();
  };
  auto rise = [&] {
    dut.clk = known<1>(1);
    dut.Work();
    dut.Xfer();
    dut.Work();
  };
  dut.Reset();
  dut.Xfer();
  dut.Work();
  require(!dut.q.element(0).isFullyKnown() && !dut.q.element(1).isFullyKnown());
  rise();
  expect(0, 0); // both independent memories read old zero before their writes.
  fall();
  dut.wdata.element(0) = known<13>(0x1abc);
  dut.wdata.element(1) = known<13>(0x0123);
  dut.wstrb.element(0) = known<2>(1);
  dut.wstrb.element(1) = known<2>(1);
  rise();
  expect(0x1555, 0x0666);
  fall();
  dut.wvalid.element(0) = known<1>(0);
  dut.wvalid.element(1) = known<1>(0);
  rise();
  expect(0x15bc, 0x0623);
  fall();
  dut.ren.element(0) = known<1>(0); // independent read lifetime.
  rise();
  expect(0x15bc, 0x0623);
  fall();
  rise();
  require(!dut.q.element(0).isFullyKnown());
  require(dut.q.element(1).isFullyKnown() &&
          dut.q.element(1).value() == gfsim::Bits<13>{0x0623});
  fall();
  dut.ren.element(0) = known<1>(1);
  dut.rst = known<1>(1);
  rise();
  require(!dut.q.element(0).isFullyKnown() && !dut.q.element(1).isFullyKnown());
  fall();
  dut.rst = known<1>(0);
  rise();
  expect(0x15bc, 0x0623); // reset did not erase contents.
  fall();
  dut.wvalid.element(0) = known<1>(1);
  dut.wvalid.element(1) = known<1>(1);
  dut.wdata.element(0) = known<13>(7);
  dut.wdata.element(1) = known<13>(9);
  dut.wstrb.element(0) = known<2>(3);
  dut.wstrb.element(1) = known<2>(3);
  dut.clk = known<1>(1);
  dut.waddr.element(1) = gfsim::wire<gfsim::Bits<4>>::unknown();
  bool rejected = false;
  try {
    dut.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  dut.Xfer();
  dut.waddr.element(1) = known<4>(0);
  dut.wvalid.element(0) = known<1>(0);
  dut.wvalid.element(1) = known<1>(0);
  dut.Work();
  expect(0x15bc, 0x0623);
  dut.Xfer();
  dut.Work();
  expect(0x15bc, 0x0623);
}
