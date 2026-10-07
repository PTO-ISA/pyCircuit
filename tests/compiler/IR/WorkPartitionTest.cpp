#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "gtest/gtest.h"

#include <cstdlib>

namespace {
namespace ac = acir::ac;
class WorkPartitionTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp top;
  ac::InstanceOp left, right;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    const char *path = std::getenv("PYCIRCUIT_TEST_MODULE_LOOP_FINAL");
    ASSERT_NE(path, nullptr)
        << "set path to the actual public module_loop linked artifact";
    package = mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
    ASSERT_TRUE(package);
    ASSERT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(*package)));
    top =
        mlir::dyn_cast_or_null<ac::ModuleOp>(mlir::SymbolTable::lookupSymbolIn(
            *package, "example_loop.module_loop.Top"));
    ASSERT_TRUE(top);
    for (auto instance : top.getBody().front().getOps<ac::InstanceOp>()) {
      if (instance.getInstanceName() == "left_cell")
        left = instance;
      if (instance.getInstanceName() == "right_cell")
        right = instance;
    }
    ASSERT_TRUE(left);
    ASSERT_TRUE(right);
  }
};
TEST_F(WorkPartitionTest, RealSourceLoopHasTwoIndependentWorkOwners) {
  ac::HardwareAnalysis analysis(*package);
  auto batch = analysis.getIndependentWorkSubtrees(top);
  ASSERT_TRUE(mlir::succeeded(batch));
  ASSERT_EQ(batch->size(), 2u);
  EXPECT_NE((*batch)[0], (*batch)[1]);
  EXPECT_TRUE(llvm::is_contained(*batch, left.getOperation()));
  EXPECT_TRUE(llvm::is_contained(*batch, right.getOperation()));
}
TEST_F(WorkPartitionTest, OwnTemporalFeedbackRemainsIndependent) {
  // Child q is an input-independent DFFE temporal cut. Connect its own data
  // pin to that q in the parent; no other occurrence must run to prepare it.
  left->setOperand(3, left.getResult(0));
  ASSERT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(*package)));
  ac::HardwareAnalysis analysis(*package);
  auto batch = analysis.getIndependentWorkSubtrees(top);
  ASSERT_TRUE(mlir::succeeded(batch));
  EXPECT_EQ(batch->size(), 2u);
}
TEST_F(WorkPartitionTest,
       CrossSiblingCombinationalInputCannotJoinTheSameBatch) {
  ac::HardwareAnalysis first(*package);
  auto child = mlir::dyn_cast<ac::ModuleOp>(first.resolveCallee(left));
  ASSERT_TRUE(child);
  // Construct a valid combinational output dependency using ordinary SSA.
  auto output = mlir::cast<ac::YieldOp>(child.getBody().front().back());
  output->setOperand(0, child.getBody().front().getArgument(3));
  right->setOperand(3, left.getResult(0));
  ASSERT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(*package)));
  ac::HardwareAnalysis analysis(*package);
  auto batch = analysis.getIndependentWorkSubtrees(top);
  ASSERT_TRUE(mlir::succeeded(batch));
  ASSERT_EQ(batch->size(), 1u);
  EXPECT_EQ(batch->front(), left.getOperation());
}
} // namespace
