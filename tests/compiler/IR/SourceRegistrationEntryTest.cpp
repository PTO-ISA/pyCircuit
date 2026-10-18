#include "Dialect/ACIR/SourceTransactionSummary.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "gtest/gtest.h"
#include <functional>

namespace acir::ac {
namespace {
using namespace mlir;
std::string registrationFixture() {
  std::string text;
  for (unsigned value : {0u, 1u, 8u})
    text += "#w" + std::to_string(value) +
        " = #ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", value = #ac.math_int<" +
        std::to_string(value) + ">}, origin = {site = {definition = @Owner, ast_path = []}, expansion = []}, location = {path = \"entry.py\", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>\n";
  text += R"ir(
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
#byte = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
#span = {path = "entry.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#decl = {site = {definition = @Owner, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#reg = {site = {definition = @Owner, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
module {
  "ac.module"() ({
  ^bb0(%seed: !b8, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    "ac.method"(%yes) ({
    ^bb0(%left: !b8, %right: !b8, %guard: !b1):
      "ac.yield"(%guard) : (!b1) -> ()
    }, {
    ^bb0(%left: !b8, %right: !b8, %guard: !b1):
      "ac.yield"(%left, %right) : (!b8, !b8) -> ()
    }) {sym_name = "shared", function_type = (!b8, !b8) -> (!b8, !b8), argument_names = ["left", "right"], ac.parameters = [{name = "left", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #decl, location = #span}, {name = "right", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #decl, location = #span}], ac.result_constraints = [#byte, #byte], ac.return_form = "tuple", ac.guard_location = #span, occurrence = #decl} : (!b1) -> ()
    %a, %b = "ac.transaction"(%clk, %rst, %seed, %seed) ({
    ^bb0(%left: !b8, %right: !b8):
      %guard_true = "ac.bits.constant"() {value = #w1} : () -> !b1
      "ac.yield"(%guard_true) : (!b1) -> ()
    }, {
    ^bb0(%left: !b8, %right: !b8):
      %path_true = "ac.bits.constant"() {value = #w1} : () -> !b1
      %a, %b = "ac.method.call"(%path_true, %left, %right) {callee = @Owner::@shared, instance_path = [], occurrence = #reg} : (!b1, !b8, !b8) -> (!b8, !b8)
      "ac.yield"(%a, %b) : (!b8, !b8) -> ()
    }) {name = "run", declaration = #decl, occurrence = #reg, ac.guard_location = #span, ac.registration_entry} : (!b1, !b1, !b8, !b8) -> (!b8, !b8)
    "ac.yield"(%a, %b) : (!b8, !b8) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "entry.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1) -> (!b8, !b8), input_names = ["seed", "pyc_clk", "pyc_rst"], output_names = ["a", "b"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
}
)ir";
  return text;
}
class SourceRegistrationEntryTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder builder{&context};
  void SetUp() override { context.loadDialect<ACIRDialect>(); }
  auto parse() {
    return parseSourceString<mlir::ModuleOp>(registrationFixture(),
                                            ParserConfig(&context, false));
  }
  ModuleOp owner(mlir::ModuleOp file) {
    return cast<ModuleOp>(SymbolTable::lookupSymbolIn(file, "Owner"));
  }
  SourceTransactionOp transaction(mlir::ModuleOp file) {
    return *owner(file).getBody().front().getOps<SourceTransactionOp>().begin();
  }
  SourceMethodCallOp call(SourceTransactionOp tx) {
    return *tx.getBody().front().getOps<SourceMethodCallOp>().begin();
  }
};
TEST_F(SourceRegistrationEntryTest, DuplicateActualsAndLexicalCapturesRemainDistinct) {
  auto file = parse(); ASSERT_TRUE(file);
  ASSERT_TRUE(succeeded(verify(*file)));
  auto tx = transaction(*file);
  auto checked = getSourceRegistrationEntry(tx);
  ASSERT_TRUE(succeeded(checked)); ASSERT_TRUE(checked->has_value());
  EXPECT_EQ((**checked).argumentCaptureOrdinals, (SmallVector<unsigned>{0, 1}));
  EXPECT_EQ(tx.getCaptures()[0], tx.getCaptures()[1]);
  EXPECT_EQ((**checked).method.getCaptures().size(), 1u);
  tx->removeAttr("ac.registration_entry");
  checked = getSourceRegistrationEntry(tx);
  ASSERT_TRUE(succeeded(checked)); EXPECT_FALSE(checked->has_value());
}
TEST_F(SourceRegistrationEntryTest, RejectsForgedTransparentBridgeShapes) {
  using Mutation = std::function<void(SourceTransactionOp)>;
  SmallVector<Mutation> mutations{
    [&](auto tx) { tx->setAttr("ac.registration_entry", builder.getBoolAttr(true)); },
    [&](auto tx) { call(tx).setOccurrenceAttr(tx.getDeclaration()); },
    [&](auto tx) { call(tx).setInstancePathAttr(builder.getStrArrayAttr({"child"})); },
    [&](auto tx) { call(tx)->setAttr("ac.formal_owner_bindings", builder.getArrayAttr({})); },
    [&](auto tx) { call(tx)->setOperand(1, tx.getBody().front().getArgument(1)); },
    [&](auto tx) { auto yield = cast<YieldOp>(tx.getBody().front().back());
                  yield->setOperand(0, call(tx).getResult(1)); },
    [&](auto tx) { tx->setOperand(0, tx.getRst()); },
    [&](auto tx) { tx.setDeclarationAttr(tx.getOccurrence()); },
    [&](auto tx) { auto constant = cast<BitsConstantOp>(tx.getGuard().front().front());
                  OpBuilder insertion(constant); insertion.clone(*constant); },
  };
  for (auto &mutate : mutations) {
    auto file = parse(); ASSERT_TRUE(file);
    auto tx = transaction(*file); mutate(tx);
    ScopedDiagnosticHandler silence(&context, [](Diagnostic &) { return success(); });
    EXPECT_TRUE(failed(getSourceRegistrationEntry(tx)));
  }
}
TEST_F(SourceRegistrationEntryTest, PublicationRetainsOnlyLocalMethodReference) {
  auto file = parse(); ASSERT_TRUE(file);
  auto module = owner(*file);
  auto summary = detail::deriveSourceTransactionSummary(module);
  ASSERT_TRUE(succeeded(summary));
  ASSERT_TRUE(succeeded(verifySourceTransactionSummary(module, *summary)));
  auto rows = summary->getAs<ArrayAttr>("autonomous");
  ASSERT_EQ(rows.size(), 1u);
  auto row = cast<DictionaryAttr>(rows[0]);
  EXPECT_EQ(row.getAs<SymbolRefAttr>("registration_entry"),
            call(transaction(*file)).getCalleeAttr());
  EXPECT_EQ(summary->getAs<ArrayAttr>("methods").size(), 1u);
  EXPECT_EQ(row.getAs<ArrayAttr>("body_effects").size(), 1u);
  NamedAttrList badRow(row);
  badRow.set("registration_entry", SymbolRefAttr::get(&context, "Other",
      {FlatSymbolRefAttr::get(&context, "shared")}));
  NamedAttrList badSummary(*summary);
  badSummary.set("autonomous", builder.getArrayAttr({badRow.getDictionary(&context)}));
  ScopedDiagnosticHandler silence(&context, [](Diagnostic &) { return success(); });
  EXPECT_TRUE(failed(verifySourceTransactionSummary(
      module, badSummary.getDictionary(&context))));
}
TEST_F(SourceRegistrationEntryTest, MarkerCannotAppearOnFinalOrOtherOperations) {
  for (bool final : {false, true}) {
    auto file = parse(); ASSERT_TRUE(file);
    if (final)
      (*file)->setAttr("ac.stage", builder.getStringAttr("final"));
    else
      owner(*file)->setAttr("ac.registration_entry", builder.getUnitAttr());
    ScopedDiagnosticHandler silence(&context, [](Diagnostic &) { return success(); });
    EXPECT_TRUE(failed(verify(*file)));
  }
}
} // namespace
} // namespace acir::ac
