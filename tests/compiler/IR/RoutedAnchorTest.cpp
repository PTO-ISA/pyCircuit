#include "Compiler/SourceHeaderHelpers.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>

namespace acir::ac {
namespace {

// Hand-authored local IR oracles. These do not certify complete routed-package
// legality, source correspondence, hardware refinement or executable behavior.
std::string preamble() {
  std::string text = R"ir(
#owner = {package = "anchor", path = "anchor.py"}
#occ = {site = {definition = @Counter, ast_path = []}, expansion = []}
#child_occ = {site = {definition = @Counter, ast_path = [{kind = "index", value = 9 : i64}]}, expansion = []}
#span = {path = "anchor.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
)ir";
  for (unsigned value : {1u, 2u, 7u, 37u})
    text += "#w" + std::to_string(value) +
            " = #ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<" +
            std::to_string(value) + ">}, origin = #occ, location = #span}>\n";
  return text + R"ir(
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b7 = !ac.bits<#w7>
!b37 = !ac.bits<#w37>
)ir";
}

std::string statePackage(llvm::StringRef extra = "") {
  return preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %candidate: !b7, %intent: !b1, %wide_intent: !b2):
    %initial = "ac.bits.constant"() {value = #w2} : () -> !b7
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #occ} : (!b1, !b1, !b7) -> !b7
    "ac.state.update"(%q, %candidate, %intent) : (!b7, !b7, !b1) -> ()
)ir" + extra.str() +
         R"ir(
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b7, !b1, !b2) -> !b7, input_names = ["clk", "rst", "candidate", "intent", "wide_intent"], output_names = ["q"]} : () -> ()
}
)ir";
}

std::string externPackage() {
  return preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64} {
  "ac.module.import"() {sym_name = "external.combine", source_owner = #owner, semantic_kind = "hardware", parameters = [], type_parameters = [], function_type = (!b7, !b7) -> !b7, input_names = ["left", "right"], output_names = ["combined"], dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = [{port = 0 : i64, path = []}, {port = 1 : i64, path = []}]}], ac.external_contract = {state = "none", effects = [], latency = 0 : i64, availability = "all_inputs", value_domain = "four_state", failure = "none"}} : () -> ()
}
)ir";
}

// This complete anchor has no author writes: inventory authenticates its one
// logical allocation, and Q/false is the independently specified hold binding.
std::string fullStateAnchor() {
  return preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1):
    %initial = "ac.bits.constant"() {value = #w2} : () -> !b7
    %false = "ac.bits.constant"() {value = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = #occ, location = #span}>} : () -> !b1
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #occ} : (!b1, !b1, !b7) -> !b7
    "ac.state.update"(%q, %q, %false) : (!b7, !b7, !b1) -> ()
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !b7, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = []}} : () -> ()
  "ac.system"() {entry = {callee = @Counter, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
}

// Deliberately omit the second input from payload dependencies. Its
// availability must still contribute under the extern all_inputs contract.
std::string fullExternAnchor(bool cycle = false, bool zeroInputs = false,
                             bool partialInput = false) {
  std::string declaration =
      zeroInputs
          ? R"ir(function_type = () -> !b7, input_names = [], output_names = ["result"], dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}])ir"
          : R"ir(function_type = (!b7, !b7) -> !b7, input_names = ["first", "second"], output_names = ["result"], dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = [{port = 0 : i64, path = []}]}])ir";
  std::string graph = zeroInputs     ? R"ir(
    %result = "ac.instance"() {instance_name = "external", callee = @External, parameters = [], type_arguments = [], occurrence = #occ} : () -> !b7
    "ac.yield"(%result) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = () -> !b7, input_names = [], output_names = ["result"]} : () -> ()
)ir"
                      : partialInput ? R"ir(
  ^bb0(%first: !b7, %second: !b7, %available: !b1):
    %result = "ac.instance"(%first, %second) {instance_name = "external", callee = @External, parameters = [], type_arguments = [], occurrence = #occ} : (!b7, !b7) -> !b7
    "ac.yield"(%result, %available) : (!b7, !b1) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b7, !b7, !b1) -> (!b7, !b1), input_names = ["first", "second", "available"], output_names = ["result", "result_available"], ac.internal_ports = {source_inputs = 2 : i64, source_outputs = 1 : i64, root_inputs = [1 : i64]}, ac.input_validity = [-1 : i64, 2 : i64], ac.output_validity = [1 : i64]} : () -> ()
)ir"
                                     : R"ir(
  ^bb0(%value: !b7):
    %result = "ac.instance"(%value, %value) {instance_name = "external", callee = @External, parameters = [], type_arguments = [], occurrence = #occ} : (!b7, !b7) -> !b7
    "ac.yield"(%result) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b7) -> !b7, input_names = ["value"], output_names = ["result"]} : () -> ()
)ir";
  if (cycle) {
    const std::string inputs = "(%value, %value)";
    graph.replace(graph.find(inputs), inputs.size(), "(%value, %result)");
    const std::string yielded = "\"ac.yield\"(%result)";
    graph.replace(graph.find(yielded), yielded.size(), "\"ac.yield\"(%value)");
  }
  return preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64, ac.source_units = [#owner]} {
  "ac.module.import"() {sym_name = "External", source_owner = #owner, semantic_kind = "hardware", parameters = [], type_parameters = [], )ir" +
         declaration +
         R"ir(, ac.external_contract = {state = "none", effects = [], latency = 0 : i64, availability = "all_inputs", value_domain = "four_state", failure = "none"}} : () -> ()
  "ac.module"() ({
)ir" + graph +
         R"ir(
  "ac.system"() {entry = {callee = @Counter, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
}

std::string fullWrittenAnchor() {
  return preamble() + R"ir(
#id = {registration = #child_occ, check = #child_occ, obligation = 0 : i64}
#state_id = {owner = {instance_path = []}, declaration = #occ, element = []}
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1):
    %initial = "ac.bits.constant"() {value = #w2} : () -> !b7
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #occ} : (!b1, !b1, !b7) -> !b7
    %next, %enabled, %fire, %raw, %reached = "ac.rule"(%clk, %rst, %q) ({
    ^bb0(%clock: !b1, %reset: !b1, %old: !b7):
      %one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %step = "ac.bits.constant"() {value = #w1} : () -> !b7
      %sum = "ac.bits.binary"(%old, %step) {opcode = "add"} : (!b7, !b7) -> !b7
      %admit = "ac.bits.compare"(%one, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %write = "ac.bits.binary"(%admit, %one) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%sum, %write, %admit, %one, %one) : (!b7, !b1, !b1, !b1, !b1) -> ()
    }) {name = "increment", occurrence = #child_occ, ac.required_checks = [{id = #id, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 2 : i64, controls = [{kind = "callable_guard", id = #id, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = []}} : (!b1, !b1, !b7) -> (!b7, !b1, !b1, !b1, !b1)
    "ac.state.update"(%q, %next, %enabled) : (!b7, !b7, !b1) -> ()
    "ac.expect"(%raw, %reached, %clk, %rst) {kind = "guard_known", location = #span, ac.check_id = #id} : (!b1, !b1, !b1, !b1) -> ()
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !b7, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = [{registration = #child_occ, ordinary_results = 0 : i64, candidates = [{state = #state_id, data = 0 : i64, enable = 1 : i64}]}]}} : () -> ()
  "ac.system"() {entry = {callee = @Counter, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
}

class RoutedAnchorTest : public ::testing::Test {
protected:
  RoutedAnchorTest() { context.loadDialect<ACIRDialect>(); }

  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef text) {
    return mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
  }

  template <typename Op> Op find(mlir::ModuleOp file) {
    Op result;
    file.walk([&](Op op) {
      if (!result)
        result = op;
    });
    EXPECT_TRUE(result);
    return result;
  }

  template <typename Verify>
  void rejects(Verify verify, llvm::StringRef reason) {
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    EXPECT_TRUE(mlir::failed(verify()));
    EXPECT_NE(diagnostics.find(reason.str()), std::string::npos) << diagnostics;
  }

  void rejects(mlir::ModuleOp file, llvm::StringRef reason) {
    rejects([&] { return mlir::verify(file); }, reason);
  }

  mlir::DictionaryAttr changed(mlir::DictionaryAttr dictionary,
                               llvm::StringRef key, mlir::Attribute value) {
    mlir::NamedAttrList fields(dictionary);
    if (value)
      fields.set(key, value);
    else
      fields.erase(key);
    return fields.getDictionary(&context);
  }

  mlir::MLIRContext context;
  mlir::Builder builder{&context};
};

TEST_F(RoutedAnchorTest, LogicalStateAndDirectUpdateHaveLocalLegality) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  auto state = find<SourceStateOp>(*file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(state);
  ASSERT_TRUE(update);
  EXPECT_EQ(update.getState(), state.getQ());
  EXPECT_EQ(update.getCandidate(), state->getBlock()->getArgument(2));
  EXPECT_EQ(update.getIntent(), state->getBlock()->getArgument(3));
  EXPECT_EQ(update->getNumResults(), 0u);
  EXPECT_EQ(update->getNumRegions(), 0u);
  EXPECT_TRUE(update->getAttrs().empty());
}

TEST_F(RoutedAnchorTest, UpdateWritesSourceStateAndCannotBeEliminatedAsPure) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  auto effects = mlir::getEffectsRecursively(update);
  ASSERT_TRUE(effects.has_value());
  EXPECT_TRUE(llvm::any_of(*effects, [](const auto &effect) {
    return mlir::isa<mlir::MemoryEffects::Write>(effect.getEffect()) &&
           effect.getResource() == SourceStateResource::get();
  }));
  EXPECT_FALSE(mlir::isMemoryEffectFree(update));
  EXPECT_FALSE(mlir::isSpeculatable(update));
  EXPECT_FALSE(mlir::isOpTriviallyDead(update));
}

TEST_F(RoutedAnchorTest, LogicalStateRequiresOneAndOnlyOneUpdate) {
  for (bool duplicate : {false, true}) {
    SCOPED_TRACE(duplicate ? "duplicate" : "missing");
    auto file = parse(statePackage());
    ASSERT_TRUE(file);
    auto update = find<StateUpdateOp>(*file);
    ASSERT_TRUE(update);
    if (duplicate)
      update->getBlock()->getOperations().insert(update->getIterator(),
                                                 update->clone());
    else
      update->erase();
    rejects(*file, "exactly one same-block update");
  }
}

TEST_F(RoutedAnchorTest, AliasAndBlockArgumentCannotSupplyStateOwnership) {
  auto file = parse(statePackage(R"ir(
    %alias = "ac.bits.select"(%intent, %q, %q) : (!b1, !b7, !b7) -> !b7
    "ac.state.update"(%alias, %candidate, %intent) : (!b7, !b7, !b1) -> ()
)ir"));
  ASSERT_TRUE(file);
  rejects(*file, "direct result of a same-module ac.state");

  file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  update->setOperand(0, update.getCandidate());
  // Isolate ownership from the declaration's separate missing-update check.
  rejects([&] { return update.verify(); },
          "direct result of a same-module ac.state");
}

TEST_F(RoutedAnchorTest, ChildResultCannotBecomeParentOwnedState) {
  auto text = statePackage(R"ir(
    %child_q = "ac.instance"(%candidate) {instance_name = "child", callee = @Child, parameters = [], type_arguments = [], occurrence = #child_occ} : (!b7) -> !b7
    "ac.state.update"(%child_q, %candidate, %intent) : (!b7, !b7, !b1) -> ()
)ir");
  const auto closing = text.rfind('}');
  ASSERT_NE(closing, std::string::npos);
  text.insert(closing, R"ir(
  "ac.module"() ({
  ^bb0(%value: !b7):
    "ac.yield"(%value) : (!b7) -> ()
  }) {sym_name = "Child", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b7) -> !b7, input_names = ["value"], output_names = ["value"]} : () -> ()
)ir");
  auto file = parse(text);
  ASSERT_TRUE(file);
  rejects(*file, "direct result of a same-module ac.state");
}

TEST_F(RoutedAnchorTest, RuleCannotContainAnUpdate) {
  auto file = parse(statePackage(R"ir(
    "ac.rule"(%q, %candidate, %intent) ({
    ^bb0(%captured_q: !b7, %next: !b7, %write: !b1):
      "ac.state.update"(%captured_q, %next, %write) : (!b7, !b7, !b1) -> ()
      "ac.yield"() : () -> ()
    }) {name = "nested", occurrence = #child_occ} : (!b7, !b7, !b1) -> ()
)ir"));
  ASSERT_TRUE(file);
  rejects(*file, "direct module graph placement");
}

TEST_F(RoutedAnchorTest, IntentMustResolveToOneBit) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  update->setOperand(2, update->getBlock()->getArgument(4));
  rejects(*file, "state update intent requires width-one Bits");
}

TEST_F(RoutedAnchorTest, CandidateMustKeepExactNominalIdentity) {
  auto text = preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 2 : i64} {
  "ac.enum"() {sym_name = "Left", width = #ac.math_int<7>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}]} : () -> ()
  "ac.enum"() {sym_name = "Right", width = #ac.math_int<7>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %next: !ac.enum<"Left">, %other: !ac.enum<"Right">, %write: !b1):
    %initial = "ac.enum.create"() {member = "ZERO"} : () -> !ac.enum<"Left">
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "tag", occurrence = #occ} : (!b1, !b1, !ac.enum<"Left">) -> !ac.enum<"Left">
    "ac.state.update"(%q, %next, %write) : (!ac.enum<"Left">, !ac.enum<"Left">, !b1) -> ()
    "ac.yield"(%q) : (!ac.enum<"Left">) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !ac.enum<"Left">, !ac.enum<"Right">, !b1) -> !ac.enum<"Left">, input_names = ["clk", "rst", "next", "other", "write"], output_names = ["tag"]} : () -> ()
}
)ir";
  auto file = parse(text);
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  update->setOperand(1, update->getBlock()->getArgument(3));
  rejects(*file, "candidate must exactly match the state payload type");
}

TEST_F(RoutedAnchorTest, UpdateRejectsOperationSpecificAttributes) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  update->setAttr("pretend_commit", builder.getBoolAttr(true));
  rejects(*file, "state update does not accept attributes");
}

TEST_F(RoutedAnchorTest, UpdateRejectsSourceFinalAndUnstagedPlacement) {
  for (llvm::StringRef stage : {"source", "final", ""}) {
    SCOPED_TRACE(stage.str());
    auto file = parse(statePackage());
    ASSERT_TRUE(file);
    auto update = find<StateUpdateOp>(*file);
    ASSERT_TRUE(update);
    (*file)->removeAttr("ac.routed_contract");
    if (stage.empty())
      (*file)->removeAttr("ac.stage");
    else
      (*file)->setAttr("ac.stage", builder.getStringAttr(stage));
    rejects([&] { return update.verify(); }, "routed contract-2 package");
  }
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  (*file)->setAttr("ac.unit_kind", builder.getStringAttr("body"));
  rejects([&] { return update.verify(); }, "routed contract-2 package");
}

TEST_F(RoutedAnchorTest, LogicalStateIsForbiddenInFinalEvenWithoutProvenance) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  (*file)->removeAttr("ac.routed_contract");
  (*file)->setAttr("ac.stage", builder.getStringAttr("final"));
  rejects(*file, "logical state is forbidden in a final package");
}

TEST_F(RoutedAnchorTest, UpdateRequiresTheRoutedContractMarker) {
  auto file = parse(statePackage());
  ASSERT_TRUE(file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(update);
  (*file)->removeAttr("ac.routed_contract");
  rejects([&] { return update.verify(); }, "routed contract-2 package");
}

TEST_F(RoutedAnchorTest, ContractMarkerRejectsMalformedAndUnknownVersions) {
  for (mlir::Attribute invalid :
       {mlir::Attribute(builder.getStringAttr("2")),
        mlir::Attribute(builder.getI32IntegerAttr(2)),
        mlir::Attribute(builder.getI64IntegerAttr(0)),
        mlir::Attribute(builder.getI64IntegerAttr(1)),
        mlir::Attribute(builder.getI64IntegerAttr(3))}) {
    auto file = parse("module attributes {ac.stage = \"routed\"} {}");
    ASSERT_TRUE(file);
    (*file)->setAttr("ac.routed_contract", invalid);
    rejects(*file, "ac.routed_contract requires version 2 : i64");
  }
}

TEST_F(RoutedAnchorTest, PreviousContractVersionIsRejectedAtBothGraphStages) {
  for (llvm::StringRef stage : {"routed", "final"}) {
    auto file = parse("module attributes {ac.stage = \"" + stage.str() +
                      "\", ac.routed_contract = 1 : i64} {}");
    ASSERT_TRUE(file);
    rejects(*file, "ac.routed_contract requires version 2 : i64");
  }
}

TEST_F(RoutedAnchorTest, ContractMarkerRequiresTopLevelRoutedOrFinalPackage) {
  for (llvm::StringRef stage : {"source", "unsupported", ""}) {
    auto file = parse("module {}");
    ASSERT_TRUE(file);
    (*file)->setAttr("ac.routed_contract", builder.getI64IntegerAttr(2));
    if (!stage.empty())
      (*file)->setAttr("ac.stage", builder.getStringAttr(stage));
    rejects(*file, "ac.routed_contract requires version 2 : i64");
  }
  auto source = parse("module attributes {ac.stage = \"routed\", ac.unit_kind "
                      "= \"body\", ac.routed_contract = 2 : i64} {}");
  ASSERT_TRUE(source);
  rejects(*source, "ac.routed_contract requires version 2 : i64");
  for (llvm::StringRef stage : {"routed", "final"}) {
    auto nested =
        parse("module attributes {ac.stage = \"" + stage.str() +
              "\", ac.routed_contract = 2 : i64} { module attributes "
              "{ac.stage = \"routed\", ac.routed_contract = 2 : i64} {} }");
    ASSERT_TRUE(nested);
    // Root identity is established by consuming the actual package. Generic
    // parsing can temporarily nest that package inside a parser-owned module.
    rejects(
        [&] {
          return stage == "routed" ? verifyRoutedPackage(*nested)
                                   : verifyHardwarePackage(*nested);
        },
        "ac.routed_contract belongs only to the top-level package");
  }
  auto onDefinition = parse(statePackage());
  ASSERT_TRUE(onDefinition);
  auto definition = find<ModuleOp>(*onDefinition);
  ASSERT_TRUE(definition);
  definition->setAttr("ac.routed_contract", builder.getI64IntegerAttr(2));
  rejects(*onDefinition, "ac.routed_contract requires version 2 : i64");
}

TEST_F(RoutedAnchorTest,
       FinalProvenanceMarkerIsOptionalButMustBeValidIfPresent) {
  auto file = parse("module attributes {ac.stage = \"final\"} {}");
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  (*file)->setAttr("ac.routed_contract", builder.getI64IntegerAttr(2));
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  (*file)->setAttr("ac.routed_contract", builder.getI64IntegerAttr(37));
  rejects(*file, "ac.routed_contract requires version 2 : i64");
}

TEST_F(RoutedAnchorTest, OrdinaryExternAcceptsExactlyTheCompleteDeclaration) {
  auto file = parse(externPackage());
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
}

TEST_F(RoutedAnchorTest, EveryExternContractKeyIsRequiredAndExtrasAreRejected) {
  for (llvm::StringRef key : {"state", "effects", "latency", "availability",
                              "value_domain", "failure", "extra"}) {
    SCOPED_TRACE(key.str());
    auto file = parse(externPackage());
    ASSERT_TRUE(file);
    auto imported = find<ModuleImportOp>(*file);
    ASSERT_TRUE(imported);
    auto contract =
        imported->getAttrOfType<mlir::DictionaryAttr>("ac.external_contract");
    imported->setAttr("ac.external_contract",
                      changed(contract, key,
                              key == "extra"
                                  ? mlir::Attribute(builder.getUnitAttr())
                                  : mlir::Attribute()));
    rejects(*file, "ac.external_contract requires exactly");
  }
}

TEST_F(RoutedAnchorTest,
       ExternContractRejectsStateEffectsLatencyAndDomainChanges) {
  const std::pair<llvm::StringRef, mlir::Attribute> mutations[] = {
      {"state", builder.getStringAttr("hidden")},
      {"effects", builder.getArrayAttr({builder.getStringAttr("observe")})},
      {"latency", builder.getI64IntegerAttr(1)},
      {"latency", builder.getI32IntegerAttr(0)},
      {"availability", builder.getStringAttr("payload_inputs")},
      {"value_domain", builder.getStringAttr("two_state")},
      {"failure", builder.getStringAttr("may_fail")}};
  for (const auto &[key, value] : mutations) {
    SCOPED_TRACE(key.str());
    auto file = parse(externPackage());
    ASSERT_TRUE(file);
    auto imported = find<ModuleImportOp>(*file);
    ASSERT_TRUE(imported);
    auto contract =
        imported->getAttrOfType<mlir::DictionaryAttr>("ac.external_contract");
    imported->setAttr("ac.external_contract", changed(contract, key, value));
    rejects(*file, "ac.external_contract requires exactly");
  }
  auto file = parse(externPackage());
  ASSERT_TRUE(file);
  auto imported = find<ModuleImportOp>(*file);
  ASSERT_TRUE(imported);
  imported->setAttr("ac.external_contract", builder.getStringAttr("complete"));
  rejects(*file, "ac.external_contract requires exactly");
}

TEST_F(RoutedAnchorTest, ExternStillRequiresExactCompleteDependencySummary) {
  for (bool missing : {true, false}) {
    auto file = parse(externPackage());
    ASSERT_TRUE(file);
    auto imported = find<ModuleImportOp>(*file);
    ASSERT_TRUE(imported);
    if (missing)
      imported->removeAttr("dependency_summary");
    else
      imported->setAttr("dependency_summary", builder.getArrayAttr({}));
    // The declaration contract does not waive the existing summary verifier.
    rejects([&] { return mlir::verify(*file); }, "dependency");
  }
}

TEST_F(RoutedAnchorTest, ExternContractIsForbiddenOnDefinitionsAndBuiltins) {
  auto external = parse(externPackage());
  ASSERT_TRUE(external);
  auto imported = find<ModuleImportOp>(*external);
  ASSERT_TRUE(imported);
  auto contract = imported->getAttr("ac.external_contract");
  auto definitionFile = parse(statePackage());
  ASSERT_TRUE(definitionFile);
  auto definition = find<ModuleOp>(*definitionFile);
  ASSERT_TRUE(definition);
  definition->setAttr("ac.external_contract", contract);
  rejects(*definitionFile, "ac.external_contract");

  imported->setAttr("sym_name",
                    builder.getStringAttr("pycircuit.__builtins__.external"));
  rejects(*external, "ac.external_contract");
}

TEST_F(RoutedAnchorTest,
       ExternContractCannotClaimPrimitiveOrTransactionSemantics) {
  for (llvm::StringRef forbidden : {"primitive_kind", "transaction_summary"}) {
    auto file = parse(externPackage());
    ASSERT_TRUE(file);
    auto imported = find<ModuleImportOp>(*file);
    ASSERT_TRUE(imported);
    imported->setAttr(forbidden,
                      forbidden == "primitive_kind"
                          ? mlir::Attribute(builder.getStringAttr("dff"))
                          : mlir::Attribute(builder.getDictionaryAttr({})));
    rejects(*file, "ac.external_contract");
  }
  auto file = parse(externPackage());
  ASSERT_TRUE(file);
  auto imported = find<ModuleImportOp>(*file);
  ASSERT_TRUE(imported);
  imported->setAttr("semantic_kind", builder.getStringAttr("transactions"));
  rejects(*file, "ac.external_contract requires an ordinary hardware import");
}

TEST_F(RoutedAnchorTest, SourceReferencesAndCallableOrderCannotSurviveRouting) {
  for (llvm::StringRef stage : {"routed", "final"}) {
    for (llvm::StringRef key : {"ac.reference_parameters", "ac.reference_bindings",
                                "ac.reference", "ac.callable_after"}) {
      for (llvm::StringRef placement : {"package", "definition", "yield"}) {
        SCOPED_TRACE(stage.str() + ":" + key.str() + ":" + placement.str());
        auto file = parse(preamble() +
            "module attributes {ac.stage = \"" + stage.str() + R"ir(", ac.routed_contract = 2 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%value: !b7):
    "ac.yield"(%value) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b7) -> !b7, input_names = ["value"], output_names = ["result"]} : () -> ()
  "ac.system"() {entry = {callee = @Counter, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir");
        ASSERT_TRUE(file);
        auto verify = [&] {
          return stage == "routed" ? verifyRoutedPackage(*file)
                                   : verifyHardwarePackage(*file);
        };
        ASSERT_TRUE(mlir::succeeded(verify()));
        mlir::Operation *target = file->getOperation();
        if (placement == "definition")
          target = find<ModuleOp>(*file).getOperation();
        else if (placement == "yield")
          target = find<YieldOp>(*file).getOperation();
        // Even empty/malformed annotations cannot silently cross the saved
        // graph boundary; placement on an unrelated op is not an escape hatch.
        target->setAttr(key, builder.getArrayAttr({}));
        rejects(verify, key);
      }
    }
  }
}

TEST_F(RoutedAnchorTest, StandaloneLogicalAnchorNeedsNoSourceOrPreparation) {
  std::string saved;
  {
    mlir::MLIRContext originalContext;
    originalContext.loadDialect<ACIRDialect>();
    auto original = mlir::parseSourceString<mlir::ModuleOp>(fullStateAnchor(),
                                                            &originalContext);
    ASSERT_TRUE(original);
    ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*original)));
    llvm::raw_string_ostream stream(saved);
    original->print(stream);
  }
  // The original operations and context have both been destroyed. Only the
  // serialized graph enters this independently constructed context.
  mlir::MLIRContext reloadedContext;
  reloadedContext.loadDialect<ACIRDialect>();
  auto restored =
      mlir::parseSourceString<mlir::ModuleOp>(saved, &reloadedContext);
  ASSERT_TRUE(restored);
  EXPECT_TRUE(mlir::succeeded(verifyRoutedPackage(*restored)));
  std::string reprinted;
  llvm::raw_string_ostream stream(reprinted);
  restored->print(stream);
  EXPECT_EQ(saved, reprinted);
}

TEST_F(RoutedAnchorTest, StandaloneLogicalQHasNoCombinationalDependencies) {
  auto file = parse(fullStateAnchor());
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  auto state = find<SourceStateOp>(*file);
  ASSERT_TRUE(state);
  HardwareBindings bindings;
  bindings.owner = state->getParentOp();
  auto dependencies =
      HardwareAnalysis(*file).getDependencies(state.getQ(), {}, bindings);
  ASSERT_TRUE(mlir::succeeded(dependencies));
  EXPECT_TRUE(dependencies->empty());
  EXPECT_EQ(find<StateUpdateOp>(*file).getCandidate(), state.getQ());
}

TEST_F(RoutedAnchorTest, StandaloneSampledWriterFeedsBackAcrossLogicalQCut) {
  auto file = parse(fullWrittenAnchor());
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  auto state = find<SourceStateOp>(*file);
  auto rule = find<RuleOp>(*file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(state);
  ASSERT_TRUE(rule);
  ASSERT_TRUE(update);
  EXPECT_EQ(rule.getCaptures()[2], state.getQ());
  EXPECT_EQ(update.getCandidate(), rule.getResult(0));
  EXPECT_EQ(update.getIntent(), rule.getResult(1));

  // A still-well-typed candidate cannot name a different declaration merely
  // by changing StateID. Both declaration and actual SSA ownership bind it.
  auto owner = find<ModuleOp>(*file);
  ASSERT_TRUE(owner);
  auto wiring =
      owner->getAttrOfType<mlir::DictionaryAttr>("ac.transaction_wiring");
  auto writers = wiring.getAs<mlir::ArrayAttr>("writers");
  auto writer = mlir::cast<mlir::DictionaryAttr>(writers[0]);
  auto candidates = writer.getAs<mlir::ArrayAttr>("candidates");
  auto candidate = mlir::cast<mlir::DictionaryAttr>(candidates[0]);
  auto stateID = candidate.getAs<mlir::DictionaryAttr>("state");
  stateID = changed(stateID, "declaration", rule.getOccurrence());
  candidate = changed(candidate, "state", stateID);
  writer = changed(writer, "candidates", builder.getArrayAttr({candidate}));
  owner->setAttr("ac.transaction_wiring",
                 changed(wiring, "writers", builder.getArrayAttr({writer})));
  rejects([&] { return verifyRoutedPackage(*file); },
          "StateID terminal owner is missing its matching inventory");
}

TEST_F(RoutedAnchorTest, StandaloneLogicalStateRequiresItsActualInventory) {
  for (llvm::StringRef mutation : {"missing", "empty", "wrong_identity"}) {
    SCOPED_TRACE(mutation.str());
    auto file = parse(fullStateAnchor());
    ASSERT_TRUE(file);
    auto owner = find<ModuleOp>(*file);
    ASSERT_TRUE(owner);
    if (mutation == "missing")
      owner->removeAttr("ac.transaction_wiring");
    else {
      auto wiring =
          owner->getAttrOfType<mlir::DictionaryAttr>("ac.transaction_wiring");
      auto replacement =
          mutation == "empty"
              ? builder.getArrayAttr({})
              : builder.getArrayAttr(
                    {find<SourceStateOp>(*file).getOccurrence()});
      if (mutation == "wrong_identity") {
        auto declaration = find<SourceStateOp>(*file).getOccurrence();
        auto site = declaration.getAs<mlir::DictionaryAttr>("site");
        auto step = builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("index")),
             builder.getNamedAttr("value", builder.getI64IntegerAttr(37))});
        declaration =
            changed(declaration, "site",
                    changed(site, "ast_path", builder.getArrayAttr({step})));
        replacement = builder.getArrayAttr({declaration});
      }
      owner->setAttr("ac.transaction_wiring",
                     changed(wiring, "states", replacement));
    }
    rejects([&] { return verifyRoutedPackage(*file); },
            mutation == "missing" ? "requires transaction_wiring companion"
            : mutation == "empty" ? "missing its transaction inventory"
                                  : "no matching direct state allocation");
  }
}

TEST_F(RoutedAnchorTest, StandaloneRejectsUnmappedWriterRegistration) {
  auto file = parse(fullStateAnchor());
  ASSERT_TRUE(file);
  auto owner = find<ModuleOp>(*file);
  ASSERT_TRUE(owner);
  auto wiring =
      owner->getAttrOfType<mlir::DictionaryAttr>("ac.transaction_wiring");
  auto writer = builder.getDictionaryAttr(
      {builder.getNamedAttr("registration",
                            find<SourceStateOp>(*file).getOccurrence()),
       builder.getNamedAttr("ordinary_results", builder.getI64IntegerAttr(0)),
       builder.getNamedAttr("candidates", builder.getArrayAttr({}))});
  owner->setAttr("ac.transaction_wiring",
                 changed(wiring, "writers", builder.getArrayAttr({writer})));
  rejects([&] { return verifyRoutedPackage(*file); },
          "writer must resolve to a sampled Rule");
}

TEST_F(RoutedAnchorTest, StandaloneUnwrittenStateRequiresQAndFalse) {
  for (bool wrongCandidate : {true, false}) {
    auto file = parse(fullStateAnchor());
    ASSERT_TRUE(file);
    auto update = find<StateUpdateOp>(*file);
    auto state = find<SourceStateOp>(*file);
    ASSERT_TRUE(update);
    ASSERT_TRUE(state);
    if (wrongCandidate)
      update->setOperand(1, state.getInitial());
    else
      update->setOperand(2, state.getClk());
    rejects([&] { return verifyRoutedPackage(*file); },
            wrongCandidate ? "candidate Q and false intent"
                           : "D/E origin differs");
  }
}

TEST_F(RoutedAnchorTest, StandaloneChecksDeadCandidateAndIntentCycles) {
  for (bool intentCycle : {false, true}) {
    SCOPED_TRACE(intentCycle);
    auto file = parse(fullStateAnchor());
    ASSERT_TRUE(file);
    auto update = find<StateUpdateOp>(*file);
    ASSERT_TRUE(update);
    mlir::OpBuilder opBuilder(update);
    mlir::Value seed = intentCycle ? update.getIntent() : update.getCandidate();
    mlir::OperationState operation(update.getLoc(),
                                   BitsBinaryOp::getOperationName());
    operation.addOperands({seed, seed});
    operation.addTypes(seed.getType());
    operation.addAttribute("opcode", builder.getStringAttr("xor"));
    auto *cycle = opBuilder.create(operation);
    cycle->setOperand(0, cycle->getResult(0));
    update->setOperand(intentCycle ? 2 : 1, cycle->getResult(0));
    // Q remains the only exported value; false intent cannot hide the data
    // cycle. A cycle used only by result-free update intent is checked too.
    rejects([&] { return verifyRoutedPackage(*file); }, "combinational cycle");
  }
}

TEST_F(RoutedAnchorTest, StandaloneRejectsMissingUpdateAndInvalidOwner) {
  for (bool missing : {true, false}) {
    auto file = parse(fullStateAnchor());
    ASSERT_TRUE(file);
    auto update = find<StateUpdateOp>(*file);
    auto state = find<SourceStateOp>(*file);
    ASSERT_TRUE(update);
    ASSERT_TRUE(state);
    if (missing) {
      update->erase();
      rejects([&] { return verifyRoutedPackage(*file); },
              "exactly one same-block update");
    } else {
      // Keep the real update, then add a forged update targeting an equal-typed
      // non-state value so declaration cardinality cannot mask ownership.
      auto *forged = update->clone();
      update->getBlock()->getOperations().insert(update->getIterator(), forged);
      forged->setOperand(0, state.getInitial());
      rejects([&] { return verifyRoutedPackage(*file); },
              "direct result of a same-module ac.state");
    }
  }
}

TEST_F(RoutedAnchorTest, StandaloneRequiresMarkerAndRejectsRemainingSourceOps) {
  for (bool missing : {true, false}) {
    auto file = parse(fullStateAnchor());
    ASSERT_TRUE(file);
    if (missing)
      (*file)->removeAttr("ac.routed_contract");
    else
      (*file)->setAttr("ac.routed_contract", builder.getI64IntegerAttr(3));
    rejects([&] { return verifyRoutedPackage(*file); },
            "unsupported or missing ac.routed_contract");
  }
  auto file = parse(fullStateAnchor());
  ASSERT_TRUE(file);
  auto state = find<SourceStateOp>(*file);
  ASSERT_TRUE(state);
  mlir::OpBuilder opBuilder(state);
  mlir::OperationState read(state.getLoc(),
                            SourceStateReadOp::getOperationName());
  read.addOperands(state.getClk());
  read.addTypes(state.getQ().getType());
  read.addAttribute("state", builder.getStringAttr("count"));
  read.addAttribute("selectors", builder.getArrayAttr({}));
  read.addAttribute("occurrence", state.getOccurrence());
  opBuilder.create(read);
  rejects([&] { return verifyRoutedPackage(*file); },
          "outside closed routed hardware package");
}

TEST_F(RoutedAnchorTest,
       StandaloneExternIsLegalButCannotBecomeExecutableByRelabeling) {
  auto file = parse(fullExternAnchor());
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  rejects([&] { return verifyHardwarePackage(*file); },
          "require state refinement");
  (*file)->setAttr("ac.stage", builder.getStringAttr("final"));
  rejects([&] { return verifyHardwarePackage(*file); },
          "implementation is required for execution completeness");
}

TEST_F(RoutedAnchorTest,
       StandaloneExternNeedsContractAndZeroInputExternIsTotal) {
  auto zero = parse(fullExternAnchor(false, true));
  ASSERT_TRUE(zero);
  EXPECT_TRUE(mlir::succeeded(verifyRoutedPackage(*zero)));
  auto file = parse(fullExternAnchor());
  ASSERT_TRUE(file);
  auto imported = find<ModuleImportOp>(*file);
  ASSERT_TRUE(imported);
  imported->removeAttr("ac.external_contract");
  rejects([&] { return verifyRoutedPackage(*file); },
          "routed ordinary module import requires ac.external_contract");
}

TEST_F(RoutedAnchorTest,
       StandaloneExternAvailabilityIncludesPayloadIndependentInputs) {
  auto file = parse(fullExternAnchor(false, false, true));
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  auto owner = find<ModuleOp>(*file);
  ASSERT_TRUE(owner);
  auto yielded = mlir::cast<YieldOp>(owner.getBody().front().back());
  // Input 0 is total and is the only payload dependency. Using it as the
  // claimed availability would ignore partial input 1, which is forbidden.
  mlir::OpBuilder opBuilder(yielded);
  mlir::OperationState one(yielded.getLoc(),
                           BitsConstantOp::getOperationName());
  one.addTypes(yielded.getValues()[1].getType());
  one.addAttribute(
      "value",
      mlir::cast<BitsType>(yielded.getValues()[1].getType()).getWidth());
  yielded->setOperand(1, opBuilder.create(one)->getResult(0));
  rejects([&] { return verifyRoutedPackage(*file); },
          "availability differs from actual payload SSA");
}

TEST_F(RoutedAnchorTest,
       StandaloneExternRejectsDeadAvailabilityCycleWithoutPayloadCycle) {
  auto file = parse(fullExternAnchor(true));
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
  rejects([&] { return verifyRoutedPackage(*file); },
          "cycle in hardware value availability");
}

TEST_F(RoutedAnchorTest,
       LogicalAndPhysicalStateCannotShareOccurrenceEvenWithDistinctNames) {
  auto text = fullStateAnchor();
  auto insertion = text.find("  \"ac.module\"()");
  ASSERT_NE(insertion, std::string::npos);
  text.insert(insertion, R"ir(
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "Storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@Storage, "T">, !ac.type_param<@Storage, "T">) -> !ac.type_param<@Storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
)ir");
  auto file = parse(text);
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  auto owner = find<ModuleOp>(*file);
  auto state = find<SourceStateOp>(*file);
  auto update = find<StateUpdateOp>(*file);
  ASSERT_TRUE(owner);
  ASSERT_TRUE(state);
  ASSERT_TRUE(update);
  mlir::OpBuilder ops(owner.getBody().front().getTerminator());
  mlir::OperationState physical(state.getLoc(), InstanceOp::getOperationName());
  physical.addOperands({state.getClk(), state.getRst(), update.getIntent(),
                        state.getQ(), state.getInitial()});
  physical.addTypes(state.getQ().getType());
  physical.addAttribute("instance_name",
                        builder.getStringAttr("physical_alias"));
  physical.addAttribute("callee",
                        mlir::FlatSymbolRefAttr::get(&context, "Storage"));
  physical.addAttribute("parameters", builder.getArrayAttr({}));
  physical.addAttribute(
      "type_arguments",
      builder.getArrayAttr({mlir::TypeAttr::get(state.getQ().getType())}));
  physical.addAttribute("occurrence", state.getOccurrence());
  auto *allocation = ops.create(physical);
  ASSERT_NE(allocation->getAttrOfType<mlir::StringAttr>("instance_name"),
            state.getInstanceNameAttr());
  rejects([&] { return verifyRoutedPackage(*file); },
          "multiple logical or physical allocations");
}

TEST_F(RoutedAnchorTest, ExternCollectionRetainsTotalInputAdmissionLimitation) {
  auto file = parse(fullExternAnchor(false, false, true));
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  auto owner = find<ModuleOp>(*file);
  auto instance = find<InstanceOp>(*file);
  ASSERT_TRUE(owner);
  ASSERT_TRUE(instance);
  auto &body = owner.getBody().front();
  auto one = mlir::cast<BitsType>(body.getArgument(2).getType()).getWidth();
  auto shape = builder.getArrayAttr({one});
  auto table = TableType::get(&context, shape, body.getArgument(0).getType());
  mlir::OpBuilder ops(instance);
  auto splat = [&](mlir::Value value) {
    mlir::OperationState state(instance.getLoc(),
                               TableSplatOp::getOperationName());
    state.addOperands(value);
    state.addTypes(table);
    state.addAttribute("shape", shape);
    return ops.create(state)->getResult(0);
  };
  auto first = splat(body.getArgument(0));
  auto second = splat(body.getArgument(1));
  mlir::OperationState state(instance.getLoc(),
                             CollectionOp::getOperationName());
  state.addOperands({first, first});
  state.addTypes(table);
  state.addAttributes(instance->getAttrs());
  state.addAttribute("shape", shape);
  auto collection = mlir::cast<CollectionOp>(ops.create(state));
  mlir::OperationState total(instance.getLoc(),
                             BitsConstantOp::getOperationName());
  total.addTypes(body.getArgument(2).getType());
  total.addAttribute("value", one);
  auto available = ops.create(total)->getResult(0);
  // Leave the Collection result dead: its input availability still needs
  // validation, independently of the total exported payload and companion.
  body.getTerminator()->setOperands({body.getArgument(0), available});
  instance->erase();
  ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(*file)));
  collection->setOperand(1, second);
  rejects(
      [&] { return verifyRoutedPackage(*file); },
      "availability requires scalar instance routing for ordinary Collections");
}

TEST_F(RoutedAnchorTest, DeclarationSnapshotEqualityIncludesExternalContract) {
  auto authority = parse(externPackage());
  auto snapshot = parse(externPackage());
  ASSERT_TRUE(authority);
  ASSERT_TRUE(snapshot);
  auto published = find<ModuleImportOp>(*authority);
  auto imported = find<ModuleImportOp>(*snapshot);
  ASSERT_TRUE(published);
  ASSERT_TRUE(imported);
  ASSERT_TRUE(acir::compiler::detail::sameDeclaration(published, imported));
  auto contract = imported->getAttr("ac.external_contract");
  imported->removeAttr("ac.external_contract");
  // An ordinary declaration awaiting a supplied body remains locally legal;
  // that does not let it impersonate a published external declaration.
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*snapshot)));
  EXPECT_FALSE(acir::compiler::detail::sameDeclaration(published, imported));
  imported->setAttr("ac.external_contract", contract);
  EXPECT_TRUE(acir::compiler::detail::sameDeclaration(published, imported));
  imported->setAttr("ac.external_contract",
                    changed(mlir::cast<mlir::DictionaryAttr>(contract),
                            "latency", builder.getI64IntegerAttr(1)));
  EXPECT_FALSE(acir::compiler::detail::sameDeclaration(published, imported));
}

} // namespace
} // namespace acir::ac
