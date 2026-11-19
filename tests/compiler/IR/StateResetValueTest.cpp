#include "Compiler/AnchorArtifact.h"
#include "Compiler/SourceUnit.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "pycircuit/Conversion/GFIRToHWIR.h"
#include "pycircuit/Dialect/ACIR/GFIRPackageValidation.h"
#include "pycircuit/Dialect/ACIR/HWIRPackageValidation.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Builders.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "pycircuit/Dialect/ACIR/HardwareDomains.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace acir::compiler {
namespace {
namespace ac = acir::ac;
namespace gf = acir::gfir;
namespace fs = std::filesystem;

// The admitted source interface supplies publication provenance only. The
// routed state and its update are specified here, not produced by routing
// a Python fixture or inferred from the implementation's output.
std::string stateIR(bool routed) {
  std::string text = R"ir(
#owner = {package = "reset_value", path = "fixture.py"}
#occ = {site = {definition = @"reset_value.fixture.Held", ast_path = []}, expansion = []}
#writer = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#id = {registration = #writer, check = #writer, obligation = 0 : i64}
#state_id = {owner = {instance_path = []}, declaration = #occ, element = []}
#span = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
)ir";
  for (unsigned value : {0u, 1u, 5u, 9u})
    text += "#w" + std::to_string(value) +
            " = #ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<" + std::to_string(value) +
            ">}, origin = #occ, location = #span}>\n";
  text += "!b1 = !ac.bits<#w1>\n!b9 = !ac.bits<#w9>\n";
  text += routed
              ? "module attributes {ac.stage = \"routed\", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {\n"
              : "module attributes {ac.stage = \"source\", ac.unit_kind = \"implementation\", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {\n";
  text += R"ir(
  "ac.module"() ({
  ^bb0(%enable: !b1, %clk: !b1, %rst: !b1):
    %initial = "ac.bits.constant"() {value = #w5} : () -> !b9
)ir";
  if (routed)
    text += R"ir(
    %reset_value = "ac.bits.constant"() {value = #w0} : () -> !b9
    %q = "gfir.state"(%clk, %rst, %initial, %reset_value) {instance_name = "held", occurrence = #occ, domain = "D", operandSegmentSizes = array<i32: 1, 1, 1, 1>, init_kind = "explicit", edge = "posedge", reset_kind = "sync", reset_polarity = "high"} : (!b1, !b1, !b9, !b9) -> !b9
    %next, %enabled, %fire, %raw, %reached = "ac.rule"(%clk, %rst, %q, %enable) ({
    ^bb0(%clock: !b1, %reset: !b1, %old: !b9, %allow: !b1):
      %one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %step = "ac.bits.constant"() {value = #w1} : () -> !b9
      %sum = "ac.bits.binary"(%old, %step) {opcode = "add"} : (!b9, !b9) -> !b9
      %admit = "ac.bits.compare"(%allow, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %write = "ac.bits.binary"(%admit, %one) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%sum, %write, %admit, %allow, %one) : (!b9, !b1, !b1, !b1, !b1) -> ()
    }) {name = "increment", occurrence = #writer, ac.required_checks = [{id = #id, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "sync", polarity = "high"}], fire_result = 2 : i64, controls = [{kind = "callable_guard", id = #id, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = []}} : (!b1, !b1, !b9, !b1) -> (!b9, !b1, !b1, !b1, !b1)
    "gfir.state.update"(%q, %next, %enabled) : (!b9, !b9, !b1) -> ()
    "ac.expect"(%raw, %reached, %clk, %rst) {kind = "guard_known", location = #span, ac.check_id = #id} : (!b1, !b1, !b1, !b1) -> ()
)ir";
  else
    text += R"ir(
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "held", occurrence = #occ} : (!b1, !b1, !b9) -> !b9
)ir";
  text += R"ir(
    "ac.yield"(%q) : (!b9) -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !b9, input_names = ["enable", "pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.origin = #occ, ac.declaration_role = "definition"
)ir";
  if (routed)
    text += ", ac.domain_params = [\"D\"], ac.default_domain = \"D\", ac.domain_clock_inputs = {D = 1 : i32}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = [{registration = #writer, ordinary_results = 0 : i64, candidates = [{state = #state_id, data = 0 : i64, enable = 1 : i64}]}]}";
  text += "} : () -> ()\n";
  if (routed)
    text += R"ir(
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
)ir";
  return text + "}\n";
}

std::string print(mlir::Operation *operation) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  operation->print(stream, mlir::OpPrintingFlags().enableDebugInfo());
  return result + '\n';
}

// These substitutions select versions of a hand-authored fixture. They never
// inspect emitted code or derive expected values from DUT output.
std::string asyncNegativeStateIR() {
  auto text = stateIR(true);
  auto replace = [&](const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    EXPECT_NE(position, std::string::npos);
    if (position != std::string::npos)
      text.replace(position, from.size(), to);
  };
  replace("edge = \"posedge\", reset_kind = \"sync\", reset_polarity = \"high\"",
          "edge = \"negedge\", reset_kind = \"async\", reset_polarity = \"low\"");
  replace("edge = \"posedge\", resets = [{capture = 1 : i64, kind = \"sync\", polarity = \"high\"}]",
          "edge = \"negedge\", resets = [{capture = 1 : i64, kind = \"async\", polarity = \"low\"}]");
  return text;
}

class StateResetValueTest : public ::testing::Test {
protected:
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect, gf::GFIRDialect>();
    llvm::SmallString<256> path;
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("state-reset-value", path));
    directory = fs::canonical(path.str().str());
  }
  void TearDown() override {
    if (!directory.empty())
      fs::remove_all(directory);
  }
  void publishSaved(const std::string &sourceText, const std::string &routedText,
                    const char *exportVariable,
                    bool checkFifoIntentAuthentication = false) {
    auto source = mlir::parseSourceString<mlir::ModuleOp>(sourceText, &context);
    auto routed = mlir::parseSourceString<mlir::ModuleOp>(routedText, &context);
    ASSERT_TRUE(source);
    ASSERT_TRUE(routed);
    auto header = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(source->getOperation()->clone()));
    mlir::PassManager manager(&context);
    manager.addPass(acir::createExtractSourceInterfacePass());
    ASSERT_TRUE(mlir::succeeded(manager.run(*header)));
    const auto original = print(routed->getOperation());
    AnchorSourceProvenance provenance{{*source, *header},
        anchorSha256(print(source->getOperation())), print(header->getOperation())};
    ASSERT_TRUE(mlir::succeeded(writeAnchorArtifacts(
        *routed, {provenance}, directory.string(), [&] {
          return mlir::emitError(mlir::UnknownLoc::get(&context));
        })));
    EXPECT_EQ(print(routed->getOperation()), original);
    source = {}; header = {}; routed = {};
    mlir::MLIRContext fresh;
    fresh.loadDialect<ac::ACIRDialect, gf::GFIRDialect>();
    auto error = [&] { return mlir::emitError(mlir::UnknownLoc::get(&fresh)); };
    auto loaded = loadVerifiedAnchor(directory.string(), fresh, error);
    ASSERT_TRUE(mlir::succeeded(loaded));
    const auto before = print(loaded->package->getOperation());
    auto final = refineVerifiedAnchor(*loaded, error);
    ASSERT_TRUE(mlir::succeeded(final));
    EXPECT_EQ(print(loaded->package->getOperation()), before);
    if (checkFifoIntentAuthentication) {
      ASSERT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(**final)));
      ASSERT_TRUE(mlir::succeeded(ac::verifyFinalHardwareCDCComponents(**final)));
      const auto unchanged = print((*final)->getOperation());
      auto mutated = mlir::OwningOpRef<mlir::ModuleOp>(
          mlir::cast<mlir::ModuleOp>((*final)->getOperation()->clone()));
      mlir::Operation *fifo = nullptr;
      mutated->walk([&](ac::CDCRefinementOp witness) {
        if (witness.getLogicalKind() == "async_fifo") fifo = witness;
      });
      ASSERT_NE(fifo, nullptr);
      auto owners = ac::getVerifiedCDCComponentOwners(*mutated);
      ASSERT_TRUE(mlir::succeeded(owners));
      // FIFO witness pins are clocks, whole reset, payload, push/pop intents,
      // then the three logical outputs. Keep the reconstructed public graph
      // and witness mutually consistent while removing transaction authority.
      const auto originalIntent = fifo->getOperand(4);
      mlir::OpBuilder builder(&fresh);
      builder.setInsertionPointToStart(fifo->getBlock());
      mlir::OperationState constant(fifo->getLoc(), "ac.bits.constant4");
      constant.addTypes(originalIntent.getType());
      constant.addAttribute("pattern", builder.getStringAttr("1"));
      auto unauthorizedIntent = builder.create(constant)->getResult(0);
      unsigned replacedEdges = 0;
      for (const auto &[node, owner] : *owners) {
        if (owner != fifo) continue;
        for (auto &operand : node->getOpOperands()) {
          if (operand.get() != originalIntent) continue;
          operand.set(unauthorizedIntent);
          ++replacedEdges;
        }
      }
      ASSERT_GT(replacedEdges, 0u);
      fifo->setOperand(4, unauthorizedIntent);
      {
        mlir::ScopedDiagnosticHandler expectedFailure(
            &fresh, [](mlir::Diagnostic &) { return mlir::success(); });
        EXPECT_TRUE(mlir::failed(ac::verifyFinalHardwareCDCComponents(*mutated)));
        EXPECT_TRUE(mlir::failed(ac::verifyHardwarePackage(*mutated)));
      }
      EXPECT_EQ(print((*final)->getOperation()), unchanged);
    }
    if (const char *output = std::getenv(exportVariable)) {
      ASSERT_FALSE(fs::exists(output));
      fs::copy(directory, output, fs::copy_options::recursive);
    }
  }
  mlir::MLIRContext context;
  fs::path directory;
};

TEST_F(StateResetValueTest, SavedLogicalStateRetainsDistinctInitializationAndReset) {
  {
    auto source = mlir::parseSourceString<mlir::ModuleOp>(stateIR(false), &context);
    auto routed = mlir::parseSourceString<mlir::ModuleOp>(stateIR(true), &context);
    ASSERT_TRUE(source);
    ASSERT_TRUE(routed);
    auto header = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(source->getOperation()->clone()));
    mlir::PassManager manager(&context);
    manager.addPass(acir::createExtractSourceInterfacePass());
    ASSERT_TRUE(mlir::succeeded(manager.run(*header)));
    ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
    const auto before = print(routed->getOperation());
    AnchorSourceProvenance provenance{{*source, *header},
                                      anchorSha256(print(source->getOperation())),
                                      print(header->getOperation())};
    ASSERT_TRUE(mlir::succeeded(writeAnchorArtifacts(
        *routed, {provenance}, directory.string(), [&] {
          return mlir::emitError(mlir::UnknownLoc::get(&context));
        })));
    EXPECT_EQ(print(routed->getOperation()), before);
  }

  // Reload and refine without the original source/header/routing objects.
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, gf::GFIRDialect>();
  auto error = [&] { return mlir::emitError(mlir::UnknownLoc::get(&fresh)); };
  auto loaded = loadVerifiedAnchor(directory.string(), fresh, error);
  ASSERT_TRUE(mlir::succeeded(loaded));
  gf::StateOp state;
  loaded->package->walk([&](gf::StateOp op) { state = op; });
  ASSERT_TRUE(state);
  ASSERT_EQ(state->getNumOperands(), 4u);
  auto init = state->getOperand(2).getDefiningOp<ac::BitsConstantOp>();
  auto reset = state->getOperand(3).getDefiningOp<ac::BitsConstantOp>();
  ASSERT_TRUE(init);
  ASSERT_TRUE(reset);
  // Compare with literal expectations, not values extracted from final IR.
  ac::HardwareBindings bindings;
  bindings.owner = state->getParentOp();
  ac::HardwareAnalysis analysis(*loaded->package);
  auto initialValue = analysis.evaluateStatic(
      mlir::cast<ac::StaticExprAttr>(init->getAttr("value")), bindings, state);
  auto resetValue = analysis.evaluateStatic(
      mlir::cast<ac::StaticExprAttr>(reset->getAttr("value")), bindings, state);
  ASSERT_TRUE(mlir::succeeded(initialValue));
  ASSERT_TRUE(mlir::succeeded(resetValue));
  EXPECT_EQ(mlir::cast<ac::MathIntAttr>(*initialValue).getCanonicalValue(), "5");
  EXPECT_EQ(mlir::cast<ac::MathIntAttr>(*resetValue).getCanonicalValue(), "0");
  EXPECT_EQ(init.getResult().getType(), state.getQ().getType());
  EXPECT_EQ(reset.getResult().getType(), state.getQ().getType());
  const auto before = print(loaded->package->getOperation());
  auto final = refineVerifiedAnchor(*loaded, error);
  ASSERT_TRUE(mlir::succeeded(final));
  EXPECT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(**final)));
  EXPECT_EQ(print(loaded->package->getOperation()), before);

  // An optional test-only destination lets public CLI consumers reuse this
  // actual publisher output; no manifest is synthesized by a Python test.
  if (const char *output = std::getenv("PYCIRCUIT_TEST_STATE_RESET_ANCHOR")) {
    ASSERT_FALSE(fs::exists(output));
    fs::copy(directory, output, fs::copy_options::recursive);
  }

  // A well-typed but semantically wrong reset must fail correspondence proof.
  // The native-resource contract normalizes the transaction reset mask before
  // state refinement; isolate this proof boundary so an earlier pass cannot
  // reject it for a different reason.
  ac::HardwareSourceCheckLimits limits;
  ac::detail::SourceCheckBudget maskBudget(limits);
  auto normalized = acir::detail::materializeTransactionResetMasks(
      *loaded->package, maskBudget);
  ASSERT_TRUE(mlir::succeeded(normalized));
  ac::detail::SourceCheckBudget loweringBudget(limits);
  auto stateFinal = acir::detail::materializeHardwareState(**normalized, loweringBudget);
  ASSERT_TRUE(mlir::succeeded(stateFinal));
  ac::detail::SourceCheckBudget baselineProof(limits);
  ASSERT_TRUE(mlir::succeeded(acir::detail::verifyHardwareStateMaterialization(
      **normalized, **stateFinal, baselineProof)));
  ac::InstanceOp storage;
  (*stateFinal)->walk([&](ac::InstanceOp op) { storage = op; });
  ASSERT_TRUE(storage);
  ASSERT_EQ(storage->getNumOperands(), 6u);
  storage->setOperand(5, storage->getOperand(4));
  ac::detail::SourceCheckBudget budget(limits);
  mlir::ScopedDiagnosticHandler expectedFailure(
      &fresh, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(acir::detail::verifyHardwareStateMaterialization(
      **normalized, **stateFinal, budget)));
  EXPECT_EQ(print(loaded->package->getOperation()), before);
}

TEST_F(StateResetValueTest, TableResetValueRetainsItsTypedLanesThroughRefinement) {
  // Reuse only static identities, not a generated state/refinement graph.
  const auto scalar = stateIR(true);
  auto text = scalar.substr(0, scalar.find("module attributes"));
  text += R"ir(
#extent = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = #occ, location = #span}>
!lanes = !ac.table<[#extent], !b9>
module attributes {ac.stage = "routed", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1):
    %five = "ac.bits.constant"() {value = #w5} : () -> !b9
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b9
    %initial = "ac.table.splat"(%five) {shape = [#extent]} : (!b9) -> !lanes
    %reset_value = "ac.table.splat"(%zero) {shape = [#extent]} : (!b9) -> !lanes
    %false = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "gfir.state"(%clk, %rst, %initial, %reset_value) {instance_name = "held", occurrence = #occ, domain = "D", operandSegmentSizes = array<i32: 1, 1, 1, 1>, init_kind = "explicit", edge = "posedge", reset_kind = "sync", reset_polarity = "high"} : (!b1, !b1, !lanes, !lanes) -> !lanes
    "gfir.state.update"(%q, %q, %false) : (!lanes, !lanes, !b1) -> ()
    "ac.yield"(%q) : (!lanes) -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !lanes, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.domain_params = ["D"], ac.default_domain = "D", ac.domain_clock_inputs = {D = 0 : i32}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = []}} : () -> ()
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
  auto routed = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
  ASSERT_TRUE(routed);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
  gf::StateOp state;
  routed->walk([&](gf::StateOp op) { state = op; });
  ASSERT_TRUE(state);
  const auto logicalType = state.getQ().getType();
  const auto before = print(routed->getOperation());
  ac::HardwareSourceCheckLimits limits;
  ac::detail::SourceCheckBudget loweringBudget(limits);
  auto final = acir::detail::materializeHardwareState(*routed, loweringBudget);
  ASSERT_TRUE(mlir::succeeded(final));
  ac::detail::SourceCheckBudget proofBudget(limits);
  ASSERT_TRUE(mlir::succeeded(acir::detail::verifyHardwareStateMaterialization(
      *routed, **final, proofBudget)));
  ac::CollectionOp storage;
  (*final)->walk([&](ac::CollectionOp op) { storage = op; });
  ASSERT_TRUE(storage);
  EXPECT_EQ(storage.getResult(0).getType(), logicalType);
  ASSERT_EQ(storage->getNumOperands(), 6u);
  EXPECT_EQ(storage->getOperand(4).getType(), logicalType);
  EXPECT_EQ(storage->getOperand(5).getType(), logicalType);
  EXPECT_NE(storage->getOperand(4), storage->getOperand(5));
  EXPECT_EQ(print(routed->getOperation()), before);
}

TEST_F(StateResetValueTest, NativeResourceStatePoliciesSurviveIndependentRefinement) {
  const auto scalar = stateIR(true);
  const auto preamble = scalar.substr(0, scalar.find("module attributes"));
  struct Policy {
    const char *edge;
    const char *kind;
    const char *polarity;
    bool initial;
  };
  const Policy cases[] = {
      {"posedge", "sync", "high", true},
      {"negedge", "sync", "low", true},
      {"posedge", "async", "low", true},
      {"negedge", "async", "high", false},
      {"posedge", "none", "", true},
      {"negedge", "none", "", false},
  };
  for (const auto &policy : cases) {
    SCOPED_TRACE(std::string(policy.edge) + "/" + policy.kind);
    const bool reset = std::string(policy.kind) != "none";
    std::string inputs = "%clk", types = "!b1";
    if (reset) { inputs += ", %rst"; types += ", !b1"; }
    if (policy.initial) { inputs += ", %five"; types += ", !b9"; }
    if (reset) { inputs += ", %zero"; types += ", !b9"; }
    std::string text = preamble + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1):
    %five = "ac.bits.constant"() {value = #w5} : () -> !b9
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b9
    %false = "ac.bits.constant"() {value = #w0} : () -> !b1
)ir";
    text += "    %q = \"gfir.state\"(" + inputs +
            ") {instance_name = \"held\", occurrence = #occ, domain = \"D\", "
            "operandSegmentSizes = array<i32: 1, " +
            std::to_string(reset) + ", " + std::to_string(policy.initial) +
            ", " + std::to_string(reset) + ">, edge = \"" + policy.edge +
            "\", reset_kind = \"" + policy.kind + "\", init_kind = \"" +
            (policy.initial ? "explicit" : "unspecified") + "\"";
    if (reset)
      text += ", reset_polarity = \"" + std::string(policy.polarity) + "\"";
    text += "} : (" + types + ") -> !b9\n";
    text += R"ir(
    "gfir.state.update"(%q, %q, %false) : (!b9, !b9, !b1) -> ()
    "ac.yield"(%q) : (!b9) -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !b9, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.domain_params = ["D"], ac.default_domain = "D", ac.domain_clock_inputs = {D = 0 : i32}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = []}} : () -> ()
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

)ir";
    auto routed = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
    ASSERT_TRUE(routed);
    ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
    gf::StateOp state;
    routed->walk([&](gf::StateOp op) { state = op; });
    ASSERT_TRUE(state);
    EXPECT_EQ(static_cast<bool>(state.getRst()), reset);
    EXPECT_EQ(static_cast<bool>(state.getInitial()), policy.initial);
    EXPECT_EQ(static_cast<bool>(state.getResetValue()), reset);
    const auto before = print(routed->getOperation());
    ac::HardwareSourceCheckLimits limits;
    ac::detail::SourceCheckBudget lowering(limits);
    auto final = acir::detail::materializeHardwareState(*routed, lowering);
    ASSERT_TRUE(mlir::succeeded(final));
    ac::detail::SourceCheckBudget proof(limits);
    EXPECT_TRUE(mlir::succeeded(acir::detail::verifyHardwareStateMaterialization(
        *routed, **final, proof)));
    EXPECT_EQ(print(routed->getOperation()), before);
  }
}

TEST_F(StateResetValueTest, PublishAsyncNegativeStateForIndependentConsumers) {
  publishSaved(stateIR(false), asyncNegativeStateIR(),
               "PYCIRCUIT_TEST_ASYNC_STATE_ANCHOR");
}

#include "StateResetCDCFixture.inc"

TEST_F(StateResetValueTest, UnusedNativeCDCDeclarationIsRemovedBeforeFinal) {
  auto routed = mlir::parseSourceString<mlir::ModuleOp>(
      withNativeCDCImport(stateIR(true), "async_reg"), &context);
  ASSERT_TRUE(routed);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
  const auto before = print(routed->getOperation());
  auto final = acir::detail::refineRoutedHardware(*routed);
  ASSERT_TRUE(mlir::succeeded(final));
  EXPECT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(**final)));
  (*final)->walk([&](ac::ModuleImportOp imported) {
    EXPECT_NE(imported.getSymName(), "pycircuit.__builtins__.async_reg");
    EXPECT_NE(imported.getSymName(), "pycircuit.__builtins__.async_fifo");
  });
  (*final)->walk([&](ac::InstanceOp instance) {
    EXPECT_NE(instance.getCallee(), "pycircuit.__builtins__.async_reg");
    EXPECT_NE(instance.getCallee(), "pycircuit.__builtins__.async_fifo");
  });
  EXPECT_EQ(print(routed->getOperation()), before);
}

TEST_F(StateResetValueTest, NativeCDCRejectsMalformedCanonicalDeclaration) {
  auto routed = mlir::parseSourceString<mlir::ModuleOp>(asyncRegSystemIR(true), &context);
  ASSERT_TRUE(routed);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
  ac::ModuleImportOp declaration;
  routed->walk([&](ac::ModuleImportOp imported) {
    if (imported.getSymName() == "pycircuit.__builtins__.async_reg")
      declaration = imported;
  });
  ASSERT_TRUE(declaration);
  // Keep the signature, kind and instance well-typed while violating the
  // canonical endpoint schema. A primitive label cannot authenticate its pins.
  mlir::OpBuilder builder(&context);
  llvm::SmallVector<mlir::Attribute> names(
      declaration.getInputNames().begin(), declaration.getInputNames().end());
  names[0] = builder.getStringAttr("unrecognized_source_clock");
  declaration->setAttr("input_names", builder.getArrayAttr(names));
  mlir::ScopedDiagnosticHandler expectedFailure(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(ac::verifyRoutedPackage(*routed)));
}

TEST_F(StateResetValueTest, SupersededExperimentalRoutedContractRejects) {
  auto routed = mlir::parseSourceString<mlir::ModuleOp>(stateIR(true), &context);
  ASSERT_TRUE(routed);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
  mlir::OpBuilder builder(&context);
  (*routed)->setAttr("ac.routed_contract", builder.getI64IntegerAttr(6));
  mlir::ScopedDiagnosticHandler expectedFailure(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(ac::verifyRoutedPackage(*routed)));
}

std::string mixedResetIR(bool routed) {
  auto text = routed ? asyncNegativeStateIR() : stateIR(false);
  auto replace = [&](const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    EXPECT_NE(position, std::string::npos);
    if (position != std::string::npos) text.replace(position, from.size(), to);
  };
  const std::string declaration =
      "#retained = {site = {definition = @\"reset_value.fixture.Held\", ast_path = [{kind = \"index\", value = 7 : i64}]}, expansion = []}\n"
      "#retained_id = {owner = {instance_path = []}, declaration = #retained, element = []}\n";
  const auto module = text.find("module attributes");
  text.insert(module, declaration);
  if (routed) {
    replace("    %next, %enabled",
        "    %retained = \"gfir.state\"(%clk, %initial) {instance_name = \"retained\", occurrence = #retained, domain = \"D\", operandSegmentSizes = array<i32: 1, 0, 1, 0>, init_kind = \"explicit\", edge = \"negedge\", reset_kind = \"none\"} : (!b1, !b9) -> !b9\n    %next, %enabled");
    replace("    \"ac.expect\"",
        "    \"gfir.state.update\"(%retained, %retained_next, %retained_enabled) : (!b9, !b9, !b1) -> ()\n    \"ac.expect\"");
    replace("%next, %enabled, %fire, %raw, %reached =",
            "%next, %enabled, %retained_next, %retained_enabled, %fire, %raw, %reached =");
    replace("\"ac.yield\"(%sum, %write, %admit, %allow, %one) : (!b9, !b1, !b1, !b1, !b1)",
            "\"ac.yield\"(%sum, %write, %sum, %write, %admit, %allow, %one) : (!b9, !b1, !b9, !b1, !b1, !b1, !b1)");
    replace("-> (!b9, !b1, !b1, !b1, !b1)",
            "-> (!b9, !b1, !b9, !b1, !b1, !b1, !b1)");
    replace("fire_result = 2 : i64", "fire_result = 4 : i64");
    replace("states = [#occ]", "states = [#occ, #retained]");
    replace("candidates = [{state = #state_id, data = 0 : i64, enable = 1 : i64}]",
        "candidates = [{state = #state_id, data = 0 : i64, enable = 1 : i64}, {state = #retained_id, data = 2 : i64, enable = 3 : i64}]");
  } else {
    replace("    \"ac.yield\"(%q)",
        "    %retained = \"ac.state\"(%clk, %rst, %initial) {instance_name = \"retained\", occurrence = #retained} : (!b1, !b1, !b9) -> !b9\n    \"ac.yield\"(%q)");
  }
  replace("\"ac.yield\"(%q) : (!b9)", "\"ac.yield\"(%q, %retained) : (!b9, !b9)");
  replace("function_type = (!b1, !b1, !b1) -> !b9",
          "function_type = (!b1, !b1, !b1) -> (!b9, !b9)");
  replace("output_names = [\"q\"]", "output_names = [\"q\", \"retained\"]");
  return text;
}

TEST_F(StateResetValueTest, PublishMixedResetStateForIndependentConsumers) {
  publishSaved(mixedResetIR(false), mixedResetIR(true),
               "PYCIRCUIT_TEST_MIXED_STATE_ANCHOR");
}

} // namespace
} // namespace acir::compiler
