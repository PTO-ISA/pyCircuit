#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>

namespace acir::ac {
namespace {

const char *prefix = R"ir(
#owner = {package = "values", path = "values.py"}
#definition = {source_owner = #owner, qualified_name = "Values"}
#occ = {site = {definition = @Values, ast_path = []}, expansion = []}
#bocc = {site = {definition = @Values, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#iocc = {site = {definition = @Values, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#uocc = {site = {definition = @Values, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}
#bstate = {definition = #definition, declaration = #bocc, element = []}
#istate = {definition = #definition, declaration = #iocc, element = []}
#ustate = {definition = #definition, declaration = #uocc, element = []}
#span = {path = "values.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#diagnostic = {kind = "assert", message = "value check", location = #span}
#range_diagnostic = {kind = "range", message = "range check", location = #span}
#bool = #ac.source_domain<{kind = "bool"}>
#two = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<2>}>
#pending = #ac.source_type_expr<{kind = "alias", name = ["Word"], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], location = #span}}>
#init = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = #uocc, location = #span}>
module {
"ac.src.unit"() ({
"ac.src.module"() ({
^bb0(%clock: !ac.clock, %reset: !ac.bool):
%bs = "ac.src.reg"(%clock, %reset) {state_decl = #bstate, initial_value = true} : (!ac.clock, !ac.bool) -> !ac.ref<#bool>
%is = "ac.src.reg"(%clock, %reset) {state_decl = #istate, initial_value = #ac.math_int<0>} : (!ac.clock, !ac.bool) -> !ac.ref<#two>
%us = "ac.src.reg"(%clock, %reset) {state_decl = #ustate, annotation = #pending, initializer = #init} : (!ac.clock, !ac.bool) -> !ac.unresolved
"ac.src.rule"(%reset, %bs, %is, %us) ({
^bb0(%rule_reset: !ac.bool, %br: !ac.ref<#bool>, %ir: !ac.ref<#two>, %ur: !ac.unresolved):
%b = "ac.src.bool"() {value = true} : () -> !ac.bool
%i = "ac.src.int"() {value = #ac.math_int<18446744073709551619>} : () -> !ac.math_int
%rb = "ac.src.read"(%br) {occurrence = #occ} : (!ac.ref<#bool>) -> !ac.bool
%ri = "ac.src.read"(%ir) {occurrence = #occ} : (!ac.ref<#two>) -> !ac.math_int
%u = "ac.src.read"(%ur) {occurrence = #occ} : (!ac.unresolved) -> !ac.unresolved
)ir";
const char *suffix = R"ir(
"ac.src.yield"() : () -> ()
}) {sym_name = "tick", occurrence = #occ} : (!ac.bool, !ac.ref<#bool>, !ac.ref<#two>, !ac.unresolved) -> ()
"ac.src.register"() {rule = @tick, occurrence = #bocc} : () -> ()
"ac.src.yield"() : () -> ()
}) {sym_name = "Values", definition = #definition, domain = "default", parameters = [], static_parameters = [], function_type = (!ac.clock, !ac.bool) -> ()} : () -> ()
}) {source_owner = #owner, kind = "body", interfaces = [#owner], exports = []} : () -> ()
}
)ir";

std::string design(llvm::StringRef body) {
  return std::string(prefix) + body.str() + suffix;
}
std::string unary(llvm::StringRef opcode, llvm::StringRef operand,
                  llvm::StringRef type,
                  llvm::StringRef result = "!ac.math_int") {
  return "%result = \"ac.src.unary\"(" + operand.str() + ") {opcode = \"" +
         opcode.str() + "\"} : (" + type.str() + ") -> " + result.str();
}
std::string binary(llvm::StringRef opcode, llvm::StringRef operands = "%ri, %i",
                   llvm::StringRef types = "!ac.math_int, !ac.math_int") {
  return "%result = \"ac.src.binary\"(" + operands.str() + ") {opcode = \"" +
         opcode.str() + "\"} : (" + types.str() + ") -> !ac.math_int";
}
std::string branch(llvm::StringRef left, llvm::StringRef leftType,
                   llvm::StringRef right, llvm::StringRef rightType,
                   llvm::StringRef resultType) {
  return "%joined = \"ac.src.if\"(%b) ({\n\"ac.src.yield\"(" + left.str() +
         ") : (" + leftType.str() + ") -> ()\n}, {\n\"ac.src.yield\"(" +
         right.str() + ") : (" + rightType.str() +
         ") -> ()\n}) : (!ac.bool) -> " + resultType.str();
}

class SourceValueOpsTest : public ::testing::Test {
protected:
  SourceValueOpsTest() { context.loadDialect<ACIRDialect>(); }
  void expect(llvm::StringRef text, bool valid, bool roundtrip = false) {
    SCOPED_TRACE(text.str());
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    ASSERT_TRUE(file) << "negative must reach operation verifier: "
                      << diagnostics;
    EXPECT_EQ(mlir::succeeded(mlir::verify(*file)), valid) << diagnostics;
    if (!valid)
      EXPECT_FALSE(diagnostics.empty());
    if (valid && roundtrip) {
      std::string printed;
      llvm::raw_string_ostream stream(printed);
      file->print(stream);
      EXPECT_TRUE(mlir::parseSourceString<mlir::ModuleOp>(printed, &context))
          << diagnostics;
    }
  }
  void rejectsDuringParse(llvm::StringRef text) {
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    EXPECT_FALSE(mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false)));
    EXPECT_FALSE(diagnostics.empty());
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

TEST_F(SourceValueOpsTest, LiteralsAndCurrentReadsPreserveSourceKinds) {
  expect(design(""), true, true);
  rejectsDuringParse(replace(design(""), "value = true} : () -> !ac.bool",
                             "value = 1 : i8} : () -> !ac.bool"));
  rejectsDuringParse(
      replace(design(""), "#ac.math_int<18446744073709551619>", "true"));
  expect(replace(design(""), "(!ac.ref<#bool>) -> !ac.bool",
                 "(!ac.ref<#bool>) -> !ac.math_int"),
         false);
  expect(replace(design(""), "(!ac.ref<#two>) -> !ac.math_int",
                 "(!ac.ref<#two>) -> !ac.bool"),
         false);
  expect(design("%wrong = \"ac.src.read\"(%b) {occurrence = #occ} : (!ac.bool) "
                "-> !ac.unresolved"),
         false);
  expect(replace(design(""), "definition = @Values, ast_path = []",
                 "definition = @Other, ast_path = []"),
         false);
}

TEST_F(SourceValueOpsTest, IntegerOperatorsAcceptOnlyTheirDeclaredValueKinds) {
  for (llvm::StringRef opcode :
       {"add", "sub", "mul", "floordiv", "mod", "and_bits", "or_bits",
        "xor_bits", "shl", "shr"}) {
    expect(design(binary(opcode)), true);
    expect(design(binary(opcode, "%u, %ri", "!ac.unresolved, !ac.math_int")),
           true);
    expect(design(binary(opcode, "%b, %u", "!ac.bool, !ac.unresolved")), false);
  }
  expect(design(binary("unknown")), false);
  expect(design(binary("add", "%ur, %i", "!ac.unresolved, !ac.math_int")),
         false);
  expect(design(binary("add", "%ir, %i", "!ac.ref<#two>, !ac.math_int")),
         false);
  expect(design(unary("neg", "%ri", "!ac.math_int")), true);
  expect(design(unary("invert", "%u", "!ac.unresolved")), true);
  expect(design(unary("not", "%rb", "!ac.bool", "!ac.bool")), true);
  expect(design(unary("not", "%u", "!ac.unresolved", "!ac.bool")), true);
  expect(design(unary("not", "%i", "!ac.math_int", "!ac.bool")), false);
  expect(design(unary("neg", "%b", "!ac.bool")), false);
  expect(design(unary("unknown", "%i", "!ac.math_int")), false);
}

TEST_F(SourceValueOpsTest, ComparisonsAndIntegerCastsUseKnownKinds) {
  for (llvm::StringRef predicate : {"eq", "ne", "lt", "le", "gt", "ge"}) {
    const std::string compare =
        "%result = \"ac.src.compare\"(%ri, %u) {predicate = \"" +
        predicate.str() + "\"} : (!ac.math_int, !ac.unresolved) -> !ac.bool";
    expect(design(compare), true);
    expect(design(replace(replace(compare, "%ri, %u", "%b, %u"),
                          "!ac.math_int, !ac.unresolved",
                          "!ac.bool, !ac.unresolved")),
           false);
  }
  expect(design("%result = \"ac.src.compare\"(%ri, %i) {predicate = "
                "\"unknown\"} : (!ac.math_int, !ac.math_int) -> !ac.bool"),
         false);
  for (auto [value, type] :
       {std::pair{"%b", "!ac.bool"}, std::pair{"%i", "!ac.math_int"},
        std::pair{"%u", "!ac.unresolved"}})
    expect(design("%result = \"ac.src.int_cast\"(" + std::string(value) +
                  ") : (" + type + ") -> !ac.math_int"),
           true);
  expect(design("%result = \"ac.src.int_cast\"(%ur) : (!ac.unresolved) -> "
                "!ac.math_int"),
         false);
}

TEST_F(SourceValueOpsTest, IfJoinsSupportValuesAndPendingValuesButNeverRefs) {
  expect(design(branch("%b", "!ac.bool", "%rb", "!ac.bool", "!ac.bool")), true,
         true);
  expect(design(branch("%i", "!ac.math_int", "%ri", "!ac.math_int",
                       "!ac.math_int")),
         true);
  expect(design(branch("%u", "!ac.unresolved", "%b", "!ac.bool",
                       "!ac.unresolved")),
         true);
  expect(design(branch("%i", "!ac.math_int", "%u", "!ac.unresolved",
                       "!ac.unresolved")),
         true);
  expect(
      design(branch("%i", "!ac.math_int", "%b", "!ac.bool", "!ac.unresolved")),
      false);
  expect(design(branch("%br", "!ac.ref<#bool>", "%br", "!ac.ref<#bool>",
                       "!ac.ref<#bool>")),
         false);
  expect(design(branch("%ur", "!ac.unresolved", "%ur", "!ac.unresolved",
                       "!ac.unresolved")),
         false);
  expect(
      design(branch("%rule_reset", "!ac.bool", "%b", "!ac.bool", "!ac.bool")),
      false);
  const std::string empty =
      "\"ac.src.if\"(%b) ({\"ac.src.yield\"() : () -> ()}, {\"ac.src.yield\"() "
      ": () -> ()}) : (!ac.bool) -> ()";
  expect(design(empty), true);
  expect(design(replace(replace(empty, "%b)", "%i)"), "(!ac.bool) -> ()",
                        "(!ac.math_int) -> ()")),
         false);
  expect(design(replace(empty, "}, {\"ac.src.yield\"() : () -> ()", "")),
         false);
  expect(design(replace(empty, "({\"ac.src.yield\"",
                        "({^branch(%arg: !ac.bool):\"ac.src.yield\"")),
         false);
}

TEST_F(SourceValueOpsTest, ProposalsAndBoundsFollowReferenceDomainKinds) {
  const std::string propose = "\"ac.src.propose\"(%br, %b) {occurrence = #occ} "
                              ": (!ac.ref<#bool>, !ac.bool) -> ()";
  expect(design(propose), true);
  expect(design("\"ac.src.propose\"(%ir, %ri) {occurrence = #occ} : "
                "(!ac.ref<#two>, !ac.math_int) -> ()"),
         true);
  expect(design("\"ac.src.propose\"(%ur, %u) {occurrence = #occ} : "
                "(!ac.unresolved, !ac.unresolved) -> ()"),
         true);
  expect(design("\"ac.src.propose\"(%br, %u) {occurrence = #occ} : "
                "(!ac.ref<#bool>, !ac.unresolved) -> ()"),
         true);
  expect(design("\"ac.src.propose\"(%br, %i) {occurrence = #occ} : "
                "(!ac.ref<#bool>, !ac.math_int) -> ()"),
         false);
  expect(design("\"ac.src.propose\"(%ir, %b) {occurrence = #occ} : "
                "(!ac.ref<#two>, !ac.bool) -> ()"),
         false);
  expect(design("\"ac.src.propose\"(%u, %i) {occurrence = #occ} : "
                "(!ac.unresolved, !ac.math_int) -> ()"),
         false);
  const std::string bound =
      "%bound = \"ac.src.bound\"(%ri) {domain = #two, diagnostic = "
      "#range_diagnostic, occurrence = #occ} : (!ac.math_int) -> !ac.math_int";
  expect(design(bound), true, true);
  expect(design(replace(bound, "domain = #two", "domain = #pending")), true);
  expect(design(replace(bound, "domain = #two", "domain = #bool")), false);
  expect(design(replace(bound, "domain = #two",
                        "domain = #ac.source_type_expr<{kind = \"bool\"}>")),
         false);
  expect(design(replace(replace(bound, "%ri)", "%rb)"), "(!ac.math_int)",
                        "(!ac.bool)")),
         false);
}

TEST_F(SourceValueOpsTest, PendingBoundDomainsUseTheirActualDeclarationOwner) {
  auto syntaxSpan = [&](llvm::StringRef path) {
    return "{path = \"" + path.str() +
           "\", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, "
           "end_column = 8 : i64}";
  };
  auto alias = [&](llvm::StringRef path) {
    return "#ac.source_type_expr<{kind = \"alias\", name = [\"Word\"], site = "
           "{ast_path = [{kind = \"field\", name = \"body\"}], location = " +
           syntaxSpan(path) + "}}>";
  };
  auto expression = [&](llvm::StringRef path) {
    return "#ac.static_expr<{kind = \"reference\", ref = {kind = \"lexical\", "
           "name = [\"constants\", \"LIMIT\"], site = "
           "{ast_path = [{kind = \"field\", name = \"body\"}], location = " +
           syntaxSpan(path) +
           "}}, origin = #occ, location = " + syntaxSpan(path) + "}>";
  };
  auto integer = [&](llvm::StringRef lower, llvm::StringRef upper) {
    return "#ac.source_type_expr<{kind = \"integer\", lower = " +
           expression(lower) + ", upper = " + expression(upper) + "}>";
  };
  auto bound = [&](llvm::StringRef domain) {
    return design("%bounded = \"ac.src.bound\"(%ri) {domain = " + domain.str() +
                  ", diagnostic = #range_diagnostic, occurrence = #occ} : "
                  "(!ac.math_int) -> !ac.math_int");
  };
  expect(bound(alias("values.py")), true);
  expect(bound(integer("values.py", "values.py")), true);
  auto rejectsOwner = [&](llvm::StringRef text) {
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    ASSERT_TRUE(file) << "owner oracle must reach verification: "
                      << diagnostics;
    EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
    EXPECT_NE(diagnostics.find("SourceOwner"), std::string::npos) << diagnostics;
  };
  rejectsOwner(bound(alias("foreign.py")));
  rejectsOwner(bound(integer("foreign.py", "values.py")));
  rejectsOwner(bound(integer("values.py", "foreign.py")));
}

TEST_F(SourceValueOpsTest, ResetControlsAndReferencesCannotMasqueradeAsData) {
  expect(design(unary("not", "%rule_reset", "!ac.bool", "!ac.bool")), false);
  expect(design("\"ac.src.assert\"(%rule_reset) {diagnostic = #diagnostic, "
                "occurrence = #occ} : (!ac.bool) -> ()"),
         false);
  expect(design("\"ac.src.assert\"(%ir) {diagnostic = #diagnostic, occurrence "
                "= #occ} : (!ac.ref<#two>) -> ()"),
         false);
  expect(design(replace(branch("%b", "!ac.bool", "%rb", "!ac.bool", "!ac.bool"),
                        "%b)", "%rule_reset)")),
         false);
  expect(design("\"ac.src.propose\"(%br, %rule_reset) {occurrence = #occ} : "
                "(!ac.ref<#bool>, !ac.bool) -> ()"),
         false);
}

TEST_F(SourceValueOpsTest,
       OrdinaryBooleanCapturesRemainDataButResetAliasesDoNot) {
  std::string text =
      design("\"ac.src.assert\"(%outer) {diagnostic = #diagnostic, "
             "occurrence = #occ} : (!ac.bool) -> ()");
  text = replace(text, "%bs =",
                 "%outside = \"ac.src.bool\"() {value = false} : () -> "
                 "!ac.bool\n%bs =");
  text = replace(text, "\"ac.src.rule\"(%reset, %bs, %is, %us)",
                 "\"ac.src.rule\"(%reset, %bs, %is, %us, %outside)");
  text = replace(
      text, "%ur: !ac.unresolved):", "%ur: !ac.unresolved, %outer: !ac.bool):");
  text = replace(
      text, "(!ac.bool, !ac.ref<#bool>, !ac.ref<#two>, !ac.unresolved) -> ()",
      "(!ac.bool, !ac.ref<#bool>, !ac.ref<#two>, !ac.unresolved, !ac.bool) -> "
      "()");
  expect(text, true);
  expect(replace(text, "%reset, %bs, %is, %us, %outside)",
                 "%reset, %bs, %is, %us, %reset)"),
         false);
}

TEST_F(SourceValueOpsTest, SharedDagsReclassifyAfterMutationsAndRejectCycles) {
  // Each equation uses the preceding SSA twice. Size is stress input, never an
  // admission limit or a timed benchmark. Semantics stay identical at all
  // sizes.
  for (unsigned depth : {8u, 64u}) {
    std::string body;
    std::string previous = "%i";
    for (unsigned index = 0; index < depth; ++index) {
      const std::string next = "%shared" + std::to_string(index);
      body += next + " = \"ac.src.binary\"(" + previous + ", " + previous +
              ") {opcode = \"add\"} : (!ac.math_int, !ac.math_int) -> "
              "!ac.math_int\n";
      previous = next;
    }
    auto file = mlir::parseSourceString<mlir::ModuleOp>(design(body), &context);
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
    llvm::SmallVector<SrcBinaryOp> equations;
    SrcBoolOp boolean;
    SrcRuleOp sourceRule;
    file->walk([&](SrcBinaryOp operation) { equations.push_back(operation); });
    file->walk([&](SrcBoolOp operation) { boolean = operation; });
    file->walk([&](SrcRuleOp operation) { sourceRule = operation; });
    ASSERT_FALSE(equations.empty());
    ASSERT_TRUE(boolean && sourceRule);
    auto first = equations.front();
    auto consumer = equations.back();
    auto original = first->getOperand(0);
    ASSERT_TRUE(mlir::succeeded(consumer.verify()));
    mlir::ScopedDiagnosticHandler suppress(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    // No cached verdict survives a later IR mutation. Consumer verification
    // must follow actual producers, even when its own operands remain M-typed.
    first->setOperand(0, boolean.getResult());
    EXPECT_TRUE(mlir::failed(consumer.verify()));
    first->setOperand(0, original);
    EXPECT_TRUE(mlir::succeeded(consumer.verify()));
    first->setOperand(0, sourceRule.getBody().front().getArgument(0));
    EXPECT_TRUE(mlir::failed(consumer.verify()));
    first->setOperand(0, original);
    EXPECT_TRUE(mlir::succeeded(consumer.verify()));
    // Direct local verification exercises active-cycle detection before whole
    // source SSA dominance verification would reject this mutated loop.
    first->setOperand(0, consumer.getResult());
    EXPECT_TRUE(mlir::failed(consumer.verify()));
    first->setOperand(0, original);
    EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  }
}

TEST_F(SourceValueOpsTest,
       ChecksAndObservationsHaveClosedSpecsAndSourceMetadata) {
  const std::string check = "\"ac.src.assert\"(%b) {diagnostic = #diagnostic, "
                            "occurrence = #occ} : (!ac.bool) -> ()";
  expect(design(check), true, true);
  expect(design(replace(check, "diagnostic = #diagnostic",
                        "diagnostic = {kind = \"assert\", message = \"value "
                        "check\", location = #span, extra = unit}")),
         false);
  expect(design(replace(check, "occurrence = #occ",
                        "occurrence = {site = {definition = @Other, ast_path = "
                        "[]}, expansion = []}")),
         false);
  expect(replace(design(check), "path = \"values.py\", line = 1",
                 "path = \"other.py\", line = 1"),
         false);
  expect(design(replace(check, "%b)", "%rb)")), true);
  expect(design("\"ac.src.assert\"(%u) {diagnostic = #diagnostic, occurrence = "
                "#occ} : (!ac.unresolved) -> ()"),
         true);
  expect(design("\"ac.src.assert\"(%i) {diagnostic = #diagnostic, occurrence = "
                "#occ} : (!ac.math_int) -> ()"),
         false);
  expect(replace(design(check), "kind = \"assert\", message",
                 "kind = \"\", message"),
         false);
  expect(design(replace(check, "diagnostic = #diagnostic",
                        "diagnostic = {kind = \"assert\", message = \"value "
                        "check\", location = "
                        "{path = \"values.py\", line = 1 : i64, column = 1 : "
                        "i64, end_line = 1 : i64, end_column = 0 : i64}}")),
         false);
  const std::string print =
      "\"ac.src.observe\"(%b, %ri) {kind = \"print\", spec = {sep = \" \", end "
      "= \"\\n\", items = [{kind = \"literal\", text = \"values\"}, {kind = "
      "\"value\", ordinal = 0 : i32}, {kind = \"value\", ordinal = 1 : i32}]}, "
      "occurrence = #occ} : (!ac.bool, !ac.math_int) -> ()";
  expect(design(print), true, true);
  expect(design(replace(print, "spec = {sep", "spec = {extra = unit, sep")),
         false);
  expect(design(replace(print, "%b, %ri", "%rule_reset, %ri")), false);
  const std::string log = replace(
      replace(print, "kind = \"print\"", "kind = \"log\""),
      "sep = \" \", end = \"\\n\"", "level = \"info\", event = \"values\"");
  expect(design(log), true);
  expect(design(replace(log, "level = \"info\"", "level = \"unknown\"")),
         false);
  expect(design(replace(print, "ordinal = 1 : i32", "ordinal = 0 : i32")),
         false);
  expect(design(replace(print, "kind = \"print\"", "kind = \"trace\"")), false);
  const std::string report =
      "\"ac.src.observe\"(%ri) {kind = \"report\", spec = {name = "
      "\"counter\"}, occurrence = #occ} : (!ac.math_int) -> ()";
  expect(design(report), true);
  expect(design(replace(replace(report, "%ri)", "%b)"), "(!ac.math_int) -> ()",
                        "(!ac.bool) -> ()")),
         false);
}

} // namespace
} // namespace acir::ac
