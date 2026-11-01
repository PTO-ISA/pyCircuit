#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "gtest/gtest.h"

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
    // This API owns physical work partitioning, independently of source
    // admission. Both occurrences share a child definition with DFFE feedback.
    package = mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
#one = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "partition.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#one>
module {
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "Storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@Storage, "T">, !ac.type_param<@Storage, "T">) -> !ac.type_param<@Storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %data: !b1, %init: !b1):
    %q = "ac.instance"(%clk, %rst, %en, %next, %init) {instance_name = "state", callee = @Storage, parameters = [], type_arguments = [!b1], occurrence = {site = {definition = @Child, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %next = "ac.bits.binary"(%q, %data) {opcode = "xor"} : (!b1, !b1) -> !b1
    "ac.yield"(%q) : (!b1) -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "partition.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1, !b1) -> !b1, input_names = ["clk", "rst", "en", "data", "init"], output_names = ["q"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en_left: !b1, %en_right: !b1, %data_left: !b1, %data_right: !b1, %init_left: !b1, %init_right: !b1):
    %left = "ac.instance"(%clk, %rst, %en_left, %data_left, %init_left) {instance_name = "left", callee = @Child, parameters = [], type_arguments = [], occurrence = {site = {definition = @Top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %right = "ac.instance"(%clk, %rst, %en_right, %data_right, %init_right) {instance_name = "right", callee = @Child, parameters = [], type_arguments = [], occurrence = {site = {definition = @Top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    "ac.yield"(%left, %right) : (!b1, !b1) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "partition.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1) -> (!b1, !b1), input_names = ["clk", "rst", "en_left", "en_right", "data_left", "data_right", "init_left", "init_right"], output_names = ["left", "right"]} : () -> ()
  "ac.system"() {entry = {callee = @Top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)mlir", &context);
    ASSERT_TRUE(package);
    ASSERT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(*package)));
    top =
        mlir::dyn_cast_or_null<ac::ModuleOp>(mlir::SymbolTable::lookupSymbolIn(
            *package, "Top"));
    ASSERT_TRUE(top);
    auto outputs = mlir::cast<ac::YieldOp>(top.getBody().front().back());
    ASSERT_EQ(outputs.getValues().size(), 2u);
    left = outputs.getValues()[0].getDefiningOp<ac::InstanceOp>();
    right = outputs.getValues()[1].getDefiningOp<ac::InstanceOp>();
    ASSERT_TRUE(left);
    ASSERT_TRUE(right);
  }
};
TEST_F(WorkPartitionTest, SharedDefinitionHasTwoIndependentWorkOwners) {
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
