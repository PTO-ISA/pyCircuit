#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

std::string design(llvm::StringRef body) {
  return R"ir(
#owner = {package = "closure", path = "closure.py"}
#def = {source_owner = #owner, qualified_name = "Root"}
#occ = {site = {definition = @Root, ast_path = []}, expansion = []}
#state = {definition = #def, declaration = #occ, element = []}
#word = {kind = "integer", storage = i7, lower = #ac.math_int<0>,
         upper = #ac.math_int<128>, interpretation = "unsigned"}
#span = {path = "closure.py", line = 1 : i64, column = 1 : i64,
         end_line = 1 : i64, end_column = 2 : i64}
module {
  "ac.hw.module"() ({
  ^bb0(%clk: !ac.clock, %rst: i1, %permit: !ac.commit_permit):
    %on = arith.constant true
    %off = arith.constant false
    %live = arith.xori %rst, %on : i1
)ir" + body.str() +
         R"ir(
  }) {sym_name = "Root", source_owner = #owner, definition = #def,
      domain = "default", function_type = (!ac.clock, i1, !ac.commit_permit) -> !ac.error,
      ports = [{name = "clock", type = !ac.clock, role = "clock"},
               {name = "reset", type = i1, role = "reset"},
               {name = "permit", type = !ac.commit_permit, role = "permit"},
               {name = "error", type = !ac.error, role = "error"}]} : () -> ()
  "ac.hw.system"() ({
  ^bb0(%clk: !ac.clock, %rst: i1):
    %error = "ac.hw.instance"(%clk, %rst, %permit)
      {callee = @Root, occurrence = #occ, actual_ref_bindings = []}
      : (!ac.clock, i1, !ac.commit_permit) -> !ac.error
    %permit = "ac.hw.commit_permit"(%error) : (!ac.error) -> !ac.commit_permit
    "ac.hw.output"() : () -> ()
  }) {sym_name = "Test", source_owner = #owner, domain = "default"} : () -> ()
}
)ir";
}

const char *check = R"ir(
%c = "ac.hw.check"(%live, %on) {source_owner = #owner, occurrence = #occ,
     diagnostic = {kind = "assert", message = "valid", location = #span}}
     : (i1, i1) -> !ac.error
)ir";
const char *output = R"ir("ac.hw.output"(%e) : (!ac.error) -> ())ir";
const char *reg = R"ir(
%q = "ac.seq.reg"(%d, %on, %clk, %rst, %permit)
  {state_decl = #state, logical_domain = #word, initial_bits = 7 : i7}
  : (i7, i1, !ac.clock, i1, !ac.commit_permit) -> i7
%one = arith.constant 1 : i7
%d = arith.addi %q, %one : i7
)ir";
const char *observation = R"ir(
"ac.hw.observe"(%live, %permit, %q) {kind = "report", spec = {name = "count"},
  domains = [#word], source_owner = #owner, occurrence = #occ}
  : (i1, !ac.commit_permit, i7) -> ()
)ir";

class HardwareClosureTest : public ::testing::Test {
protected:
  HardwareClosureTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }

  void expect(llvm::StringRef text, bool valid, llvm::StringRef reason = {},
              bool sharedPipeline = false, bool locallyValid = true) {
    auto module = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    ASSERT_TRUE(module);
    if (locallyValid)
      ASSERT_TRUE(mlir::succeeded(mlir::verify(*module)))
          << "closure oracle requires locally valid hardware IR";
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    mlir::PassManager manager(&context);
    if (sharedPipeline)
      acir::buildHardwareVerificationPipeline(manager);
    else
      manager.addPass(acir::createVerifyHardwarePass());
    EXPECT_EQ(mlir::succeeded(manager.run(*module)), valid) << diagnostics;
    if (!valid)
      EXPECT_NE(diagnostics.find(reason.str()), std::string::npos)
          << diagnostics;
  }

  std::string replace(std::string text, llvm::StringRef old,
                      llvm::StringRef replacement) {
    auto offset = text.find(old.str());
    EXPECT_NE(offset, std::string::npos);
    if (offset != std::string::npos)
      text.replace(offset, old.size(), replacement.str());
    return text;
  }
  mlir::MLIRContext context;
};

TEST_F(HardwareClosureTest, RootPermitFeedbackAndSequentialFeedbackAreLegal) {
  expect(
      design(std::string("%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output),
      true);
  expect(design(std::string(reg) + check + observation +
                "%e = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n" +
                output),
         true);
  expect(
      design(std::string(check) + "\"ac.hw.output\"(%c) : (!ac.error) -> ()"),
      true);
}

TEST_F(HardwareClosureTest, SharedPipelineAcceptsTheSameHardwareContract) {
  const std::string text = design(
      std::string(reg) + check + observation +
      "%e = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n" + output);
  expect(text, true, {}, true);
  expect(replace(text, "\"ac.hw.check\"(%live", "\"ac.hw.check\"(%on"), false,
         "ordinary demand suppressed under reset", true);
}

TEST_F(HardwareClosureTest, ChildErrorsMustReachTheRootErrorClosure) {
  std::string leaf =
      design(std::string("%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output);
  auto begin = leaf.find("  \"ac.hw.module\"");
  auto end = leaf.find("  \"ac.hw.system\"", begin);
  ASSERT_NE(begin, std::string::npos);
  ASSERT_NE(end, std::string::npos);
  std::string child = replace(leaf.substr(begin, end - begin),
                              "sym_name = \"Root\"", "sym_name = \"Child\"");
  child = replace(child, "definition = #def", "definition = #childDef");
  std::string text = design(
      "%child = \"ac.hw.instance\"(%clk, %rst, %permit) "
      "{callee = @Child, occurrence = #occ, actual_ref_bindings = []} "
      ": (!ac.clock, i1, !ac.commit_permit) -> !ac.error\n"
      "%e = \"ac.hw.error_reduce\"(%child) : (!ac.error) -> !ac.error\n" +
      std::string(output));
  text.insert(
      text.find("module {"),
      "#childDef = {source_owner = #owner, qualified_name = \"Child\"}\n");
  text.insert(text.find("  \"ac.hw.system\""), child);
  expect(text, true);
  expect(replace(text, "%e = \"ac.hw.error_reduce\"(%child) : (!ac.error)",
                 "%e = \"ac.hw.error_reduce\"() : ()"),
         false, "missing from module output reduction closure");
  // A dead child-local combinational cycle is rejected through hierarchy
  // traversal.
  const std::string cycle = "%left = arith.addi %right, %one : i7\n"
                            "%right = arith.subi %left, %one : i7\n"
                            "%one = arith.constant 1 : i7\n";
  const auto childStart =
      text.find("  \"ac.hw.module\"", text.find("  \"ac.hw.module\"") + 1);
  ASSERT_NE(childStart, std::string::npos);
  text.insert(text.find("    %on =", childStart), cycle);
  expect(text, false, "combinational cycle");
}

TEST_F(HardwareClosureTest, StatefulSiblingOccurrenceExpansionDefinesIdentity) {
  std::string leaf =
      design(std::string(reg) +
             "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n" + output);
  auto begin = leaf.find("  \"ac.hw.module\"");
  auto end = leaf.find("  \"ac.hw.system\"", begin);
  ASSERT_NE(begin, std::string::npos);
  ASSERT_NE(end, std::string::npos);
  std::string child = replace(leaf.substr(begin, end - begin),
                              "sym_name = \"Root\"", "sym_name = \"Child\"");
  child = replace(child, "definition = #def", "definition = #childDef");
  child = replace(child, "state_decl = #state", "state_decl = #childState");
  std::string other =
      replace(child, "sym_name = \"Child\"", "sym_name = \"Other\"");
  other = replace(other, "definition = #childDef", "definition = #otherDef");
  other =
      replace(other, "state_decl = #childState", "state_decl = #otherState");
  for (auto [left, right] : {std::pair{5, 71}, std::pair{903, 27}}) {
    auto occurrence = [](int ordinal) {
      return "{site = {definition = @Root, ast_path = []}, expansion = ["
             "{kind = \"iteration\", site = {definition = @Root, ast_path = "
             "[]}, "
             "ordinal = " +
             std::to_string(ordinal) +
             " : i64, "
             "value = {kind = \"integer\", value = #ac.math_int<3>}}]}\n";
    };
    std::string text = design(
        "%a = \"ac.hw.instance\"(%clk, %rst, %permit) "
        "{callee = @Child, occurrence = #firstOcc, actual_ref_bindings = []} "
        ": (!ac.clock, i1, !ac.commit_permit) -> !ac.error\n"
        "%b = \"ac.hw.instance\"(%clk, %rst, %permit) "
        "{callee = @Child, occurrence = #secondOcc, actual_ref_bindings = []} "
        ": (!ac.clock, i1, !ac.commit_permit) -> !ac.error\n"
        "%e = \"ac.hw.error_reduce\"(%a, %b) : (!ac.error, !ac.error) -> "
        "!ac.error\n" +
        std::string(output));
    text.insert(
        text.find("module {"),
        "#childDef = {source_owner = #owner, qualified_name = \"Child\"}\n"
        "#otherDef = {source_owner = #owner, qualified_name = \"Other\"}\n"
        "#childState = {definition = #childDef, declaration = {site = "
        "{definition = @Child, ast_path = []}, expansion = []}, element = []}\n"
        "#otherState = {definition = #otherDef, declaration = {site = "
        "{definition = @Other, ast_path = []}, expansion = []}, element = []}\n"
        "#firstOcc = " +
            occurrence(left) + "#secondOcc = " + occurrence(right));
    text.insert(text.find("  \"ac.hw.system\""), child + other);
    expect(text, true);
    expect(replace(text, "occurrence = #secondOcc", "occurrence = #firstOcc"),
           false, "instance occurrence must be unique", false, false);
    text = replace(
        text, "%b = \"ac.hw.instance\"(%clk, %rst, %permit) {callee = @Child",
        "%b = \"ac.hw.instance\"(%clk, %rst, %permit) {callee = @Other");
    expect(text, true);
    expect(replace(text, "occurrence = #secondOcc", "occurrence = #firstOcc"),
           false, "instance occurrence must be unique", false, false);
  }
}

TEST_F(HardwareClosureTest, ChecksMustReachOutputExactlyOnce) {
  // Shared empty reducers contain no error leaves, so they remain legal.
  expect(
      design(
          std::string(
              "%empty = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
              "%a = \"ac.hw.error_reduce\"(%empty) : (!ac.error) -> !ac.error\n"
              "%b = \"ac.hw.error_reduce\"(%empty) : (!ac.error) -> !ac.error\n"
              "%e = \"ac.hw.error_reduce\"(%a, %b) : (!ac.error, !ac.error) -> "
              "!ac.error\n") +
          output),
      true);
  const std::string missing =
      design(std::string(check) +
             "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n" + output);
  expect(missing, false, "missing from module output reduction closure");
  expect(design(std::string(check) +
                "%a = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n"
                "%b = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n"
                "%e = \"ac.hw.error_reduce\"(%a, %b) : (!ac.error, !ac.error) "
                "-> !ac.error\n" +
                output),
         false, "duplicated in reduction closure");
  expect(design(std::string(
                    "%unused = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
                    "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
                output),
         false, "missing from module output reduction closure");
}

TEST_F(HardwareClosureTest, ResetSuppressesChecksAndObservations) {
  const std::string text = design(
      std::string(reg) + check + observation +
      "%e = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n" + output);
  expect(replace(text, "\"ac.hw.check\"(%live", "\"ac.hw.check\"(%on"), false,
         "ordinary demand suppressed under reset");
  expect(replace(text, "\"ac.hw.observe\"(%live", "\"ac.hw.observe\"(%on"),
         false, "ordinary demand suppressed under reset");
  // Annihilation must prove suppression even when another operand is old Q.
  std::string annihilation =
      replace(text, "%c =",
              "%pred = arith.cmpi ne, %q, %one : i7\n"
              "%demand = arith.andi %pred, %live : i1\n%c =");
  expect(
      replace(annihilation, "\"ac.hw.check\"(%live", "\"ac.hw.check\"(%demand"),
      true);
}

TEST_F(HardwareClosureTest, DeadCombinationalCyclesAreRejected) {
  expect(
      design(std::string("%a = arith.addi %b, %one : i7\n"
                         "%b = arith.xori %a, %one : i7\n"
                         "%one = arith.constant 1 : i7\n"
                         "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output),
      false, "combinational cycle");
  expect(
      design(
          std::string(
              "%e = \"ac.hw.error_reduce\"(%e) : (!ac.error) -> !ac.error\n") +
          output),
      false, "cycle in error reduction DAG");
}

TEST_F(HardwareClosureTest, PermitCannotBeDecodedIntoDataOrCheckConditions) {
  expect(design(std::string(
                    "%decoded = builtin.unrealized_conversion_cast %permit "
                    ": !ac.commit_permit to i1\n") +
                replace(check, "%live, %on", "%live, %decoded") +
                "%e = \"ac.hw.error_reduce\"(%c) : (!ac.error) -> !ac.error\n" +
                output),
         false, "outside closed hardware legality");
  // Arithmetic legality is checked on dead equations as well as output cones.
  expect(
      design(std::string("%one = arith.constant 1 : i7\n"
                         "%poison = arith.addi %one, %one overflow<nsw> : i7\n"
                         "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output),
      false, "free of poison flags");
}

TEST_F(HardwareClosureTest, ClosedPrimitiveTypesAndAttributesAreRequired) {
  expect(
      design(std::string("%wide = arith.constant 1 : i65\n"
                         "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output),
      false, "unsupported hardware result type");
  expect(design(std::string(
                    "%v = \"arith.constant\"() {value = 1 : i7, forged = true} "
                    ": () -> i7\n"
                    "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
                output),
         false, "unsupported arithmetic semantic attribute");
  expect(
      design(std::string("%one = arith.constant 1 : i7\n"
                         "%unknown = arith.divui %one, %one : i7\n"
                         "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n") +
             output),
      false, "outside closed hardware legality");
}

} // namespace
} // namespace acir::ac
