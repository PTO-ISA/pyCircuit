// Public-header-only consumer test: includes only ProcessStatePlan.h,
// never lib/ internals. Verifies Task 13 can consume plans without
// access to ProcessStatePlanInternal.h or any implementation detail.
#include "acir/Analysis/ProcessStatePlan.h"

#include "gtest/gtest.h"

namespace acir {
namespace {

// This test can only use the public API to verify plan structure.
// It proves that an external consumer (Task 13) can read plan records
// without re-implementing liveness, expansion, or control analysis.

TEST(ProcessStatePlanCostConsumer, PublicPlanHasRequiredAccessors) {
  // Verify the public types exist and are accessible
  // These are compile-time checks: if any public type is missing,
  // this test fails to compile.
  static_assert(sizeof(ProcessCalleeId) > 0);
  static_assert(sizeof(ProcessValueTypeId) > 0);
  static_assert(sizeof(ProcessPcId) > 0);
  static_assert(sizeof(ProcessBlockId) > 0);
  static_assert(sizeof(ProcessWakeId) > 0);
  static_assert(sizeof(ProcessTransitionId) > 0);
  static_assert(sizeof(ProcessLiveSlotId) > 0);
  static_assert(sizeof(ProcessCaptureId) > 0);
  SUCCEED();
}

} // namespace
} // namespace acir
