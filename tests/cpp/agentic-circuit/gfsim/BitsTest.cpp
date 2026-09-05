#include "gfsim/bits.h"
#include "gfsim/count_leading_zeros.h"
#include "gfsim/popcount.h"
#include "gfsim/priority_encode.h"

#include "gtest/gtest.h"

#include <cstdint>

namespace gfsim {
namespace {

TEST(UIntTest, OperationsTruncateToDeclaredWidth) {
  UInt<3> seven = 7;
  UInt<3> one = 1;

  EXPECT_EQ(0u, (seven + one).value());
  EXPECT_EQ(6u, (seven - one).value());
  EXPECT_EQ(1u, (seven * seven).value());
  EXPECT_EQ(7u, (seven / one).value());
  EXPECT_EQ(0u, (seven / UInt<3>{0}).value());
  EXPECT_EQ(1u, (seven & one).value());
  EXPECT_EQ(7u, (seven | one).value());
  EXPECT_EQ(6u, (seven ^ one).value());
  EXPECT_EQ(0u, (~seven).value());
}

TEST(UIntTest, OneBitArithmeticUsesModuloTwo) {
  UInt<1> one = 1;

  EXPECT_EQ(0u, (one + one).value());
  EXPECT_EQ(0u, (one + 1).value());
  EXPECT_EQ(0u, (~one).value());
  EXPECT_EQ(0u, (one << 1).value());
  EXPECT_TRUE(one);
}

TEST(UIntTest, ShiftsAndComparisonsAreUnsignedAndWidthBounded) {
  UInt<7> high = 0x40;
  UInt<7> low = 0x03;

  EXPECT_TRUE(high > low);
  EXPECT_EQ(0x0cu, (low << UInt<7>{2}).value());
  EXPECT_EQ(0x10u, (high >> UInt<7>{2}).value());
  EXPECT_EQ(0u, (high << UInt<7>{7}).value());
  EXPECT_EQ(0u, (high >> UInt<7>{7}).value());
  EXPECT_EQ(0u, (high << 8).value());
  EXPECT_EQ(0u, (high >> 8).value());
  EXPECT_EQ(0x40u, static_cast<std::uint64_t>(high));
}

TEST(UIntTest, SignedViewAndArithmeticShiftRespectDeclaredWidth) {
  UInt<3> negativeOne = 7;
  UInt<3> negativeFour = 4;
  UInt<3> positiveThree = 3;

  EXPECT_EQ(-1, negativeOne.signedValue());
  EXPECT_EQ(-4, negativeFour.signedValue());
  EXPECT_EQ(3, positiveThree.signedValue());
  EXPECT_EQ(7u, negativeOne.arithmeticShiftRight(UInt<3>{3}).value());
  EXPECT_EQ(6u, negativeFour.arithmeticShiftRight(UInt<3>{1}).value());
  EXPECT_EQ(0u, positiveThree.arithmeticShiftRight(UInt<3>{3}).value());
  EXPECT_EQ(1u, PacketTraits<UInt<3>>::serializedSize);
  EXPECT_EQ(2u, PacketTraits<UInt<13>>::serializedSize);
}

TEST(UIntTest, SixtyFourBitArithmeticAlsoWrapsExactly) {
  UInt<64> maximum = ~std::uint64_t{0};
  UInt<64> one = 1;

  EXPECT_EQ(0u, (maximum + one).value());
  EXPECT_EQ(~std::uint64_t{0}, (one - UInt<64>{2}).value());
}

TEST(PriorityEncodeTest, SimQueueBlockPreservesLowAndHighOrder) {
  SimQueue<UInt<13>> lowInput("low_input", 1, nullptr, 1);
  SimQueue<PriorityEncodeResult<13>> lowOutput("low_output", 2, nullptr, 1);
  PriorityEncode<13, true> low("low", 3, nullptr, lowInput, lowOutput);
  SimQueue<UInt<13>> highInput("high_input", 4, nullptr, 1);
  SimQueue<PriorityEncodeResult<13>> highOutput("high_output", 5, nullptr, 1);
  PriorityEncode<13, false> high("high", 6, nullptr, highInput, highOutput);
  const UInt<13> mask = (std::uint64_t{1} << 11) | (std::uint64_t{1} << 3);

  ASSERT_TRUE(lowInput.proposePush(mask));
  ASSERT_TRUE(highInput.proposePush(mask));
  lowInput.doXfer({0, 0});
  highInput.doXfer({0, 0});
  low.doWork({1, 0});
  high.doWork({1, 0});
  lowOutput.doXfer({1, 0});
  highOutput.doXfer({1, 0});
  lowInput.doXfer({1, 0});
  highInput.doXfer({1, 0});

  ASSERT_EQ(lowOutput.committedSize(), 1u);
  ASSERT_EQ(highOutput.committedSize(), 1u);
  EXPECT_EQ(lowOutput.peek()->index.value(), 3u);
  EXPECT_TRUE(static_cast<bool>(lowOutput.peek()->valid));
  EXPECT_EQ(highOutput.peek()->index.value(), 11u);
  EXPECT_TRUE(static_cast<bool>(highOutput.peek()->valid));
}

TEST(PopcountTest, ExactWidthsAndSimQueueBlockAgree) {
  static_assert(PopcountWidth<1> == 1);
  static_assert(PopcountWidth<13> == 4);
  static_assert(PopcountWidth<64> == 7);
  EXPECT_EQ(populationCount(UInt<1>{1}).value(), 1u);
  EXPECT_EQ(populationCount(UInt<13>{0x1123}).value(), 5u);
  EXPECT_EQ(populationCount(UInt<64>{~std::uint64_t{0}}).value(), 64u);

  SimQueue<UInt<13>> input("input", 1, nullptr, 1);
  SimQueue<UInt<4>> output("output", 2, nullptr, 1);
  Popcount<13> block("popcount", 3, nullptr, input, output);
  ASSERT_TRUE(input.proposePush(UInt<13>{0x1123}));
  input.doXfer({0, 0});
  block.doWork({1, 0});
  input.doXfer({1, 0});
  output.doXfer({1, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(output.peek()->value(), 5u);
}

TEST(CountLeadingZerosTest, ExactWidthsAndSimQueueBlockAgree) {
  static_assert(CountLeadingZerosWidth<1> == 1);
  static_assert(CountLeadingZerosWidth<13> == 4);
  static_assert(CountLeadingZerosWidth<64> == 7);
  EXPECT_EQ(countLeadingZeros(UInt<1>{0}).value(), 1u);
  EXPECT_EQ(countLeadingZeros(UInt<1>{1}).value(), 0u);
  EXPECT_EQ(countLeadingZeros(UInt<13>{0}).value(), 13u);
  EXPECT_EQ(countLeadingZeros(UInt<13>{0x0123}).value(), 4u);
  EXPECT_EQ(countLeadingZeros(UInt<64>{1}).value(), 63u);

  SimQueue<UInt<13>> input("clz_input", 4, nullptr, 1);
  SimQueue<UInt<4>> output("clz_output", 5, nullptr, 1);
  CountLeadingZeros<13> block("count_leading_zeros", 6, nullptr, input, output);
  ASSERT_TRUE(input.proposePush(UInt<13>{0x0123}));
  input.doXfer({0, 0});
  block.doWork({1, 0});
  input.doXfer({1, 0});
  output.doXfer({1, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(output.peek()->value(), 4u);
}

} // namespace
} // namespace gfsim
