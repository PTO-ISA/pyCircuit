#include "Analysis/ProcessStatePlanInternal.h"
#include "Analysis/ProcessStatePlanTestHooks.h"
#include "ProcessStatePlanTestSupport.h"
#include "acir/Analysis/ProcessStatePlan.h"
#include "acir/InitAllDialects.h"

#include "gtest/gtest.h"

namespace acir {
namespace {

using PlanSetBuilder = detail::PlanSetBuilder;

static ProcessStatePlanSet getYieldOnlyPlan() {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto built = PlanSetBuilder::buildYieldOnly(*module);
  assert(mlir::succeeded(built));
  return *built;
}

TEST(ProcessStatePlanEmissionTest, YieldOnlyBlockCostIsTwo) {
  auto plans = getYieldOnlyPlan();
  const auto &process = plans.processes()[0];
  ASSERT_GE(process.blocks().size(), 1u);
  EXPECT_EQ(process.blocks()[0].cost(), 2u);
}

TEST(ProcessStatePlanEmissionTest, YieldOnlyNoLiveSlots) {
  auto plans = getYieldOnlyPlan();
  const auto &process = plans.processes()[0];
  EXPECT_EQ(process.liveSlots().size(), 0u);
}

TEST(ProcessStatePlanEmissionTest, YieldOnlyFairnessEqualsBlockCost) {
  auto plans = getYieldOnlyPlan();
  const auto &process = plans.processes()[0];
  EXPECT_EQ(process.fairnessWork(), 2u);
}

TEST(ProcessStatePlanEmissionTest, YieldOnlyEdgeIsSuspend) {
  auto plans = getYieldOnlyPlan();
  const auto &process = plans.processes()[0];
  ASSERT_GE(process.blocks().size(), 1u);
  EXPECT_EQ(process.blocks()[0].edge().kind(), ProcessControlEdgeKind::Suspend);
}

TEST(ProcessStatePlanEmissionTest, NoCapturesInYieldOnly) {
  auto plans = getYieldOnlyPlan();
  const auto &process = plans.processes()[0];
  EXPECT_EQ(process.captures().size(), 0u);
}

TEST(ProcessStatePlanEmissionTest, NoValueTypesInYieldOnly) {
  auto plans = getYieldOnlyPlan();
  EXPECT_EQ(plans.valueTypes().size(), 0u);
}

TEST(ProcessStatePlanEmissionTest, OneCalleeInYieldOnly) {
  auto plans = getYieldOnlyPlan();
  ASSERT_GE(plans.callees().size(), 1u);
  EXPECT_EQ(plans.callees()[0].id().value(), 0u);
  EXPECT_EQ(plans.callees()[0].role(), ProcessHelperRole::WakeNextDelta);
}

} // namespace
} // namespace acir
