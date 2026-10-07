#include "pycircuit_system.hpp"
#include <cstdlib>
#include <type_traits>

template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
void require(bool value) { if (!value) std::abort(); }
int main() {
  top dut("nested"); dut.Build();
  using Payload = typename std::remove_cvref_t<decltype(dut.d)>::value_type;
  using Body = decltype(Payload{}.data);
  static_assert(gfsim::hardware_traits<Body>::width == 71);
  static_assert(gfsim::hardware_traits<Payload>::width == 79);
  Payload initial{gfsim::Bits<8>{0xc3}, Body{gfsim::Bits<1>{1}, gfsim::shl(gfsim::Bits<70>{1}, 69)}};
  Payload update{gfsim::Bits<8>{0x27}, Body{gfsim::Bits<1>{0}, gfsim::Bits<70>{67}}};
  const auto expected = gfsim::shl(gfsim::Bits<79>{0xc3}, 71)
                      | gfsim::shl(gfsim::Bits<79>{1}, 70)
                      | gfsim::shl(gfsim::Bits<79>{1}, 69);
  require(gfsim::hardware_traits<Payload>::pack(initial) == expected);
  dut.init = gfsim::wire<Payload>::known(initial);
  dut.d = gfsim::wire<Payload>::known(update);
  dut.clk = known<1>(0); dut.rst = known<1>(0);
  dut.Reset(); dut.Xfer(); dut.Work();
  require(dut.q.packed().value() == expected && dut.tag.value() == initial.tag);
  dut.clk = known<1>(1); dut.Work(); dut.Xfer(); dut.Work();
  require(dut.q.isFullyKnown());
  require(dut.q.value().tag == update.tag && dut.tag.value() == update.tag);
  require(dut.q.value().data.kind == update.data.kind);
  require(dut.q.value().data.value == update.data.value);
}
