#include "pycircuit_system.hpp"
#include <cstdlib>
void require(bool value) {
  if (!value)
    std::abort();
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <unsigned W>
void expect(const gfsim::wire<gfsim::Bits<W>> &wire, unsigned value) {
  require(wire.isFullyKnown() && wire.value() == gfsim::Bits<W>{value});
}
int main() {
  top dut("edges");
  dut.Build();
  dut.t1.element(0) = gfsim::wire<gfsim::Bits<8>>::fromPacked(
      gfsim::FourState<8>::highImpedance());
  dut.t3.element(0) = known<8>(250);
  dut.t3.element(1) = known<8>(10);
  dut.t3.element(2) = known<8>(5);
  for (unsigned i = 0; i < 4; ++i)
    dut.t4.element(i) = known<8>(i + 1);
  for (unsigned i = 0; i < 5; ++i)
    dut.t5.element(i) = known<8>(i + 1);
  for (unsigned i = 0; i < 65; ++i)
    dut.t65.element(i) = known<8>(1);
  dut.coordinate = known<70>(0);
  dut.Work();
  require(dut.f1_add.packed().zMask() == gfsim::Bits<8>::ones());
  require(dut.f1_and.packed().zMask() == gfsim::Bits<8>::ones());
  require(dut.f1_min.packed().zMask() == gfsim::Bits<8>::ones());
  expect(dut.f3_add, 9);
  expect(dut.f3_mul, 212);
  expect(dut.f5_add, 15);
  expect(dut.f5_mul, 120);
  expect(dut.f5_and, 0);
  expect(dut.f5_or, 7);
  expect(dut.f5_xor, 1);
  expect(dut.f5_min, 1);
  expect(dut.f5_max, 5);
  expect(dut.f65_add, 65);
  expect(dut.f65_mul, 1);
  expect(dut.index1, 0);
  expect(dut.index3, 0);
  expect(dut.index4, 0);
  expect(dut.range1, 1);
  expect(dut.range3, 1);
  expect(dut.range4, 1);
  require(dut.value1.packed().zMask() == gfsim::Bits<8>::ones());
  // Balanced adjacent tree differs from a serial minimum under X. For
  // [0,3,2,0X,1], pair (2,0X) loses both low known bits before merging
  // with pair (0,3); a serial fold would incorrectly retain bit 1 as zero.
  dut.t5.element(0) = known<8>(0);
  dut.t5.element(1) = known<8>(3);
  dut.t5.element(2) = known<8>(2);
  dut.t5.element(3) =
      gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(
          gfsim::Bits<8>{0}, gfsim::Bits<8>{0xfe}, gfsim::Bits<8>{0}));
  dut.t5.element(4) = known<8>(1);
  dut.Work();
  require(dut.f5_min.packed().knownMask() == gfsim::Bits<8>{0xfc});
  require(dut.f5_max.packed().knownMask() == gfsim::Bits<8>{0xfc});
  dut.coordinate = known<70>(3);
  dut.Work();
  expect(dut.index1, 1);
  expect(dut.index3, 3);
  expect(dut.index4, 3);
  expect(dut.range1, 0);
  expect(dut.range3, 0);
  expect(dut.range4, 1);
  expect(dut.value4, 4);
  dut.coordinate = known<70>(4);
  dut.Work();
  expect(dut.index4, 4);
  expect(dut.range4, 0);
  dut.coordinate = gfsim::wire<gfsim::Bits<70>>::fromPacked(
      gfsim::FourState<70>::highImpedance());
  dut.Work();
  require(dut.index1.packed().knownMask() == gfsim::Bits<1>{0});
  require(dut.index3.packed().knownMask() == gfsim::Bits<2>{0});
  require(dut.index4.packed().knownMask() == gfsim::Bits<3>{0});
  require(dut.range1.packed().knownMask() == gfsim::Bits<1>{0});
  require(dut.range3.packed().knownMask() == gfsim::Bits<1>{0});
  require(dut.range4.packed().knownMask() == gfsim::Bits<1>{0});
}
