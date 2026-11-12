#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Analysis/LogicalStateValidation.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OperationSupport.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace {

std::string preamble() {
  std::string text = R"ir(
#owner = {package = "anchor", path = "anchor.py"}
#occ = {site = {definition = @Counter, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#span = {path = "anchor.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
)ir";
  for (unsigned value : {1u, 7u})
    text += "#w" + std::to_string(value) +
            " = #ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<" +
            std::to_string(value) + ">}, origin = #occ, location = #span}>\n";
  return text + R"ir(
!b1 = !ac.bits<#w1>
!b7 = !ac.bits<#w7>
)ir";
}

// The routed fixture carries the GFIR spellings; the source ac.state class
// never implements the routed interface after the migration.
std::string routedPackage() {
  return preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 4 : i64} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %candidate: !b7, %intent: !b1):
    %initial = "ac.bits.constant"() {value = #w7} : () -> !b7
    %q = "gfir.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #occ} : (!b1, !b1, !b7) -> !b7
    "gfir.state.update"(%q, %candidate, %intent) : (!b7, !b7, !b1) -> ()
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b7, !b1) -> !b7, input_names = ["clk", "rst", "candidate", "intent"], output_names = ["q"]} : () -> ()
}
)ir";
}

class LogicalStateInterfacesTest : public ::testing::Test {
protected:
  LogicalStateInterfacesTest() {
    context.loadDialect<acir::ac::ACIRDialect>();
    context.loadDialect<acir::gfir::GFIRDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef text) {
    return mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
  }

  std::string diagnostics;
  mlir::MLIRContext context;
};

TEST_F(LogicalStateInterfacesTest, StructureVerifiersAcceptWellFormedPair) {
  auto file = parse(routedPackage());
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  acir::LogicalStateOpInterface state;
  acir::LogicalStateUpdateOpInterface update;
  file->walk([&](acir::LogicalStateOpInterface found) { state = found; });
  file->walk([&](acir::LogicalStateUpdateOpInterface found) { update = found; });
  ASSERT_TRUE(static_cast<bool>(state));
  ASSERT_TRUE(static_cast<bool>(update));
  EXPECT_TRUE(mlir::succeeded(acir::verifyLogicalStateStructure(state.getOperation())));
  EXPECT_TRUE(mlir::succeeded(acir::verifyLogicalStateUpdateStructure(update.getOperation())));
}

// The interface implementer is the routed GFIR class alone; a source ac.state
// declaration exposes neither contract even in a routed module graph.
TEST_F(LogicalStateInterfacesTest, SourceStateIsNotAnInterfaceImplementer) {
  auto file = parse(preamble() + R"ir(
module attributes {ac.stage = "routed", ac.routed_contract = 4 : i64} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %candidate: !b7, %intent: !b1):
    %initial = "ac.bits.constant"() {value = #w7} : () -> !b7
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #occ} : (!b1, !b1, !b7) -> !b7
    "gfir.state.update"(%q, %candidate, %intent) : (!b7, !b7, !b1) -> ()
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "Counter", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b7, !b1) -> !b7, input_names = ["clk", "rst", "candidate", "intent"], output_names = ["q"]} : () -> ()
}
)ir");
  ASSERT_TRUE(file);
  acir::LogicalStateOpInterface state;
  (*file)->walk([&](acir::LogicalStateOpInterface found) { state = found; });
  EXPECT_FALSE(static_cast<bool>(state));
}

mlir::Value stateQ(mlir::OwningOpRef<mlir::ModuleOp> &file) {
  acir::LogicalStateOpInterface state;
  (*file)->walk([&](acir::LogicalStateOpInterface found) { state = found; });
  return state.getStateQ();
}

mlir::Value intentOf(mlir::OwningOpRef<mlir::ModuleOp> &file) {
  acir::LogicalStateUpdateOpInterface update;
  (*file)->walk(
      [&](acir::LogicalStateUpdateOpInterface found) { update = found; });
  return update.getStateWriteIntent();
}

TEST_F(LogicalStateInterfacesTest, StructureVerifiersRejectWrongKind) {
  auto file = parse(routedPackage());
  ASSERT_TRUE(file);
  acir::LogicalStateOpInterface state;
  acir::LogicalStateUpdateOpInterface update;
  file->walk([&](acir::LogicalStateOpInterface found) { state = found; });
  file->walk([&](acir::LogicalStateUpdateOpInterface found) { update = found; });
  ASSERT_TRUE(static_cast<bool>(state));
  ASSERT_TRUE(static_cast<bool>(update));
  mlir::ScopedDiagnosticHandler capture(
      &context, [&](mlir::Diagnostic &) { return mlir::success(); });
  // Wrong-kind rejection: each verifier returns at its interface-cast
  // check when handed the other contract's op. Shape-level arms are
  // covered separately via generic malformed construction below.
  EXPECT_TRUE(mlir::failed(acir::verifyLogicalStateStructure(update.getOperation())));
  EXPECT_TRUE(mlir::failed(acir::verifyLogicalStateUpdateStructure(state.getOperation())));
}

TEST_F(LogicalStateInterfacesTest, BoundBindingVerifiesWellFormedUpdate) {
  auto file = parse(routedPackage());
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)))
      << "fixture must be a legal routed package";
  acir::LogicalStateUpdateOpInterface update;
  file->walk([&](acir::LogicalStateUpdateOpInterface found) { update = found; });
  ASSERT_TRUE(static_cast<bool>(update));
  // The bound helper performs the full extracted verification chain,
  // including the structural preflight on the update contract.
  EXPECT_TRUE(mlir::succeeded(
      acir::ac::verifyLogicalStateUpdateBinding(update)));
}

TEST_F(LogicalStateInterfacesTest, AdmissionPairingFindsSingleUpdate) {
  auto file = parse(routedPackage());
  ASSERT_TRUE(file);
  acir::LogicalStateOpInterface state;
  file->walk([&](acir::LogicalStateOpInterface found) { state = found; });
  ASSERT_TRUE(static_cast<bool>(state));
  acir::ac::HardwareSourceCheckLimits limits;
  acir::ac::detail::SourceCheckBudget budget{limits};
  auto paired =
      acir::ac::admissionPairLogicalStateUpdate(state, budget);
  ASSERT_TRUE(mlir::succeeded(paired));
  EXPECT_EQ(paired->getUpdatedStateQ(), state.getStateQ());
}

TEST_F(LogicalStateInterfacesTest, StructureVerifiersRejectMalformedShapes) {
  auto file = parse(routedPackage());
  ASSERT_TRUE(file);
  mlir::OpBuilder builder(&file->getBodyRegion());
  mlir::Location unknown = mlir::UnknownLoc::get(&context);

  // Generic creation bypasses the ODS builders, so the op kind still
  // implements the interface while the actual shape violates the contract.
  mlir::OperationState badUpdate(unknown, "gfir.state.update");
  auto *twoOperand = builder.create(badUpdate);
  mlir::ScopedDiagnosticHandler capture(
      &context, [&](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(
      acir::verifyLogicalStateUpdateStructure(twoOperand)));

  mlir::OperationState regionUpdate(unknown, "gfir.state.update");
  regionUpdate.addOperands({stateQ(file), stateQ(file), intentOf(file)});
  regionUpdate.addRegion();
  auto *withRegion = builder.create(regionUpdate);
  EXPECT_TRUE(mlir::failed(
      acir::verifyLogicalStateUpdateStructure(withRegion)));

  mlir::OperationState noResultState(unknown, "gfir.state");
  auto *emptyState = builder.create(noResultState);
  EXPECT_TRUE(mlir::failed(
      acir::verifyLogicalStateStructure(emptyState)));
  EXPECT_TRUE(mlir::failed(mlir::verify(twoOperand)));
}

} // namespace
