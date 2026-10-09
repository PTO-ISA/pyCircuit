#include "pycircuit_system.hpp"
#include <cstdlib>

void require(bool condition) {
  if (!condition)
    std::abort();
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <unsigned W>
void expect(const gfsim::wire<gfsim::Bits<W>> &wire, unsigned value) {
  require(wire.isFullyKnown() && wire.value() == gfsim::Bits<W>{value});
}
template <class Array>
void expect_array(const Array &wire, std::initializer_list<unsigned> values) {
  require(wire.size == values.size());
  std::size_t i = 0;
  for (unsigned value : values)
    expect(wire.element(i++), value);
}
int main() {
  top dut("values");
  dut.Build();
  // Hand-derived row-major oracle; expected data never uses DUT algorithms.
  for (std::size_t i = 0; i < 6; ++i)
    dut.a.element(i) = known<8>(static_cast<unsigned>(i + 1));
  dut.threshold = known<8>(4);
  dut.row = known<70>(1);
  dut.col = known<70>(2);
  dut.get_index = known<70>(3);
  dut.guard = known<1>(1);
  dut.base = known<8>(0xa5);
  dut.candidate = known<8>(0xa7);
  dut.Work();
  expect_array(dut.mapped, {5, 6, 7, 8, 9, 10});
  expect_array(dut.transposed, {1, 4, 2, 5, 3, 6});
  expect_array(dut.sliced, {1, 3, 4, 6});
  expect_array(dut.rotated, {3, 1, 2, 6, 4, 5});
  expect_array(dut.reshaped, {1, 2, 3, 4, 5, 6});
  expect_array(dut.broadcast, {4, 4, 4, 4, 4, 4});
  expect_array(dut.created, {0xa5, 0xa7});
  expect(dut.ordinal, 5);
  expect(dut.selected, 4);
  expect(dut.in_range, 1);
  expect(dut.mask,
         56); // mask bit 0 maps to logical element 0, unlike packed table bits.
  expect(dut.low0, 3);
  expect(dut.low1, 4);
  expect(dut.high0, 5);
  expect(dut.high1, 4);
  expect(dut.lowvalid0, 1);
  expect(dut.lowvalid1, 1);
  expect(dut.highvalid0, 1);
  expect(dut.highvalid1, 1);
  expect(dut.fold_add, 21);
  expect(dut.fold_mul, 208);
  expect(dut.fold_and, 0);
  expect(dut.fold_or, 7);
  expect(dut.fold_xor, 7);
  expect(dut.fold_min, 1);
  expect(dut.fold_max, 6);
  expect(dut.merged, 0xa7);
  expect(dut.merge_en, 1);

  dut.threshold = known<8>(7);
  dut.get_index = known<70>(0);
  dut.Work();
  expect(dut.mask, 0);
  expect(dut.low0, 0);
  expect(dut.low1, 0);
  expect(dut.lowvalid0, 0);
  expect(dut.lowvalid1, 0);
  expect(dut.highvalid0, 0);
  expect(dut.highvalid1, 0);
  expect(dut.selected, 1);
  expect(dut.in_range, 1); // in_range does not mean choose hit.

  dut.row = known<70>(0);
  dut.col = known<70>(3);
  dut.get_index = known<70>(6);
  dut.Work();
  expect(dut.ordinal, 6);
  expect(dut.in_range, 0);
  require(dut.selected.packed().knownMask() == gfsim::Bits<8>{0});
  dut.row =
      gfsim::wire<gfsim::Bits<70>>::known(gfsim::shl(gfsim::Bits<70>{1}, 65));
  dut.col = known<70>(0);
  dut.get_index = dut.row;
  dut.Work();
  expect(dut.ordinal, 6);
  expect(dut.in_range, 0);
  dut.row = gfsim::wire<gfsim::Bits<70>>::unknown();
  dut.col = known<70>(3);
  dut.Work();
  expect(dut.ordinal, 6); // one known OOB axis dominates unknown other axis.
  dut.col = known<70>(2);
  dut.get_index = gfsim::wire<gfsim::Bits<70>>::unknown();
  dut.guard = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.Work();
  require(dut.ordinal.packed().knownMask() == gfsim::Bits<3>{0});
  require(dut.in_range.packed().knownMask() == gfsim::Bits<1>{0});
  require(dut.selected.packed().knownMask() == gfsim::Bits<8>{0});
  require(dut.merged.packed().knownMask() == gfsim::Bits<8>{0xfd});
  require((dut.merged.packed().value() & gfsim::Bits<8>{0xfd}) ==
          gfsim::Bits<8>{0xa5});
  require(dut.merged.packed().zMask() == gfsim::Bits<8>{0});
  require(dut.merge_en.packed().knownMask() == gfsim::Bits<1>{0});

  dut.a.element(0) = gfsim::wire<gfsim::Bits<8>>::unknown();
  dut.threshold = known<8>(4);
  dut.Work();
  require(dut.mask.packed().knownMask() == gfsim::Bits<6>{62});
  expect(dut.lowvalid0, 1);
  expect(dut.lowvalid1, 1);
  require(dut.low0.packed().knownMask() == gfsim::Bits<3>{4});
  require(dut.low1.packed().knownMask() == gfsim::Bits<3>{0});
  expect(dut.high0, 5);
  expect(dut.high1, 4);
  expect(dut.fold_and, 0);
  require(dut.fold_or.packed().knownMask() == gfsim::Bits<8>{7});
  require(dut.fold_add.packed().knownMask() == gfsim::Bits<8>{0});
  require(dut.fold_mul.packed().knownMask() == gfsim::Bits<8>{0});
  require(dut.fold_xor.packed().knownMask() == gfsim::Bits<8>{0});
}
