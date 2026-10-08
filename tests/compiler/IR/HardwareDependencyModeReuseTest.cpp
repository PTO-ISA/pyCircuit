#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <iterator>
#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

class HardwareDependencyModeReuseTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp root;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() {
    return FileLineColLoc::get(&context, "dependency.py", 1, 1);
  }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  ac::StaticExprAttr literal(unsigned value) {
    const auto text =
        "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
        "value = #ac.math_int<" +
        std::to_string(value) +
        ">}, origin = {site = {definition = @Root, ast_path = []}, "
        "expansion = []}, location = {path = \"dependency.py\", line = 1 : "
        "i64, "
        "column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  Type bits(unsigned width) {
    return ac::BitsType::get(&context, literal(width));
  }
  Operation *op(StringRef name, ValueRange inputs = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  void dag(unsigned width, unsigned vertices) {
    package = mlir::ModuleOp::create(loc());
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr("Root")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("dependency.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr({b.getStringAttr("input")})),
         f("output_names", b.getArrayAttr({b.getStringAttr("output")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType({bits(width)}, {bits(width)})))});
    state.addRegion();
    root = cast<ac::ModuleOp>(b.create(state));
    auto *body = new Block;
    root.getBody().push_back(body);
    const Value input = body->addArgument(bits(width), loc());
    b.setInsertionPointToEnd(body);
    // A shallow fanout DAG avoids making stack depth the capacity boundary.
    // Its computations are deliberately unobserved and must still be checked.
    for (unsigned i = 0; i < vertices; ++i)
      op(ac::BitsUnaryOp::getOperationName(), {input}, {bits(width)},
         {f("opcode", b.getStringAttr("not"))});
    op(ac::YieldOp::getOperationName(), {input});
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("entry", d({f("callee", FlatSymbolRefAttr::get(&context, "Root")),
                      f("parameters", b.getArrayAttr({})),
                      f("type_arguments", b.getArrayAttr({}))})),
        f("domain", b.getStringAttr("default"))});
  }
  size_t vertices() {
    const auto operations = root.getBody().front().getOps<ac::BitsUnaryOp>();
    return std::distance(operations.begin(), operations.end());
  }
  void deadCycle() {
    auto &body = root.getBody().front();
    b.setInsertionPoint(body.getTerminator());
    auto *first = op(ac::BitsUnaryOp::getOperationName(), {body.getArgument(0)},
                     {body.getArgument(0).getType()},
                     {f("opcode", b.getStringAttr("not"))});
    auto *second = op(ac::BitsUnaryOp::getOperationName(),
                      {first->getResult(0)}, {body.getArgument(0).getType()},
                      {f("opcode", b.getStringAttr("not"))});
    first->setOperand(0, second->getResult(0));
  }
  FailureOr<ac::HardwareSourceCheckPlan>
  plan(ac::HardwareSourceCheckLimits limits = {}) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream stream(diagnostics);
      error.print(stream);
      return success();
    });
    return ac::HardwareAnalysis(*package).getSourceCheckPlan(limits);
  }
  void memories(bool cycle) {
    // Each read depends only on its independent raddr in the ordinary graph.
    // Only operational all-input dependencies expose the cross-memory cycle.
    const std::string header = R"mlir(
#one = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#four = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<4>}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#eight = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#seven = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<7>}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#aw = #ac.static_expr<{kind = "reference", ref = {kind = "parameter", owner = @Memory, name = "ADDR_WIDTH"}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#pw = #ac.static_expr<{kind = "type_width", type = !ac.type_param<@Memory, "T">, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#rounded = #ac.static_expr<{kind = "binary", operator = "add", lhs = #pw, rhs = #seven, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#lanes = #ac.static_expr<{kind = "binary", operator = "floordiv", lhs = #rounded, rhs = #eight, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, location = {path = "dependency.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#one>
!b4 = !ac.bits<#four>
!b8 = !ac.bits<#eight>
!address = !ac.bits<#aw>
!strobe = !ac.bits<#lanes>
module {
  "ac.module.import"() {sym_name = "Memory", source_owner = {package = "gfsim", path = "byte_mem.py"}, parameters = [{name = "ADDR_WIDTH", type = !ac.math_int}, {name = "DEPTH", type = !ac.math_int}], type_parameters = ["T"], function_type = (!b1, !b1, !address, !b1, !address, !ac.type_param<@Memory, "T">, !strobe) -> !ac.type_param<@Memory, "T">, input_names = ["clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"], output_names = ["rdata"], primitive_kind = "byte_mem", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = [{port = 2 : i64, path = []}]}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %addr_a: !b4, %addr_b: !b4, %valid: !b1, %strobe: !b1, %seed: !b8):
)mlir";
    auto instance = [](StringRef name, StringRef address, StringRef data,
                       unsigned site) {
      return "    %" + name.str() + " = \"ac.instance\"(%clk, %rst, %" +
             address.str() + ", %valid, %" + address.str() + ", %" +
             data.str() + ", %strobe) {instance_name = \"" + name.str() +
             "\", callee = @Memory, parameters = [#four, #seven], "
             "type_arguments = [!b8], occurrence = {site = {definition = "
             "@Root, ast_path = [{kind = \"index\", value = " +
             std::to_string(site) +
             " : i64}]}, expansion = []}} : (!b1, !b1, !b4, !b1, !b4, !b8, "
             "!b1) -> !b8\n";
    };
    const std::string tail = R"mlir(
    "ac.yield"(%b) : (!b8) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "dependency.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b4, !b4, !b1, !b1, !b8) -> !b8, input_names = ["clk", "rst", "addr_a", "addr_b", "valid", "strobe", "seed"], output_names = ["result"]} : () -> ()
  "ac.system"() {entry = {callee = @Root, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)mlir";
    package = parseSourceString<mlir::ModuleOp>(
        header + instance("a", "addr_a", cycle ? "b" : "seed", 0) +
            instance("b", "addr_b", "a", 1) + tail,
        &context);
    ASSERT_TRUE(package);
    root = *package->getOps<ac::ModuleOp>().begin();
  }
};

TEST_F(HardwareDependencyModeReuseTest, LargeNoMemoryDAGRetainsEveryVertex) {
  dag(37, 45000);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  ASSERT_EQ(vertices(), 45000u);
  auto result = plan();
  ASSERT_TRUE(succeeded(result)) << diagnostics;
  EXPECT_EQ(vertices(), 45000u);
  EXPECT_TRUE(result->owners.empty());
  EXPECT_TRUE(result->checks.empty());
  EXPECT_TRUE(result->commits.empty());
  EXPECT_TRUE(
      succeeded(ac::HardwareAnalysis(*package).verifySourceCheckPlan(*result)));
  EXPECT_EQ(vertices(), 45000u);
}

TEST_F(HardwareDependencyModeReuseTest, ExplicitLowBudgetStillRejectsFullDAG) {
  dag(5, 45000);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  EXPECT_TRUE(failed(plan({32})));
  EXPECT_NE(diagnostics.find("analysis work budget"), std::string::npos);
  EXPECT_EQ(vertices(), 45000u);
}

TEST_F(HardwareDependencyModeReuseTest, UnobservedNoMemoryCycleStillRejects) {
  dag(13, 64);
  deadCycle();
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  EXPECT_TRUE(failed(plan()));
  EXPECT_NE(
      diagnostics.find("combinational cycle in hardware field dependencies"),
      std::string::npos)
      << diagnostics;
  EXPECT_EQ(vertices(), 66u);
}

TEST_F(HardwareDependencyModeReuseTest,
       CrossMemoryOperationalCycleStillRejects) {
  memories(true);
  ASSERT_TRUE(package);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  EXPECT_TRUE(failed(plan()));
  EXPECT_NE(diagnostics.find("single-Work sampling cycle"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(
      std::distance(root.getBody().front().getOps<ac::InstanceOp>().begin(),
                    root.getBody().front().getOps<ac::InstanceOp>().end()),
      2);
}

TEST_F(HardwareDependencyModeReuseTest, MemoryDAGRemainsAccepted) {
  memories(false);
  ASSERT_TRUE(package);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  auto result = plan();
  ASSERT_TRUE(succeeded(result)) << diagnostics;
  EXPECT_TRUE(
      succeeded(ac::HardwareAnalysis(*package).verifySourceCheckPlan(*result)));
}
} // namespace
