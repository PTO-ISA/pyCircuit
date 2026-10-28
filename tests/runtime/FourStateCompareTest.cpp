#include "gfsim/four_state.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdint>

namespace {

using Bit = gfsim::Bits<1>;
using State = gfsim::FourState<1>;

State symbol(char value, bool noise = false) {
  const bool unknown = value == 'x' || value == 'z';
  const Bit stored{unknown && noise ? 1u : value == '1' ? 1u : 0u};
  if (value == '0' || value == '1')
    return State::known(stored);
  if (value == 'x')
    return State::unknown(stored);
  return State::highImpedance(stored);
}

TEST(FourStateCompareTest, CaseEqualityCoversEveryOneBitSymbolPair) {
  constexpr std::array symbols{'0', '1', 'x', 'z'};
  for (char lhsSymbol : symbols) {
    for (char rhsSymbol : symbols) {
      const auto result = gfsim::compare<gfsim::ComparePredicate::CASE_EQ>(
          symbol(lhsSymbol), symbol(rhsSymbol, true));
      EXPECT_TRUE(result.isFullyKnown());
      EXPECT_EQ(result.zMask(), Bit{0});
      EXPECT_EQ(result.value(), Bit{lhsSymbol == rhsSymbol ? 1u : 0u});
    }
  }
}

TEST(FourStateCompareTest,
     CaseEqualityIgnoresNoiseOnlyUnderMatchingUnknownMasks) {
  using Bits = gfsim::Bits<130>;
  using FourState = gfsim::FourState<130>;
  Bits known = Bits::ones();
  Bits z{0};
  z.setWord(0, std::uint64_t{1} << 63);
  known.setWord(0, known.word(0) & ~(std::uint64_t{1} << 63));
  known.setWord(1, known.word(1) & ~std::uint64_t{1});
  Bits lhsValue{0x55};
  Bits rhsValue{0x55};
  lhsValue.setWord(0, lhsValue.word(0) | (std::uint64_t{1} << 63));
  rhsValue.setWord(1, rhsValue.word(1) | std::uint64_t{1});
  const auto lhs = FourState::fromMasks(lhsValue, known, z);
  const auto rhs = FourState::fromMasks(rhsValue, known, z);

  const auto equal = gfsim::compare<gfsim::ComparePredicate::CASE_EQ>(lhs, rhs);
  EXPECT_TRUE(equal.isFullyKnown());
  EXPECT_EQ(equal.value(), Bit{1});
  EXPECT_EQ(equal.zMask(), Bit{0});

  Bits differentKnownValue = rhsValue;
  differentKnownValue.setWord(1, differentKnownValue.word(1) |
                                     (std::uint64_t{1} << 1));
  const auto mismatch = gfsim::compare<gfsim::ComparePredicate::CASE_EQ>(
      lhs, FourState::fromMasks(differentKnownValue, known, z));
  EXPECT_TRUE(mismatch.isFullyKnown());
  EXPECT_EQ(mismatch.value(), Bit{0});
  EXPECT_EQ(mismatch.zMask(), Bit{0});
}

TEST(FourStateCompareTest, OrdinaryEqualityRetainsUnknownSemantics) {
  for (char unknown : {'x', 'z'}) {
    const auto same = gfsim::compare<gfsim::ComparePredicate::EQ>(
        symbol(unknown), symbol(unknown, true));
    EXPECT_FALSE(same.isFullyKnown());
    EXPECT_EQ(same.knownMask(), Bit{0});
    EXPECT_EQ(same.zMask(), Bit{0});
  }
  const auto knownMismatch =
      gfsim::compare<gfsim::ComparePredicate::EQ>(symbol('0'), symbol('1'));
  EXPECT_TRUE(knownMismatch.isFullyKnown());
  EXPECT_EQ(knownMismatch.value(), Bit{0});
}

TEST(FourStateCompareTest, AvailabilityNormalizationPreservesThreeOutcomes) {
  auto outcome = [](const State &value) {
    return value.isFullyKnown() ? (value.value().toBool() ? 1 : 0) : 2;
  };
  constexpr std::array symbols{'0', '1', 'x', 'z'};
  const auto zero = symbol('0'), one = symbol('1');
  for (char raw : symbols) {
    SCOPED_TRACE(raw);
    const auto value = symbol(raw);
    EXPECT_EQ(outcome(gfsim::bit_and(value, one)), outcome(value));
    EXPECT_EQ(outcome(gfsim::bit_and(value, value)), outcome(value));
    EXPECT_EQ(outcome(gfsim::select(value, one, zero)), outcome(value));
    for (unsigned extent : {1u, 3u, 5u}) {
      auto folded = one;
      for (unsigned row = 0; row < extent; ++row)
        folded = gfsim::bit_and(folded, value);
      EXPECT_EQ(outcome(folded), outcome(value));
    }
    // A reached zero blocks, a reached unknown faults, and an unreached use
    // bypasses either. Keeping only an "is one" classification would lose this.
    EXPECT_EQ(outcome(gfsim::select(one, value, one)), outcome(value));
    EXPECT_EQ(outcome(gfsim::select(zero, value, one)), 1);
    for (char other : symbols) {
      auto canonical = [](char c) { return symbol(c == 'z' ? 'x' : c); };
      EXPECT_EQ(outcome(gfsim::bit_and(value, symbol(other))),
                outcome(gfsim::bit_and(canonical(raw), canonical(other))));
      for (char fallback : symbols)
        EXPECT_EQ(outcome(gfsim::select(value, symbol(other), symbol(fallback))),
                  outcome(gfsim::select(canonical(raw), canonical(other),
                                        canonical(fallback))));
    }
  }
  // Proof equivalence does not license replacing the emitted raw operations:
  // both results are X, while their original operand is Z.
  EXPECT_EQ(gfsim::bit_and(symbol('z'), symbol('z')).zMask(), Bit{0});
  EXPECT_EQ(gfsim::select(symbol('z'), one, zero).zMask(), Bit{0});
  EXPECT_EQ(symbol('z').zMask(), Bit{1});
}

} // namespace
