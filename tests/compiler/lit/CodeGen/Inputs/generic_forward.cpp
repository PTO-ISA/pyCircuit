#include "pycircuit_system.hpp"
#include <cstdlib>
#include <type_traits>
int main() {
  top dut("generic"); dut.Build();
  using Payload = typename std::remove_cvref_t<decltype(dut.p)>::value_type;
  Payload value{gfsim::Bits<5>{3}, gfsim::shl(gfsim::Bits<70>{1}, 67)};
  dut.a = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{17});
  dut.p = gfsim::wire<Payload>::known(value); dut.Work();
  if (!dut.b.isFullyKnown() || dut.b.value() != gfsim::Bits<8>{17} ||
      !dut.q.isFullyKnown() || dut.q.value().tag != value.tag || dut.q.value().data != value.data)
    std::abort();
  dut.a = gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(gfsim::Bits<8>{1}, gfsim::Bits<8>{1}, gfsim::Bits<8>{4}));
  dut.p = gfsim::wire<Payload>::fromPacked(gfsim::FourState<75>::highImpedance());
  dut.Work();
  if (dut.b.packed().knownMask() != gfsim::Bits<8>{1} || dut.b.packed().zMask() != gfsim::Bits<8>{4} ||
      dut.q.packed().knownMask() != gfsim::Bits<75>{0} || dut.q.packed().zMask() != gfsim::Bits<75>::ones())
    std::abort();
}
