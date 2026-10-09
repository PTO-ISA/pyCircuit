#include "pycircuit_system.hpp"
#include <cstdlib>
#include <type_traits>

template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
void require(bool value) { if (!value) std::abort(); }
int main() {
  top dut("wide"); dut.Build();
  using Payload = typename std::remove_cvref_t<decltype(dut.d)>::value_type;
  static_assert(gfsim::hardware_traits<Payload>::width == 75);
  Payload initial{gfsim::Bits<5>{18}, gfsim::shl(gfsim::Bits<70>{1}, 68)};
  Payload update{gfsim::Bits<5>{7}, gfsim::Bits<70>{53}};
  dut.init = gfsim::wire<Payload>::known(initial);
  dut.d = gfsim::wire<Payload>::known(update);
  dut.clk = known<1>(0); dut.rst = known<1>(0);
  dut.Reset(); dut.Xfer(); dut.Work();
  require(dut.q.isFullyKnown() && dut.tag.isFullyKnown());
  require(dut.q.value().tag == initial.tag && dut.q.value().data == initial.data);
  require(dut.tag.value() == initial.tag);
  dut.clk = known<1>(1); dut.Work();
  require(dut.q.value().data == initial.data);
  dut.Xfer(); dut.Work();
  require(dut.q.value().tag == update.tag && dut.q.value().data == update.data);
  require(dut.tag.value() == update.tag);
  // A known tag and unknown/Z data must retain field-local mask planes.
  dut.clk = known<1>(0); dut.Work(); dut.Xfer();
  using Mask = gfsim::Bits<75>;
  const auto tagValue = gfsim::shl(Mask{9}, 70);
  const auto tagKnown = gfsim::shl(Mask{31}, 70);
  const auto z = gfsim::shl(Mask{1}, 68);
  dut.d = gfsim::wire<Payload>::fromPacked(gfsim::FourState<75>::fromMasks(tagValue, tagKnown, z));
  dut.clk = known<1>(1); dut.Work(); dut.Xfer(); dut.Work();
  require(!dut.q.isFullyKnown());
  require(dut.q.packed().knownMask() == tagKnown && dut.q.packed().zMask() == z);
  require(dut.tag.isFullyKnown() && dut.tag.value() == gfsim::Bits<5>{9});
}
