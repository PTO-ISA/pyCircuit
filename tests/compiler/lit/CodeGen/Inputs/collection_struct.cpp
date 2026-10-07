#include "pycircuit_system.hpp"
#include <cstdlib>
#include <type_traits>
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
  top dut("records");
  dut.Build();
  using Payload = std::remove_cvref_t<decltype(dut.base)>::value_type;
  static_assert(gfsim::hardware_traits<Payload>::width == 28);
  Payload base{};
  base.tag = gfsim::Bits<4>{10};
  base.inner.x = gfsim::Bits<8>{0x12};
  base.inner.y = gfsim::Bits<8>{0xa5};
  base.tail = gfsim::Bits<8>{0x99};
  dut.base = gfsim::wire<Payload>::known(base);
  dut.guard_tag = known<1>(1);
  dut.guard_y = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.tag_candidate = known<4>(3);
  dut.y_candidate = known<8>(0xa7);
  const unsigned tags[3] = {2, 4, 2}, xs[3] = {7, 10, 13}, ys[3] = {8, 11, 14},
                 tails[3] = {9, 12, 15};
  for (unsigned i = 0; i < 3; ++i) {
    Payload value{};
    value.tag = gfsim::Bits<4>{tags[i]};
    value.inner.x = gfsim::Bits<8>{xs[i]};
    value.inner.y = gfsim::Bits<8>{ys[i]};
    value.tail = gfsim::Bits<8>{tails[i]};
    dut.records.element(i) = gfsim::wire<Payload>::known(value);
  }
  dut.query_tag = known<4>(2);
  dut.index = known<70>(1);
  dut.Work();
  require(dut.merged.packed().knownMask() == gfsim::Bits<28>{0xffffdff});
  require((dut.merged.packed().value() & gfsim::Bits<28>{0xffffdff}) ==
          gfsim::Bits<28>{0x312a599});
  expect(dut.merge_en, 1);
  expect(dut.zero_en, 0);
  require(dut.unchanged.packed().value() == gfsim::Bits<28>{0xa12a599});
  require(dut.unchanged.isFullyKnown());
  for (unsigned i = 0; i < 3; ++i)
    expect(dut.xs.element(i), xs[i]);
  expect(dut.mask, 5);
  expect(dut.choice0, 0);
  expect(dut.choice1, 2);
  expect(dut.valid0, 1);
  expect(dut.valid1, 1);
  expect(dut.in_range, 1);
  require(dut.selected.isFullyKnown() &&
          dut.selected.value().tag == gfsim::Bits<4>{4});
  require(dut.selected.value().inner.x == gfsim::Bits<8>{10});
  dut.base =
      gfsim::wire<Payload>::fromPacked(gfsim::FourState<28>::highImpedance());
  dut.guard_tag = known<1>(0);
  dut.guard_y = known<1>(0);
  dut.Work();
  require(dut.unchanged.packed().zMask() == gfsim::Bits<28>::ones());
  require(dut.merged.packed().zMask() == gfsim::Bits<28>::ones());
  expect(dut.merge_en, 0);
  expect(dut.zero_en, 0);
}
