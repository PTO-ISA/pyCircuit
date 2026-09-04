#include "Analysis/ProcessStatePlanInternal.h"
#include "Analysis/ProcessStatePlanTestHooks.h"
#include "ProcessStatePlanTestSupport.h"
#include "acir/Analysis/ProcessStatePlan.h"
#include "acir/InitAllDialects.h"

#include "gtest/gtest.h"

namespace acir {
namespace {

using PlanSetBuilder = detail::PlanSetBuilder;

TEST(ProcessStatePlanLimitsTest, DefaultLimitsAreWithinContract) {
  ProcessStateLimits limits;
  EXPECT_EQ(limits.maxProcesses, 1U << 20);
  EXPECT_EQ(limits.maxProgramCounters, 1U << 20);
  EXPECT_EQ(limits.maxLiveSlots, 1U << 20);
  EXPECT_EQ(limits.maxWakeRecords, 1U << 20);
  EXPECT_EQ(limits.maxCalleeDescriptors, 1U << 20);
  EXPECT_EQ(limits.maxPlannedOperations, 1U << 20);
  EXPECT_EQ(limits.maxFairnessWork, 1U << 20);
  EXPECT_EQ(limits.maxTransitions, 1U << 22);
  EXPECT_EQ(limits.maxNestedRegionDepth, 512u);
  EXPECT_EQ(limits.maxCanonicalReportBytes, 1U << 24);
}

TEST(ProcessStatePlanLimitsTest, YieldOnlyPlanWithinAllLimits) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  EXPECT_LE(plans->processes().size(), 1U << 20);
  const auto &process = plans->processes()[0];
  EXPECT_LE(process.pcs().size(), 1U << 20);
  EXPECT_LE(process.blocks().size(), 1U << 20);
  EXPECT_LE(process.wakes().size(), 1U << 20);
  EXPECT_LE(process.transitions().size(), 1U << 22);
  EXPECT_LE(process.liveSlots().size(), 1U << 20);
  EXPECT_LE(process.fairnessWork(), 1U << 20);
  EXPECT_LE(plans->callees().size(), 1U << 20);
}

TEST(ProcessStatePlanLimitsTest, VerificationPassesOnValidPlan) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  auto result = verifyProcessStatePlan(*plans);
  EXPECT_TRUE(mlir::succeeded(result));
}

TEST(ProcessStatePlanLimitsTest, SerializationProducesExpectedContractEpoch) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  auto result = serializeProcessStatePlan(*plans);
  ASSERT_TRUE(static_cast<bool>(result));
  EXPECT_NE(result->find("\"contract_epoch\":\"0.5\""), std::string::npos);
  EXPECT_NE(result->find("\"schema\":\"acir-process-state-plan-0.1\""),
            std::string::npos);
}

TEST(ProcessStatePlanLimitsTest, SerializationExceedsByteCapFails) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  ProcessStateLimits tightLimits;
  tightLimits.maxCanonicalReportBytes = 1;
  auto result = serializeProcessStatePlan(*plans, tightLimits);
  EXPECT_FALSE(static_cast<bool>(result));
}

} // namespace
} // namespace acir
