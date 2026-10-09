#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>
#include <vector>

namespace acir::ac {
namespace {

const char *boolDomain = "#ac.source_domain<{kind = \"bool\"}>";
const char *intDomain = "#ac.source_domain<{kind = \"integer\", lower = "
                        "#ac.math_int<-8>, upper = #ac.math_int<8>}>";

std::string owner(llvm::StringRef path = "top.py") {
  return "{package = \"inference\", path = \"" + path.str() + "\"}";
}
std::string occurrence(llvm::StringRef module, unsigned site = 0) {
  return "{site = {definition = @\"" + module.str() +
         "\", ast_path = [{kind = \"index\", value = " + std::to_string(site) +
         " : i64}]}, expansion = []}";
}
std::string diagnostic(llvm::StringRef path = "top.py") {
  return "{kind = \"assert\", message = \"checked\", location = {path = \"" +
         path.str() +
         "\", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column "
         "= 2 : i64}}";
}
struct Parameter {
  std::string name;
  std::string domain = boolDomain;
};
std::string formal(llvm::StringRef name, unsigned ordinal) {
  return "{kind = \"formal\", parameter = \"" + name.str() +
         "\", ordinal = " + std::to_string(ordinal) + " : i64}";
}
std::string parameters(const std::vector<Parameter> &params) {
  std::string result = "[";
  for (auto [index, parameter] : llvm::enumerate(params)) {
    if (index)
      result += ", ";
    result += "{name = \"" + parameter.name +
              "\", binding = \"positional_or_keyword\", domain = " +
              parameter.domain +
              ", formal_ref = " + formal(parameter.name, index) + "}";
  }
  return result + "]";
}
std::string reference(const Parameter &parameter) {
  return "!ac.ref<" + parameter.domain + ">";
}
std::string module(llvm::StringRef name, const std::vector<Parameter> &params,
                   llvm::StringRef body, llvm::StringRef path = "top.py",
                   llvm::StringRef effects = {}, bool declaration = false) {
  std::string signature = "(!ac.clock, !ac.bool";
  std::string block = "^bb0(%clock: !ac.clock, %reset: !ac.bool";
  for (const auto &parameter : params) {
    signature += ", " + reference(parameter);
    block += ", %" + parameter.name + ": " + reference(parameter);
  }
  signature += ") -> ()";
  block += "):\n";
  std::string attrs =
      "{sym_name = \"" + name.str() +
      "\", definition = {source_owner = " + owner(path) +
      ", qualified_name = \"" + name.str() +
      "\"}, domain = \"default\", parameters = " + parameters(params) +
      ", static_parameters = [], function_type = " + signature +
      (effects.empty() ? "" : ", effects = " + effects.str()) + "}";
  if (declaration)
    return "\"ac.src.module_decl\"() " + attrs + " : () -> ()\n";
  return "\"ac.src.module\"() ({\n" + block + body.str() +
         "\n\"ac.src.yield\"() : () -> ()\n}) " + attrs + " : () -> ()\n";
}
std::string unit(llvm::StringRef body, llvm::StringRef path = "top.py",
                 llvm::StringRef dependencies = {}, bool interface = false) {
  return "\"ac.src.unit\"() ({\n^unit:\n" + body.str() +
         "}) {source_owner = " + owner(path) + ", kind = \"" +
         (interface ? "interface" : "body") + "\", interfaces = [" +
         owner(path) + (dependencies.empty() ? "" : ", " + dependencies.str()) +
         "], exports = []} : () -> ()\n";
}
std::string package(llvm::StringRef units) {
  return "module {\n" + units.str() + "}\n";
}
std::string rule(llvm::StringRef moduleName,
                 const std::vector<Parameter> &params, llvm::StringRef body,
                 bool registered = true, llvm::StringRef ruleName = "step",
                 unsigned site = 0) {
  std::string operands = "%reset", types = "!ac.bool",
              block = "^bb0(%rr: !ac.bool";
  for (const auto &parameter : params) {
    operands += ", %" + parameter.name;
    types += ", " + reference(parameter);
    block += ", %r_" + parameter.name + ": " + reference(parameter);
  }
  block += "):\n";
  std::string result =
      "\"ac.src.rule\"(" + operands + ") ({\n" + block + body.str() +
      "\n\"ac.src.yield\"() : () -> ()\n}) {sym_name = \"" + ruleName.str() +
      "\", occurrence = " + occurrence(moduleName, site) + "} : (" + types +
      ") -> ()\n";
  if (registered)
    result += "\"ac.src.register\"() {rule = @\"" + ruleName.str() +
              "\", occurrence = " + occurrence(moduleName, site + 1) +
              "} : () -> ()\n";
  return result;
}
std::string effects(const std::vector<Parameter> &params,
                    const std::vector<std::pair<bool, bool>> &flags,
                    bool check = false, bool observe = false) {
  std::string result = "{refs = [";
  for (auto [index, parameter] : llvm::enumerate(params)) {
    if (index)
      result += ", ";
    result += "{formal_ref = " + formal(parameter.name, index) +
              ", read = " + (flags[index].first ? "true" : "false") +
              ", write = " + (flags[index].second ? "true" : "false") + "}";
  }
  return result + "], may_check = " + (check ? "true" : "false") +
         ", may_observe = " + (observe ? "true" : "false") + "}";
}
std::string readBool(llvm::StringRef moduleName, llvm::StringRef ref) {
  return "%value = \"ac.src.read\"(" + ref.str() +
         ") {occurrence = " + occurrence(moduleName, 12) + "} : (!ac.ref<" +
         boolDomain + ">) -> !ac.bool\n";
}
std::string proposeBool(llvm::StringRef moduleName, llvm::StringRef ref) {
  return "\"ac.src.propose\"(" + ref.str() +
         ", %value) {occurrence = " + occurrence(moduleName, 13) +
         "} : (!ac.ref<" + boolDomain + ">, !ac.bool) -> ()\n";
}
std::string instance(llvm::StringRef parent, llvm::StringRef callee,
                     llvm::StringRef actuals, llvm::StringRef names,
                     unsigned site = 20, unsigned count = 2) {
  std::string types = "!ac.clock, !ac.bool";
  for (unsigned index = 0; index < count; ++index)
    types += ", !ac.ref<" + std::string(boolDomain) + ">";
  return "\"ac.src.instance\"(%clock, %reset" +
         (actuals.empty() ? "" : ", " + actuals.str()) + ") {callee = @\"" +
         callee.str() + "\", argument_names = " + names.str() +
         ", occurrence = " + occurrence(parent, site) +
         ", static_actuals = []} : (" + types + ") -> ()\n";
}
std::string print(mlir::Operation *operation) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  operation->print(stream);
  return text;
}

class InferEffectsTest : public ::testing::Test {
protected:
  InferEffectsTest() { context.loadDialect<ACIRDialect>(); }
  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef text) {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    EXPECT_TRUE(file);
    return file;
  }
  bool infer(mlir::ModuleOp file, bool complete = false,
             bool registered = false) {
    mlir::ScopedDiagnosticHandler capture(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    mlir::PassManager manager(&context);
    if (registered) {
      acir::registerACIRPasses();
      if (mlir::failed(mlir::parsePassPipeline(
              complete ? "pycircuit-infer-effects{complete-closure=true}"
                       : "pycircuit-infer-effects",
              manager)))
        return false;
    } else
      manager.addPass(acir::createInferEffectsPass(complete));
    return mlir::succeeded(manager.run(file));
  }
  SrcModuleOp find(mlir::ModuleOp file, llvm::StringRef name) {
    SrcModuleOp found;
    file.walk([&](SrcModuleOp op) {
      if (op.getSymName() == name)
        found = op;
    });
    EXPECT_TRUE(found);
    return found;
  }
  void expectSummary(mlir::ModuleOp file, llvm::StringRef name,
                     const std::vector<Parameter> &params,
                     const std::vector<std::pair<bool, bool>> &flags,
                     bool check = false, bool observe = false) {
    auto op = find(file, name);
    ASSERT_TRUE(op);
    auto actual = op->getAttrOfType<mlir::DictionaryAttr>("effects");
    ASSERT_TRUE(actual);
    auto refs = actual.getAs<mlir::ArrayAttr>("refs");
    ASSERT_TRUE(refs);
    ASSERT_EQ(refs.size(), params.size());
    mlir::Builder builder(&context);
    for (auto [index, parameter] : llvm::enumerate(params)) {
      auto entry = mlir::cast<mlir::DictionaryAttr>(refs[index]);
      auto id = entry.getAs<mlir::DictionaryAttr>("formal_ref");
      EXPECT_EQ(id.getAs<mlir::IntegerAttr>("ordinal").getInt(), index);
      EXPECT_EQ(id.getAs<mlir::StringAttr>("parameter").getValue(),
                parameter.name);
      EXPECT_EQ(entry.getAs<mlir::BoolAttr>("read").getValue(),
                flags[index].first);
      EXPECT_EQ(entry.getAs<mlir::BoolAttr>("write").getValue(),
                flags[index].second);
    }
    EXPECT_EQ(actual.getAs<mlir::BoolAttr>("may_check").getValue(), check);
    EXPECT_EQ(actual.getAs<mlir::BoolAttr>("may_observe").getValue(), observe);
  }
  void rejectsUnchanged(mlir::ModuleOp file, bool complete) {
    std::string before = print(file);
    EXPECT_FALSE(infer(file, complete));
    EXPECT_EQ(print(file), before) << "failure must publish no partial effects";
  }
  mlir::MLIRContext context;
};

TEST_F(InferEffectsTest, ZeroRuleModulesRecomputeEmptyOrderedSummaries) {
  const std::vector<Parameter> params{{"one"}, {"two"}, {"three"}, {"four"}};
  auto file = parse(package(unit(module(
      "Top", params, "", "top.py",
      effects(params, {{true, true}, {true, true}, {true, true}, {true, true}},
              true, true)))));
  ASSERT_TRUE(file);
  ASSERT_TRUE(infer(*file));
  expectSummary(
      *file, "Top", params,
      {{false, false}, {false, false}, {false, false}, {false, false}});
}

TEST_F(InferEffectsTest,
       RegisteredRulesDeriveOrderedRefsAndIgnoreInactiveRules) {
  const std::vector<Parameter> params{{"unused"}, {"input"}, {"output"}};
  const std::string active =
      readBool("Top", "%r_input") + proposeBool("Top", "%r_output");
  const std::string inactive =
      "%literal = \"ac.src.int\"() {value = #ac.math_int<1>} : () -> "
      "!ac.math_int\n"
      "\"ac.src.observe\"(%literal) {kind = \"report\", spec = {name = "
      "\"inactive\"}, occurrence = " +
      occurrence("Top", 32) + "} : (!ac.math_int) -> ()";
  auto file = parse(package(unit(
      module("Top", params,
             rule("Top", params, active) +
                 rule("Top", params, inactive, false, "inactive", 30),
             "top.py",
             effects(params, {{true, true}, {false, false}, {false, false}},
                     true, true)))));
  ASSERT_TRUE(file);
  ASSERT_TRUE(infer(*file));
  expectSummary(*file, "Top", params,
                {{false, false}, {true, false}, {false, true}});
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  auto stable = print(*file);
  ASSERT_TRUE(infer(*file, false, true));
  EXPECT_EQ(print(*file), stable);
}

TEST_F(InferEffectsTest, BothIfArmsAndFaultsContributeConservatively) {
  const std::vector<Parameter> params{{"left"}, {"right"}};
  const std::string conditional =
      "%condition = \"ac.src.bool\"() {value = false} : () -> !ac.bool\n"
      "\"ac.src.if\"(%condition) ({\n" +
      readBool("Top", "%r_left") +
      "\"ac.src.assert\"(%value) {diagnostic = " + diagnostic() +
      ", occurrence = " + occurrence("Top", 40) +
      "} : (!ac.bool) -> ()\n\"ac.src.yield\"() : () -> ()\n}, {\n" +
      readBool("Top", "%r_right") +
      "\"ac.src.observe\"(%value) {kind = \"print\", spec = {sep = \" \", end "
      "= \"\\n\", items = [{kind = \"value\", ordinal = 0 : i32}]}, occurrence "
      "= " +
      occurrence("Top", 42) +
      "} : (!ac.bool) -> ()\n\"ac.src.yield\"() : () -> ()\n}) : (!ac.bool) -> "
      "()";
  auto file = parse(
      package(unit(module("Top", params, rule("Top", params, conditional)))));
  ASSERT_TRUE(file);
  ASSERT_TRUE(infer(*file));
  expectSummary(*file, "Top", params, {{true, false}, {true, false}}, true,
                true);
}

TEST_F(InferEffectsTest, IntegerProposalsAndUnusedArithmeticRequireChecks) {
  const std::vector<Parameter> params{{"state", intDomain}, {"unused"}};
  for (llvm::StringRef opcode : {"add", "floordiv", "shl"}) {
    const std::string body =
        "%literal = \"ac.src.int\"() {value = #ac.math_int<1>} : () -> "
        "!ac.math_int\n"
        "%unused_value = \"ac.src.binary\"(%literal, %literal) {opcode = \"" +
        opcode.str() +
        "\"} : (!ac.math_int, !ac.math_int) -> !ac.math_int\n"
        "\"ac.src.propose\"(%r_state, %literal) {occurrence = " +
        occurrence("Top", 50) + "} : (!ac.ref<" + intDomain +
        ">, !ac.math_int) -> ()";
    auto file =
        parse(package(unit(module("Top", params, rule("Top", params, body)))));
    ASSERT_TRUE(file);
    ASSERT_TRUE(infer(*file));
    expectSummary(*file, "Top", params, {{false, true}, {false, false}}, true);
  }
}

TEST_F(InferEffectsTest,
       BoundsAndUnusedArithmeticCarryConservativeCheckEffects) {
  const std::vector<Parameter> params{{"state"}};
  const std::string literals = "%integer = \"ac.src.int\"() {value = "
                               "#ac.math_int<1>} : () -> !ac.math_int\n";
  for (llvm::StringRef opcode : {"add", "floordiv", "mod", "shl", "shr"}) {
    auto file = parse(package(unit(module(
        "Top", params,
        rule(
            "Top", params,
            literals +
                "%unused = \"ac.src.binary\"(%integer, %integer) {opcode = \"" +
                opcode.str() +
                "\"} : (!ac.math_int, !ac.math_int) -> !ac.math_int")))));
    ASSERT_TRUE(file);
    ASSERT_TRUE(infer(*file));
    expectSummary(*file, "Top", params, {{false, false}}, opcode != "add");
  }
  auto bounded = parse(package(unit(
      module("Top", params,
             rule("Top", params,
                  literals + "%bound = \"ac.src.bound\"(%integer) {domain = " +
                      intDomain + ", diagnostic = " + diagnostic() +
                      ", occurrence = " + occurrence("Top", 52) +
                      "} : (!ac.math_int) -> !ac.math_int")))));
  ASSERT_TRUE(bounded);
  ASSERT_TRUE(infer(*bounded));
  expectSummary(*bounded, "Top", params, {{false, false}}, true);
}

TEST_F(InferEffectsTest, CalleeTopologyAndKeywordBindingFollowFormalIdentity) {
  const std::vector<Parameter> parent{{"unused"}, {"input"}, {"output"}};
  const std::vector<Parameter> child{{"in"}, {"out"}};
  const std::string leaf =
      module("Leaf", child,
             rule("Leaf", child,
                  readBool("Leaf", "%r_in") + proposeBool("Leaf", "%r_out")));
  const std::string top =
      module("Top", parent,
             instance("Top", "Leaf", "%output, %input", "[\"out\", \"in\"]"));
  auto file = parse(package(unit(top + leaf))); // Parent precedes callee.
  ASSERT_TRUE(file);
  ASSERT_TRUE(infer(*file));
  expectSummary(*file, "Top", parent,
                {{false, false}, {true, false}, {false, true}});
  expectSummary(*file, "Leaf", child, {{true, false}, {false, true}});
}

TEST_F(InferEffectsTest,
       InterfaceOnlySummariesAndOwnedActualsDoNotInventFormalEffects) {
  const std::vector<Parameter> parent{{"input"}, {"unused"}};
  const std::vector<Parameter> child{{"in"}, {"out"}};
  const std::string privateState =
      "%owned = \"ac.src.reg\"(%clock, %reset) {state_decl = {definition = "
      "{source_owner = " +
      owner() +
      ", qualified_name = \"Top\"}, declaration = " + occurrence("Top", 60) +
      ", element = []}, initial_value = false} : (!ac.clock, !ac.bool) -> "
      "!ac.ref<" +
      boolDomain + ">\n";
  auto file = parse(package(unit(
      module("Leaf", child, "", "leaf.py",
             effects(child, {{true, false}, {false, true}}, true, true), true) +
          module("Top", parent,
                 privateState +
                     instance("Top", "Leaf", "%input, %owned", "[unit, unit]")),
      "top.py", owner("leaf.py"))));
  ASSERT_TRUE(file);
  ASSERT_TRUE(infer(*file));
  expectSummary(*file, "Top", parent, {{true, false}, {false, false}}, true,
                true);
}

TEST_F(InferEffectsTest,
       CompleteClosureRecomputesBodiesAndRejectsStaleOrMissingEvidence) {
  const std::vector<Parameter> parent{{"input"}, {"output"}};
  const std::vector<Parameter> child{{"in"}, {"out"}};
  const std::string leafBody =
      module("Leaf", child,
             rule("Leaf", child,
                  readBool("Leaf", "%r_in") + proposeBool("Leaf", "%r_out")),
             "leaf.py");
  auto closure = [&](llvm::StringRef summary, bool includeBody) {
    return package(unit(module("Leaf", child, "", "leaf.py", summary, true) +
                            module("Top", parent,
                                   instance("Top", "Leaf", "%input, %output",
                                            "[unit, unit]")),
                        "top.py", owner("leaf.py")) +
                   (includeBody ? unit(leafBody, "leaf.py") : ""));
  };
  auto good =
      parse(closure(effects(child, {{true, false}, {false, true}}), true));
  ASSERT_TRUE(good);
  ASSERT_TRUE(infer(*good, true));
  expectSummary(*good, "Top", parent, {{true, false}, {false, true}});
  auto stale =
      parse(closure(effects(child, {{false, false}, {false, false}}), true));
  ASSERT_TRUE(stale);
  rejectsUnchanged(*stale, true);
  const std::vector<Parameter> renamed{{"in"}, {"renamed_output"}};
  auto wrongSignature = parse(package(
      unit(module("Leaf", renamed, "", "leaf.py",
                  effects(renamed, {{true, false}, {false, true}}), true) +
               module(
                   "Top", parent,
                   instance("Top", "Leaf", "%input, %output", "[unit, unit]")),
           "top.py", owner("leaf.py")) +
      unit(leafBody, "leaf.py")));
  ASSERT_TRUE(wrongSignature);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*wrongSignature)));
  rejectsUnchanged(*wrongSignature, true);
  auto missing =
      parse(closure(effects(child, {{true, false}, {false, true}}), false));
  ASSERT_TRUE(missing);
  rejectsUnchanged(*missing, true);
}

TEST_F(InferEffectsTest,
       InvalidInactiveRulesUnknownCalleesAndHierarchyCyclesAreTransactional) {
  const std::vector<Parameter> params{{"state"}};
  const std::string malformed = "%integer = \"ac.src.int\"() {value = "
                                "#ac.math_int<1>} : () -> !ac.math_int\n"
                                "\"ac.src.assert\"(%integer) {diagnostic = " +
                                diagnostic() +
                                ", occurrence = " + occurrence("Top", 70) +
                                "} : (!ac.math_int) -> ()";
  auto invalid = parse(package(
      unit(module("Top", params, rule("Top", params, malformed, false)))));
  ASSERT_TRUE(invalid);
  rejectsUnchanged(*invalid, false);
  auto unresolved = parse(package(unit(module(
      "Top", params, instance("Top", "Missing", "%state", "[unit]", 20, 1)))));
  ASSERT_TRUE(unresolved);
  rejectsUnchanged(*unresolved, false);
  auto cyclic = parse(package(unit(module(
      "Top", params, instance("Top", "Top", "%state", "[unit]", 20, 1)))));
  ASSERT_TRUE(cyclic);
  rejectsUnchanged(*cyclic, false);
}

} // namespace
} // namespace acir::ac
