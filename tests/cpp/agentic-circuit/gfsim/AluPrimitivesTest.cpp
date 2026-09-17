#include "gfsim/alu.h"
#include "gfsim/bitfield.h"

#include "gtest/gtest.h"

#include <cstdint>

namespace gfsim {
namespace {

TEST(AluPrimitivesTest, WordArithmeticLogicAndShiftsWrapArchitecturally) {
  EXPECT_EQ(addw(UInt<64>{0x7fffffff}, UInt<64>{1}).value(),
            0xffffffff80000000ULL);
  EXPECT_EQ(subw(UInt<64>{0}, UInt<64>{1}).value(), 0xffffffffffffffffULL);
  EXPECT_EQ(andw(UInt<64>{0xffffffff}, UInt<64>{0x80000001}).value(),
            0xffffffff80000001ULL);
  EXPECT_EQ(orw(UInt<64>{0x80000000}, UInt<64>{0}).value(),
            0xffffffff80000000ULL);
  EXPECT_EQ(xorw(UInt<64>{0x80000000}, UInt<64>{0xffffffff}).value(),
            0x7fffffffULL);

  EXPECT_EQ(sll(UInt<64>{1}, UInt<64>{64}).value(), 1ULL);
  EXPECT_EQ(srl(UInt<64>{0x80}, UInt<64>{67}).value(), 0x10ULL);
  EXPECT_EQ(sra(UInt<64>{0x8000000000000000ULL}, UInt<64>{63}).value(),
            0xffffffffffffffffULL);
  EXPECT_EQ(sllw(UInt<64>{1}, UInt<64>{32}).value(), 1ULL);
  EXPECT_EQ(srlw(UInt<64>{0xffffffff}, UInt<64>{4}).value(), 0x0fffffffULL);
  EXPECT_EQ(sraw(UInt<64>{0x80000000}, UInt<64>{4}).value(),
            0xfffffffff8000000ULL);
}

TEST(AluPrimitivesTest, SignedUnsignedCompareAndMultiplyWrap) {
  EXPECT_EQ(smin(UInt<64>{0x8000000000000000ULL}, UInt<64>{0}).value(),
            0x8000000000000000ULL);
  EXPECT_EQ(smax(UInt<64>{0x8000000000000000ULL}, UInt<64>{0}).value(), 0ULL);
  EXPECT_EQ(umin(UInt<64>{1}, UInt<64>{~std::uint64_t{0}}).value(), 1ULL);
  EXPECT_EQ(umax(UInt<64>{1}, UInt<64>{~std::uint64_t{0}}).value(),
            ~std::uint64_t{0});
  EXPECT_EQ(mulw(UInt<64>{0xffffffff}, UInt<64>{2}).value(),
            0xfffffffffffffffeULL);
  EXPECT_EQ(madd(UInt<64>{7}, UInt<64>{5}, UInt<64>{3}).value(), 38ULL);
  EXPECT_EQ(maddw(UInt<64>{0xffffffff}, UInt<64>{2}, UInt<64>{1}).value(),
            0xffffffffffffffffULL);
  EXPECT_EQ(msub(UInt<64>{7}, UInt<64>{5}, UInt<64>{3}).value(),
            0xffffffffffffffe0ULL);
}

TEST(AluPrimitivesTest, WrappingBitfieldsAndZeroScansHaveDefinedEdges) {
  constexpr UInt<64> wrapped =
      bitfieldInsert(UInt<64>{0}, UInt<64>{0xab}, UInt<7>{8}, UInt<6>{60});
  static_assert(wrapped.value() == 0xb000000000000000ULL + 0xAULL);
  EXPECT_EQ(wrapped.value(), 0xb000000000000000ULL + 0xAULL);
  EXPECT_EQ(bitfieldExtract(UInt<64>{0xf000000000000001ULL}, UInt<7>{8},
                            UInt<6>{60}, false)
                .value(),
            0x1fULL);
  EXPECT_EQ(bitfieldClz(UInt<64>{0}, UInt<7>{8}, UInt<6>{0}).value(), 8ULL);
  EXPECT_EQ(bitfieldCtz(UInt<64>{0}, UInt<7>{8}, UInt<6>{0}).value(), 8ULL);
  EXPECT_EQ(
      bitfieldReverseBytes(UInt<64>{0xffff}, UInt<7>{7}, UInt<6>{0}).value(),
      0ULL);
  EXPECT_EQ(bitfieldClear(UInt<64>{0xffff}, UInt<7>{8}, UInt<6>{4}).value(),
            0xf00fULL);
  EXPECT_EQ(bitfieldSet(UInt<64>{0}, UInt<7>{8}, UInt<6>{4}).value(), 0xff0ULL);
}

TEST(AluPrimitivesTest, SelectExtensionsAndNegatedFalsePath) {
  EXPECT_EQ(
      csel(UInt<1>{1}, UInt<64>{0x1111}, UInt<64>{0x2222}, UInt<1>{1}).value(),
      0x1111ULL);
  EXPECT_EQ(
      csel(UInt<1>{0}, UInt<64>{0x1111}, UInt<64>{0x2222}, UInt<1>{0}).value(),
      0x2222ULL);
  EXPECT_EQ(
      csel(UInt<1>{0}, UInt<64>{0x1111}, UInt<64>{0x2222}, UInt<1>{1}).value(),
      0xffffffffffffdddeULL);
  EXPECT_EQ(sextLow(UInt<64>{0x80}, UInt<7>{8}).value(), 0xffffffffffffff80ULL);
  EXPECT_EQ(zextLow(UInt<64>{0x180}, UInt<7>{8}).value(), 0x80ULL);
}

} // namespace
} // namespace gfsim
