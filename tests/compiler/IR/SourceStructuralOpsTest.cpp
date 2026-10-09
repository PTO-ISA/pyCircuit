#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>

namespace acir::ac {
namespace {

const char *metadata = R"ir(
#owner = {package = "structure", path = "structure.py"}
#dep = {package = "library", path = "leaf.py"}
#definition = {source_owner = #owner, qualified_name = "Model"}
#foreign = {source_owner = #dep, qualified_name = "Leaf"}
#occ = {site = {definition = @Model, ast_path = []}, expansion = []}
#regocc = {site = {definition = @Model, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}
#instanceocc = {site = {definition = @Model, ast_path = [{kind = "index", value = 11 : i64}]}, expansion = []}
#state = {definition = #definition, declaration = #regocc, element = []}
#span = {path = "structure.py", line = 1 : i64, column = 1 : i64,
         end_line = 1 : i64, end_column = 2 : i64}
#site = {ast_path = [], location = #span}
#bool = #ac.source_domain<{kind = "bool"}>
#two = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<2>}>
#annotation = #ac.source_type_expr<{kind = "bool"}>
#initializer = #ac.static_expr<{kind = "literal", value = {kind = "bool", value = true},
                  origin = #regocc, location = #span}>
#formal = {kind = "formal", parameter = "state", ordinal = 0 : i64}
#parameter = {name = "state", binding = "positional_or_keyword", domain = #bool, formal_ref = #formal}
#pending = {name = "state", binding = "positional_or_keyword", domain = #annotation, formal_ref = #formal}
#effects = {refs = [{formal_ref = #formal, read = true, write = false}], may_check = false, may_observe = false}
#empty_effects = {refs = [], may_check = false, may_observe = false}
)ir";

std::string unit(llvm::StringRef body, llvm::StringRef kind = "body") {
  return std::string(metadata) + "module {\n\"ac.src.unit\"() ({\n^unit:\n" +
         body.str() + "\n}) {source_owner = #owner, kind = \"" + kind.str() +
         "\", interfaces = [#owner], exports = []} : () -> ()\n}";
}

std::string sourceModule(llvm::StringRef body, bool parameter = false,
                         bool pending = false) {
  std::string reference = pending ? "!ac.unresolved" : "!ac.ref<#bool>";
  return "\"ac.src.module\"() ({\n^bb0(%clock: !ac.clock, %reset: !ac.bool" +
         (parameter ? ", %state: " + reference : "") + "):\n" + body.str() +
         "\n\"ac.src.yield\"() : () -> ()\n}) {sym_name = \"Model\", "
         "definition = #definition, domain = \"default\", static_parameters = "
         "[], "
         "parameters = " +
         (parameter ? (pending ? "[#pending]" : "[#parameter]") : "[]") +
         ", function_type = (!ac.clock, !ac.bool" +
         (parameter ? ", " + reference : "") + ") -> ()} : () -> ()\n";
}

const char *declaration = R"ir(
"ac.src.module_decl"() {sym_name = "Leaf", definition = #foreign,
  domain = "default", static_parameters = [], parameters = [#parameter],
  function_type = (!ac.clock, !ac.bool, !ac.ref<#bool>) -> (), effects = #effects} : () -> ()
)ir";

const char *rule = R"ir(
"ac.src.rule"(%reset, %state) ({
^bb0(%rule_reset: !ac.bool, %captured: !ac.ref<#bool>):
  "ac.src.yield"() : () -> ()
}) {sym_name = "tick", occurrence = #occ} : (!ac.bool, !ac.ref<#bool>) -> ()
"ac.src.register"() {rule = @tick, occurrence = #regocc} : () -> ()
)ir";

const char *resolvedReg = R"ir(
%state = "ac.src.reg"(%clock, %reset) {state_decl = #state, initial_value = true}
  : (!ac.clock, !ac.bool) -> !ac.ref<#bool>
)ir";

const char *pendingReg = R"ir(
%state = "ac.src.reg"(%clock, %reset)
  {state_decl = #state, annotation = #annotation, initializer = #initializer}
  : (!ac.clock, !ac.bool) -> !ac.unresolved
)ir";

std::string instance(llvm::StringRef callee = "@Leaf",
                     llvm::StringRef names = "[unit]") {
  return "\"ac.src.instance\"(%clock, %reset, %state) {callee = " +
         callee.str() + ", argument_names = " + names.str() +
         ", occurrence = #instanceocc, static_actuals = []} "
         ": (!ac.clock, !ac.bool, !ac.ref<#bool>) -> ()\n";
}

std::string ownerSpan(llvm::StringRef path) {
  return "{path = \"" + path.str() +
         "\", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column "
         "= 9 : i64}";
}
std::string ownerAlias(llvm::StringRef path) {
  return "#ac.source_type_expr<{kind = \"alias\", name = [\"Word\"], site = "
         "{ast_path = [{kind = \"field\", name = \"body\"}], location = " +
         ownerSpan(path) + "}}>";
}
std::string ownerLiteral(llvm::StringRef path, llvm::StringRef value = "0") {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
         "value = #ac.math_int<" +
         value.str() + ">}, origin = #occ, location = " + ownerSpan(path) +
         "}>";
}
std::string ownerCall(llvm::StringRef path) {
  return "#ac.static_expr<{kind = \"call\", callee = [\"factory\", \"value\"], "
         "callee_site = "
         "{ast_path = [{kind = \"field\", name = \"body\"}], location = " +
         ownerSpan(path) + "}, arguments = [{kind = \"positional\", value = " +
         ownerLiteral(path) +
         "}], origin = #occ, location = " + ownerSpan(path) + "}>";
}

class SourceStructuralOpsTest : public ::testing::Test {
protected:
  SourceStructuralOpsTest() { context.loadDialect<ACIRDialect>(); }

  void expect(llvm::StringRef source, bool valid, bool roundtrip = false) {
    SCOPED_TRACE(source.str());
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostic);
      d.print(stream);
      return mlir::success();
    });
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        source, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    ASSERT_TRUE(file) << "structural negative must reach verifier: "
                      << diagnostic;
    EXPECT_EQ(mlir::succeeded(mlir::verify(*file)), valid) << diagnostic;
    if (!valid)
      EXPECT_FALSE(diagnostic.empty());
    if (valid && roundtrip) {
      std::string text;
      llvm::raw_string_ostream stream(text);
      file->print(stream);
      auto copy = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
      ASSERT_TRUE(copy) << diagnostic;
    }
  }

  void rejectsOwnerPath(llvm::StringRef source) {
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        source, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    ASSERT_TRUE(file) << "owner oracle must reach verification: "
                      << diagnostics;
    EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
    EXPECT_NE(diagnostics.find("SourceOwner"), std::string::npos) << diagnostics;
  }
  std::string replace(std::string text, llvm::StringRef before,
                      llvm::StringRef after) {
    auto offset = text.find(before.str());
    EXPECT_NE(offset, std::string::npos);
    if (offset != std::string::npos)
      text.replace(offset, before.size(), after.str());
    return text;
  }
  mlir::MLIRContext context;
};

TEST_F(SourceStructuralOpsTest, EmptyUnitsAndZeroRuleModulesAreLegal) {
  expect(unit(declaration), false);
  expect(unit(sourceModule(""), "interface"), false);
  expect(unit(""), false);
  expect(unit("", "interface"), true, true);
  expect(unit("", "interface"), true);
  expect(unit(sourceModule("")), true, true);
  expect(replace(unit("", "interface"), "kind = \"interface\"",
                 "kind = \"unknown\""),
         false);
  expect(replace(unit("", "interface"), "interfaces = [#owner]",
                 "interfaces = []"),
         false);
  expect(replace(unit("", "interface"), "interfaces = [#owner]",
                 "interfaces = [#dep, #owner]"),
         false);
  expect(replace(unit("", "interface"), "interfaces = [#owner]",
                 "interfaces = [#owner, #owner]"),
         false);
  expect(replace(unit("", "interface"), "interfaces = [#owner]",
                 "interfaces = [#owner, #dep]"),
         true);
}

TEST_F(SourceStructuralOpsTest,
       ModuleSignaturesDistinguishPendingAndResolvedRefs) {
  expect(unit(sourceModule("", true)), true, true);
  expect(unit(sourceModule("", true, true)), true, true);
  std::string mixed = sourceModule("", true);
  mixed = replace(mixed, "%state: !ac.ref<#bool>",
                  "%state: !ac.ref<#bool>, %other: !ac.unresolved");
  mixed = replace(mixed, "parameters = [#parameter]",
                  "parameters = [#parameter, #pendingOther]");
  mixed = replace(mixed, "!ac.ref<#bool>) -> ()",
                  "!ac.ref<#bool>, !ac.unresolved) -> ()");
  mixed = unit(mixed);
  mixed.insert(
      mixed.find("module {"),
      "#pendingOther = {name = \"other\", binding = \"positional_or_keyword\", "
      "domain = #annotation, formal_ref = {kind = \"formal\", parameter = "
      "\"other\", ordinal = 1 : i64}}\n");
  expect(mixed, true, true);
  const std::string text = unit(sourceModule("", true));
  expect(replace(text, "%reset: !ac.bool", "%reset: i1"), false);
  expect(replace(text, "%state: !ac.ref<#bool>", "%state: !ac.ref<#two>"),
         false);
  expect(replace(text, "%state: !ac.ref<#bool>", "%state: !ac.unresolved"),
         false);
  expect(replace(text, ", %state: !ac.ref<#bool>", ""), false);
  expect(replace(text, "static_parameters = []", "static_parameters = [unit]"),
         false);
  expect(replace(text, "domain = \"default\"", "domain = \"other\""), false);
  expect(
      replace(text, "qualified_name = \"Model\"", "qualified_name = \"Other\""),
      false);
}

TEST_F(SourceStructuralOpsTest,
       RuleCapturesMatchSignatureAndLocalRegistration) {
  const std::string text = unit(sourceModule(rule, true));
  expect(text, true, true);
  // Captures are actual lexical SSA; another in-scope same-domain ref is a
  // legal different design, not a metadata forgery of Python source meaning.
  const std::string secondReg =
      replace(replace(resolvedReg, "%state =", "%other ="),
              "state_decl = #state", "state_decl = #otherstate");
  std::string differentCapture =
      unit(sourceModule(std::string(resolvedReg) + secondReg +
                        replace(rule, "(%reset, %state)", "(%reset, %other)")));
  differentCapture.insert(differentCapture.find("module {"),
                          "#otherstate = {definition = #definition, "
                          "declaration = #instanceocc, element = []}\n");
  expect(differentCapture, true);
  expect(unit(sourceModule(std::string(rule) + resolvedReg)), false);

  expect(replace(text, "%captured: !ac.ref<#bool>", "%captured: !ac.ref<#two>"),
         false);
  expect(replace(text, "rule = @tick", "rule = @missing"), false);
  expect(replace(text, "rule = @tick", "rule = @Model"), false);
  expect(replace(text, "sym_name = \"tick\", occurrence = #occ",
                 "sym_name = \"tick\", occurrence = #instanceocc"),
         true);
  expect(replace(text, "definition = @Model, ast_path = []",
                 "definition = @Other, ast_path = []"),
         false);
  // Isolated rules cannot refer to their parent's SSA without an explicit
  // capture.
  expect(replace(text, "\"ac.src.yield\"() : () -> ()",
                 "\"ac.src.yield\"(%reset) : (!ac.bool) -> ()"),
         false);
  expect(replace(text, "):\n  \"ac.src.yield\"",
                 "):\n^second:\n  \"ac.src.yield\""),
         false);
}

TEST_F(SourceStructuralOpsTest, RegistersHaveExclusivePendingOrResolvedForms) {
  expect(unit(sourceModule(resolvedReg)), true, true);
  expect(unit(sourceModule(pendingReg)), true, true);
  const std::string resolved = unit(sourceModule(resolvedReg));
  expect(replace(resolved, "initial_value = true",
                 "initial_value = #ac.math_int<1>"),
         false);
  expect(replace(resolved, "initial_value = true",
                 "initial_value = true, annotation = #annotation"),
         false);
  expect(replace(resolved, "initial_value = true",
                 "initial_value = true, initializer = #initializer"),
         false);
  expect(replace(resolved, "initial_value = true", "annotation = #annotation"),
         false);
  expect(replace(unit(sourceModule(pendingReg)), "initializer = #initializer",
                 "initializer = #initializer, initial_value = true"),
         false);
  expect(unit(sourceModule(std::string(resolvedReg) +
                           replace(resolvedReg, "%state =", "%duplicate ="))),
         false);
}

TEST_F(SourceStructuralOpsTest,
       PendingRegSyntaxMustBelongToItsDeclarationOwner) {
  auto reg = [&](llvm::StringRef annotation, llvm::StringRef initializer) {
    return unit(sourceModule(replace(
        replace(pendingReg, "annotation = #annotation",
                "annotation = " + annotation.str()),
        "initializer = #initializer", "initializer = " + initializer.str())));
  };
  expect(reg(ownerAlias("structure.py"), ownerCall("structure.py")), true);
  rejectsOwnerPath(reg(ownerAlias("foreign.py"), ownerCall("structure.py")));
  rejectsOwnerPath(reg(ownerAlias("structure.py"), ownerCall("foreign.py")));
  rejectsOwnerPath(reg(ownerAlias("structure.py"), ownerLiteral("foreign.py")));
  // Outer tree belongs to the declaration; a foreign nested argument has
  // internally self-consistent syntax but still belongs to another file.
  const std::string nested =
      replace(ownerCall("structure.py"), ownerLiteral("structure.py"),
              ownerCall("foreign.py"));
  rejectsOwnerPath(reg(ownerAlias("structure.py"), nested));
}

TEST_F(SourceStructuralOpsTest,
       PendingParameterAliasAndIntegerBoundsKeepOwnerContext) {
  auto withDomain = [&](llvm::StringRef annotation) {
    return replace(unit(sourceModule("", true, true)),
                   "domain = #annotation, formal_ref",
                   "domain = " + annotation.str() + ", formal_ref");
  };
  expect(withDomain(ownerAlias("structure.py")), true);
  rejectsOwnerPath(withDomain(ownerAlias("foreign.py")));
  expect(withDomain("#ac.source_type_expr<{kind = \"bool\"}>"), true);
  auto integer = [&](llvm::StringRef lower, llvm::StringRef upper) {
    return "#ac.source_type_expr<{kind = \"integer\", lower = " + lower.str() +
           ", upper = " + upper.str() + "}>";
  };
  expect(
      withDomain(integer(ownerLiteral("structure.py"),
                         ownerLiteral("structure.py", "18446744073709551617"))),
      true);
  rejectsOwnerPath(withDomain(
      integer(ownerLiteral("foreign.py"),
              ownerLiteral("structure.py", "18446744073709551617"))));
  rejectsOwnerPath(withDomain(
      integer(ownerLiteral("structure.py"), ownerCall("foreign.py"))));
}

TEST_F(SourceStructuralOpsTest, ControlsMustForwardTheOwningModuleSSA) {
  const std::string text =
      unit(std::string(declaration) +
           sourceModule(std::string(resolvedReg) + rule + instance()));
  auto file = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
  ASSERT_TRUE(file);
  SrcRegOp reg;
  SrcRuleOp sourceRule;
  SrcInstanceOp child;
  file->walk([&](SrcRegOp candidate) { reg = candidate; });
  file->walk([&](SrcRuleOp candidate) { sourceRule = candidate; });
  file->walk([&](SrcInstanceOp candidate) { child = candidate; });
  ASSERT_TRUE(reg && sourceRule && child);
  mlir::Value owningReset = reg->getOperand(1);
  mlir::Value otherReset = sourceRule.getBody().front().getArgument(0);
  ASSERT_EQ(owningReset.getType(), otherReset.getType());
  mlir::ScopedDiagnosticHandler suppress(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  // Check each local identity law directly; no unimplemented bool producer is
  // admitted just to manufacture an alternative source control.
  reg->setOperand(1, otherReset);
  EXPECT_TRUE(mlir::failed(reg.verify()));
  reg->setOperand(1, owningReset);
  child->setOperand(1, otherReset);
  EXPECT_TRUE(mlir::failed(child.verify()));
  child->setOperand(1, owningReset);
  sourceRule->setOperand(0, otherReset);
  EXPECT_TRUE(mlir::failed(sourceRule.verify()));
  sourceRule->setOperand(0, owningReset);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
}

TEST_F(SourceStructuralOpsTest,
       IntegerRegisterInitializationUsesHalfOpenDomains) {
  auto design = [&](llvm::StringRef lower, llvm::StringRef upper,
                    llvm::StringRef init) {
    std::string domain =
        "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<" +
        lower.str() + ">, upper = #ac.math_int<" + upper.str() + ">}>";
    return unit(
        sourceModule("%state = \"ac.src.reg\"(%clock, %reset) "
                     "{state_decl = #state, initial_value = #ac.math_int<" +
                     init.str() +
                     ">} "
                     ": (!ac.clock, !ac.bool) -> !ac.ref<" +
                     domain + ">"));
  };
  expect(design("-71", "93", "-71"), true);
  expect(design("-71", "93", "92"), true);
  expect(design("-71", "93", "-72"), false);
  expect(design("-71", "93", "93"), false);
  expect(design("18446744073709551616", "18446744073709551620",
                "18446744073709551618"),
         true);
}

TEST_F(SourceStructuralOpsTest,
       ImportedDeclarationIsLocalShapeNotProviderAuthority) {
  expect(unit(std::string(declaration) + sourceModule("")), true, true);
  expect(unit(declaration, "interface"), true);
  expect(replace(unit(std::string(declaration) + sourceModule("")),
                 "effects = #effects", "effects = #empty_effects"),
         false);
  expect(replace(unit(std::string(declaration) + sourceModule("")),
                 "!ac.ref<#bool>", "!ac.unresolved"),
         false);
  expect(replace(unit(std::string(declaration) + sourceModule("")),
                 "qualified_name = \"Leaf\"", "qualified_name = \"Other\""),
         false);
  // This fixture verifies a foreign declaration signature only. Supplying a
  // provider/interface closure is a separate publication verification
  // obligation.
}

TEST_F(SourceStructuralOpsTest, InstancesBindArgumentsAndOwnUniqueOccurrences) {
  const std::string text =
      unit(std::string(declaration) + sourceModule(instance(), true));
  expect(text, true, true);
  expect(unit(std::string(declaration) +
              sourceModule(instance("@Leaf", "[\"state\"]"), true)),
         true);
  expect(unit(sourceModule(instance("[\"library\", \"Leaf\"]"), true)), true);
  expect(
      unit(std::string(declaration) +
           sourceModule(replace(instance(), "!ac.ref<#bool>", "!ac.unresolved"),
                        true, true)),
      true);
  std::string pendingMismatch =
      unit(std::string(declaration) +
           sourceModule(replace(instance(), "!ac.ref<#bool>", "!ac.unresolved"),
                        true, true));
  pendingMismatch =
      replace(pendingMismatch, "domain = #bool, formal_ref = #formal}",
              "domain = #two, formal_ref = #formal}");
  pendingMismatch =
      replace(pendingMismatch, "!ac.ref<#bool>) -> ()", "!ac.ref<#two>) -> ()");
  expect(pendingMismatch, false);
  // Keep the parent signature internally consistent while changing its domain;
  // this reaches instance domain matching rather than signature mismatch.
  std::string integerParent = sourceModule(instance(), true);
  while (integerParent.find("!ac.ref<#bool>") != std::string::npos)
    integerParent = replace(integerParent, "!ac.ref<#bool>", "!ac.ref<#two>");
  integerParent = replace(integerParent, "parameters = [#parameter]",
                          "parameters = [#integerParam]");
  std::string mismatch = unit(std::string(declaration) + integerParent);
  mismatch.insert(
      mismatch.find("module {"),
      "#integerParam = {name = \"state\", binding = \"positional_or_keyword\", "
      "domain = #two, formal_ref = #formal}\n");
  expect(mismatch, false);
  expect(replace(text, "callee = @Leaf", "callee = @Missing"), false);
  expect(replace(text, "argument_names = [unit]", "argument_names = []"),
         false);
  expect(
      replace(text, "argument_names = [unit]", "argument_names = [\"wrong\"]"),
      false);
  expect(replace(text, "static_actuals = []", "static_actuals = [unit]"),
         false);
  expect(unit(std::string(declaration) +
              sourceModule(instance() + instance(), true)),
         false);
  expect(replace(text, "!ac.ref<#bool>", "!ac.ref<#two>"), false);
}

TEST_F(SourceStructuralOpsTest, WritableAliasLawUsesOrderedDeclaredRefEffects) {
  const std::string second = R"ir(
#secondref = {kind = "formal", parameter = "other", ordinal = 1 : i64}
#secondparam = {name = "other", binding = "positional_or_keyword", domain = #bool, formal_ref = #secondref}
)ir";
  for (auto [writeFirst, writeSecond] :
       {std::pair{false, false}, std::pair{true, false}, std::pair{false, true},
        std::pair{true, true}}) {
    std::string callee = replace(declaration, "parameters = [#parameter]",
                                 "parameters = [#parameter, #secondparam]");
    callee = replace(callee, "!ac.ref<#bool>) -> ()",
                     "!ac.ref<#bool>, !ac.ref<#bool>) -> ()");
    callee = replace(
        callee, "effects = #effects",
        "effects = {refs = [{formal_ref = #formal, read = true, write = " +
            std::string(writeFirst ? "true" : "false") +
            "}, {formal_ref = #secondref, read = true, write = " +
            (writeSecond ? "true" : "false") +
            "}], may_check = false, may_observe = false}");
    std::string call = replace(instance(), "%clock, %reset, %state)",
                               "%clock, %reset, %state, %state)");
    call = replace(call, "argument_names = [unit]",
                   "argument_names = [unit, unit]");
    call = replace(call, "!ac.ref<#bool>) -> ()",
                   "!ac.ref<#bool>, !ac.ref<#bool>) -> ()");
    std::string text = unit(callee + sourceModule(call, true));
    text.insert(text.find("module {"), second);
    expect(text, !(writeFirst && writeSecond));
    expect(replace(text, "argument_names = [unit, unit]",
                   "argument_names = [\"state\", \"state\"]"),
           false);
    expect(replace(text, "argument_names = [unit, unit]",
                   "argument_names = [\"other\", unit]"),
           false);
  }
}

TEST_F(SourceStructuralOpsTest,
       ImportAndExportMetadataRetainNamespaceSiteShape) {
  const std::string request =
      "\"ac.src.import\"() {local_name = \"Leaf\", site = #site, "
      "request = {module = \"library\", level = 0 : i64, name = \"Leaf\"}} : "
      "() -> ()";
  expect(unit(request + sourceModule("")), true, true);
  expect(replace(unit(request + sourceModule("")), "level = 0 : i64",
                 "level = -1 : i64"),
         false);
  expect(
      replace(unit(request + sourceModule("")), "site = #site", "site = #occ"),
      false);
  expect(replace(unit(request + sourceModule("")), "local_name = \"Leaf\"",
                 "local_name = \"bad-name\""),
         false);
  expect(replace(unit(request + sourceModule("")),
                 "level = 0 : i64, name = \"Leaf\"}",
                 "level = 0 : i64, name = \"Leaf\", extra = unit}"),
         false);
  const std::string requestFields =
      "request = {module = \"library\", level = 0 : i64, name = \"Leaf\"}";
  const std::string namespaceAuthority =
      "authority = {kind = \"namespace\", source = #dep}";
  auto withDependency = [&](std::string text) {
    return replace(text, "interfaces = [#owner]",
                   "interfaces = [#owner, #dep]");
  };
  expect(replace(unit(request + sourceModule("")), ", " + requestFields, ""),
         false);
  expect(withDependency(replace(unit(request + sourceModule("")), requestFields,
                                requestFields + ", " + namespaceAuthority)),
         false);
  const std::string resolvedImport = replace(unit(request + sourceModule("")),
                                             requestFields, namespaceAuthority);
  expect(resolvedImport,
         false); // provider owner missing from interface envelope
  expect(withDependency(resolvedImport), true, true);
  const std::string symbolAuthority =
      "authority = {kind = \"symbol\", binding = {source = #dep, name = "
      "\"Leaf\", target = @Leaf, site = #site}}";
  expect(withDependency(replace(unit(request + sourceModule("")), requestFields,
                                symbolAuthority)),
         true);
  expect(withDependency(replace(resolvedImport, "kind = \"namespace\"",
                                "kind = \"unknown\"")),
         false);
  const std::string exported =
      replace(unit(sourceModule("")), "exports = []",
              "exports = [{name = \"Model\", target = @Model, site = #site}]");
  expect(exported, true, true);
  expect(replace(exported, "target = @Model", "target = \"Model\""), false);
  expect(replace(exported, "site = #site", "site = #occ"), false);
}

} // namespace
} // namespace acir::ac
