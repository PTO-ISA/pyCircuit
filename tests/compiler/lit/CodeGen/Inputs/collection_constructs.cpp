#include "pycircuit_system.hpp"
#include <cstdlib>
void require(bool value) {
  if (!value)
    std::abort();
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  top dut("constructs");
  dut.Build();
  dut.pyc_696e707574.element(0) = known<8>(7);
  dut.pyc_696e707574.element(1) = known<8>(11);
  dut.pyc_696e707574.element(2) = gfsim::wire<gfsim::Bits<8>>::fromPacked(
      gfsim::FourState<8>::highImpedance());
  using QueryPayload = std::remove_cvref_t<decltype(dut.query_data.element(0))>::value_type;
  using QueryBits = gfsim::Bits<14>;
  using QueryState = gfsim::FourState<14>;
  const QueryState query_values[] = {
      QueryState::known(QueryBits{0x1234}),
      QueryState::fromMasks(QueryBits{0x2bad}, QueryBits{0}, QueryBits::ones()),
      QueryState::fromMasks(QueryBits{0x3123}, QueryBits{0x3cff}, QueryBits{0x100})};
  for (unsigned i = 0; i < 3; ++i)
    dut.query_data.element(i) = gfsim::wire<QueryPayload>::fromPacked(query_values[i]);
  for (unsigned i = 0; i < 4; ++i) dut.query_indices.element(i) = known<70>(i);
  gfsim::Bits<70> high_index{0}; high_index.setWord(1, 1);
  dut.query_indices.element(4) = gfsim::wire<gfsim::Bits<70>>::known(high_index);
  dut.Work();
  static_assert(std::remove_cvref_t<decltype(dut.nested)>::size == 6);
  for (unsigned offset : {0u, 3u}) {
    require(dut.nested.element(offset).isFullyKnown() &&
            dut.nested.element(offset).value() == gfsim::Bits<8>{7});
    require(dut.nested.element(offset + 1).isFullyKnown() &&
            dut.nested.element(offset + 1).value() == gfsim::Bits<8>{11});
    require(dut.nested.element(offset + 2).packed().zMask() ==
            gfsim::Bits<8>::ones());
  }
  // (2^200+1) mod 3 is 2; its negative has mathematical modulo 1.
  require(dut.pyc_6c61726765.element(0).packed().zMask() ==
          gfsim::Bits<8>::ones());
  require(dut.pyc_6c61726765.element(1).isFullyKnown() &&
          dut.pyc_6c61726765.element(1).value() == gfsim::Bits<8>{7});
  require(dut.pyc_6c61726765.element(2).isFullyKnown() &&
          dut.pyc_6c61726765.element(2).value() == gfsim::Bits<8>{11});
  require(dut.negative.element(0).isFullyKnown() &&
          dut.negative.element(0).value() == gfsim::Bits<8>{11});
  require(dut.negative.element(1).packed().zMask() == gfsim::Bits<8>::ones());
  require(dut.negative.element(2).isFullyKnown() &&
          dut.negative.element(2).value() == gfsim::Bits<8>{7});
  for (unsigned ordinal : {0u, 1u, 31u, 63u, 64u})
    require(dut.ordinals.element(ordinal).isFullyKnown() &&
            dut.ordinals.element(ordinal).value() == gfsim::Bits<7>{ordinal});
  require(dut.sum.isFullyKnown() &&
          dut.sum.value() == gfsim::Bits<7>{32}); // 0+...+64 = 2080 mod 128.
  auto differential = [&] {
    for (unsigned row = 0; row < 5; ++row) {
      const auto gathered = dut.query_gathered.element(row).packed();
      const auto reference = dut.query_reference.element(row).packed();
      require(gathered.knownMask() == reference.knownMask());
      require(gathered.zMask() == reference.zMask());
      const auto index = dut.query_indices.element(row).packed();
      if (index.isFullyKnown() && index.value().word(1) == 0 &&
          index.value().word(0) < 3) {
        const auto expected = dut.query_data.element(index.value().word(0)).packed();
        // A legal read transports complete value/known/Z planes, including
        // latent X/Z payload bits in nested Struct and nominal Enum fields.
        require(gathered.value() == expected.value());
        require(gathered.knownMask() == expected.knownMask());
        require(gathered.zMask() == expected.zMask());
        require(reference.value() == expected.value());
      } else {
        require(gathered.knownMask() == QueryBits{0} && gathered.zMask() == QueryBits{0});
        // Computed all-X payload values have no specified latent value plane.
      }
    }
  };
  differential();
  for (bool z : {false, true}) {
    for (unsigned i = 0; i < 3; ++i)
      dut.query_data.element(i) = gfsim::wire<QueryPayload>::fromPacked(QueryState::known(QueryBits{0x2777}));
    gfsim::Bits<70> high_unknown{0}; high_unknown.setWord(1, 1);
    auto low = gfsim::Bits<70>{1};
    dut.query_indices.element(0) = gfsim::wire<gfsim::Bits<70>>::fromPacked(
        gfsim::FourState<70>::fromMasks(low, ~high_unknown, z ? high_unknown : gfsim::Bits<70>{0}));
    dut.query_indices.element(1) = gfsim::wire<gfsim::Bits<70>>::unknown();
    dut.query_indices.element(2) = gfsim::wire<gfsim::Bits<70>>::fromPacked(
        gfsim::FourState<70>::fromMasks(gfsim::Bits<70>{2}, ~gfsim::Bits<70>{1},
                                      z ? gfsim::Bits<70>{1} : gfsim::Bits<70>{0}));
    dut.query_indices.element(3) = known<70>(3);
    dut.query_indices.element(4) = known<70>(1);
    dut.Work(); differential();
  }

}
