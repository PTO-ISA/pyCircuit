#include "gfsim/SimDFF.h"
#include "gtest/gtest.h"

namespace {

TEST(RegRuntimeTest, V10_DffePublishesOnlyEnabledWritesAtXfer) {
  gfsim::SimDFFE<int> reg(3);
  static_assert(noexcept(reg.Xfer()));
  EXPECT_EQ(reg.Read(), 3);

  EXPECT_TRUE(reg.Write(99, false));
  EXPECT_TRUE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 3);
  reg.Xfer();
  EXPECT_FALSE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 3);

  // Disabled pending data is consumed, never replayed by a later empty cycle.
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 3);

  EXPECT_TRUE(reg.Write(7, true));
  EXPECT_EQ(reg.Read(), 3);
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 7);

  reg.DiscardNext();
  EXPECT_FALSE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 7);
}

TEST(RegRuntimeTest, V11_ResetIsPendingAndWinsAtXfer) {
  gfsim::SimDFFE<int> reg(3);
  EXPECT_TRUE(reg.Write(7, true));
  reg.Reset();
  EXPECT_TRUE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 3);
  reg.Xfer();
  EXPECT_FALSE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 3);

  EXPECT_TRUE(reg.Write(9, true));
  reg.DiscardNext();
  EXPECT_FALSE(reg.HasPending());
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 3);
}

TEST(RegRuntimeTest, DuplicateWriteRejectsWithoutReplacingFirstPair) {
  gfsim::SimDFFE<int> reg(3);
  EXPECT_TRUE(reg.Write(5, true));
  EXPECT_FALSE(reg.Write(9, true));
  EXPECT_TRUE(reg.HasPending());
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 5);
  EXPECT_FALSE(reg.HasPending());
}

TEST(RegRuntimeTest, WriteAfterResetIsRejectedAndResetWinsAtXfer) {
  gfsim::SimDFFE<int> reg(3);
  EXPECT_TRUE(reg.Write(7, true));
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 7);

  reg.Reset();
  EXPECT_FALSE(reg.Write(11, true));
  EXPECT_TRUE(reg.HasPending());
  EXPECT_EQ(reg.Read(), 7);
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 3);
  EXPECT_FALSE(reg.HasPending());
}

TEST(RegRuntimeTest, DiscardClearsPendingWriteAndPendingReset) {
  gfsim::SimDFFE<int> reg(3);
  EXPECT_TRUE(reg.Write(7, true));
  reg.DiscardNext();
  EXPECT_FALSE(reg.HasPending());
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 3);

  EXPECT_TRUE(reg.Write(8, true));
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 8);
  reg.Reset();
  reg.DiscardNext();
  EXPECT_FALSE(reg.HasPending());
  reg.Xfer();
  EXPECT_EQ(reg.Read(), 8);
}

} // namespace
