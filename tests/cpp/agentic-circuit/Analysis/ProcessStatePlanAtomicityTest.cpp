#include "Analysis/ProcessStatePlanInternal.h"
#include "Analysis/ProcessStatePlanTestHooks.h"
#include "ProcessStatePlanTestSupport.h"
#include "acir/Analysis/ProcessStatePlan.h"
#include "acir/InitAllDialects.h"

#include "gtest/gtest.h"

namespace acir {
namespace {

using PlanSetBuilder = detail::PlanSetBuilder;

TEST(ProcessStatePlanAtomicityTest, SerializationIsDeterministic) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  auto result1 = serializeProcessStatePlan(*plans);
  auto result2 = serializeProcessStatePlan(*plans);
  ASSERT_TRUE(static_cast<bool>(result1));
  ASSERT_TRUE(static_cast<bool>(result2));
  EXPECT_EQ(*result1, *result2);
}

TEST(ProcessStatePlanAtomicityTest,
     CloneWithMissingWakeCalleeFailsVerification) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  auto clone = PlanSetBuilder::cloneWithMissingWakeCallee(*plans);
  auto result = verifyProcessStatePlan(clone);
  EXPECT_TRUE(mlir::failed(result));
}

TEST(ProcessStatePlanAtomicityTest, CloneWithDanglingSuspendFailsVerification) {
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = test::parseAndFreezeYieldOnly(context);
  auto plans = PlanSetBuilder::buildYieldOnly(*module);
  ASSERT_TRUE(mlir::succeeded(plans));
  auto clone = PlanSetBuilder::cloneWithDanglingSuspendTransition(*plans);
  auto result = verifyProcessStatePlan(clone);
  EXPECT_TRUE(mlir::failed(result));
}

TEST(ProcessStatePlanAtomicityTest,
     PublicFactoryCanonicalizesDeclarationPermutation) {
  mlir::MLIRContext context;
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  context.appendDialectRegistry(registry);
  auto first = test::parseAndFreezeYieldPermutation(context, false);
  auto second = test::parseAndFreezeYieldPermutation(context, true);
  ASSERT_TRUE(first && second);
  auto firstPlan = planProcessState(*first);
  auto secondPlan = planProcessState(*second);
  ASSERT_TRUE(mlir::succeeded(firstPlan));
  ASSERT_TRUE(mlir::succeeded(secondPlan));
  auto firstReport = serializeProcessStatePlan(*firstPlan);
  auto secondReport = serializeProcessStatePlan(*secondPlan);
  ASSERT_TRUE(static_cast<bool>(firstReport));
  ASSERT_TRUE(static_cast<bool>(secondReport));
  EXPECT_EQ(*firstReport, *secondReport);
}

TEST(ProcessStatePlanAtomicityTest,
     PublicFactoryFailurePreservesFrozenModuleBytes) {
  mlir::MLIRContext context;
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  context.appendDialectRegistry(registry);
  auto module = test::parseAndFreezeYieldPermutation(context, false);
  ASSERT_TRUE(module);
  std::string textBefore = test::moduleText(*module);
  std::string bytesBefore = test::moduleBytecode(*module);
  ProcessStateLimits limits;
  limits.maxProcesses = 1;
  mlir::ScopedDiagnosticHandler suppress(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(planProcessState(*module, limits)));
  EXPECT_EQ(test::moduleText(*module), textBefore);
  EXPECT_EQ(test::moduleBytecode(*module), bytesBefore);
}

} // namespace
} // namespace acir
