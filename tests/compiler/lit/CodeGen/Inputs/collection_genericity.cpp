#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <type_traits>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "generic shape/type/lift oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  top dut("genericity");
  dut.Build();
  using Payload = std::remove_cvref_t<decltype(dut.r3.element(0))>::value_type;
  static_assert(gfsim::hardware_traits<Payload>::width == 12);
  static_assert(std::is_base_of_v<gfsim::SimModule, family<3, 2, Payload>>);
  static_assert(std::is_base_of_v<gfsim::SimModule, family<5, 3, Payload>>);
  static_assert(
      std::is_base_of_v<gfsim::SimModule, family<3, 2, gfsim::Bits<8>>>);

  for (unsigned i = 0; i < 3; ++i) {
    Payload value{};
    value.tag = gfsim::Bits<4>{i + 1};
    value.data = gfsim::Bits<8>{10 * (i + 1)};
    dut.r3.element(i) = gfsim::wire<Payload>::known(value);
    dut.g3.element(i) = known<1>(0);
    dut.c3.element(i) = gfsim::wire<Payload>::known(value);
  }
  dut.g3.element(0) = gfsim::wire<gfsim::Bits<1>>::unknown();
  Payload candidate{};
  candidate.tag = gfsim::Bits<4>{3};
  candidate.data = gfsim::Bits<8>{31};
  dut.c3.element(0) = gfsim::wire<Payload>::known(candidate);
  for (unsigned i = 0; i < 5; ++i) {
    Payload value{};
    value.tag = gfsim::Bits<4>{i + 1};
    value.data = gfsim::Bits<8>{10 * (i + 1)};
    dut.r5.element(i) = gfsim::wire<Payload>::known(value);
    dut.g5.element(i) = known<1>(0);
    dut.c5.element(i) = gfsim::wire<Payload>::known(value);
  }
  dut.r5.element(4) =
      gfsim::wire<Payload>::fromPacked(gfsim::FourState<12>::highImpedance());
  dut.bits.element(0) = known<8>(0x12);
  dut.bits.element(1) = known<8>(0x34);
  dut.bits.element(2) = known<8>(0xa5);
  dut.bits_candidate.element(0) = known<8>(0xa7);
  dut.bits_candidate.element(1) = known<8>(99);
  dut.bits_candidate.element(2) = known<8>(88);
  dut.bits_guard.element(0) = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.bits_guard.element(1) = known<1>(1);
  dut.bits_guard.element(2) = known<1>(0);
  dut.Work();
  // Parent N=3 forwards into child M=3; T=Record forwards into child U.
  for (unsigned lane = 0; lane < 3; ++lane) {
    require(dut.o3.element(lane).packed().knownMask() ==
            gfsim::Bits<12>{0xffe});
    require((dut.o3.element(lane).packed().value() & gfsim::Bits<12>{0xffe}) ==
            gfsim::Bits<12>{0x31e});
    require(dut.o3.element(lane + 3).isFullyKnown() &&
            dut.o3.element(lane + 3).packed().value() ==
                gfsim::Bits<12>{0x10a});
    require(dut.o3.element(lane + 6).isFullyKnown() &&
            dut.o3.element(lane + 6).packed().value() ==
                gfsim::Bits<12>{0x214});
    require(dut.bo.element(lane).packed().knownMask() == gfsim::Bits<8>{0xfd});
    require((dut.bo.element(lane).packed().value() & gfsim::Bits<8>{0xfd}) ==
            gfsim::Bits<8>{0xa5});
    require(dut.bo.element(lane + 3).isFullyKnown() &&
            dut.bo.element(lane + 3).value() == gfsim::Bits<8>{99});
    require(dut.bo.element(lane + 6).isFullyKnown() &&
            dut.bo.element(lane + 6).value() == gfsim::Bits<8>{0x34});
  }
  // The same generic definitions also bind N/M=5, including zero-merge Z
  // passthrough.
  for (unsigned lane = 0; lane < 5; ++lane)
    require(dut.o5.element(lane).packed().zMask() == gfsim::Bits<12>::ones());
  const unsigned packed[4] = {0x10a, 0x214, 0x31e, 0x428};
  for (unsigned row = 0; row < 4; ++row)
    for (unsigned lane = 0; lane < 5; ++lane)
      require(dut.o5.element((row + 1) * 5 + lane).isFullyKnown() &&
              dut.o5.element((row + 1) * 5 + lane).packed().value() ==
                  gfsim::Bits<12>{packed[row]});
}
