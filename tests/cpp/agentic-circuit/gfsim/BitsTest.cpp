#include "gfsim/bits.h"

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

} // namespace
} // namespace gfsim
