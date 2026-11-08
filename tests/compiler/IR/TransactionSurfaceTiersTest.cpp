// B2.5-1 acceptance (r8/r9 review batch 1): one minimal transaction design
// proves the transaction-surface tier contract. The symbolic template
// publishes standalone at the source stage (deferred obligations held), a
// definite payload disagreement is already rejected at that same source
// stage, the legal actual binding completes routing, and the illegal actual
// binding is rejected at the routing preparation with the Method-family
// diagnostic. The same design carries a Table-indexed state: the index is
// the M-bit truncation of the value formal, so the checked index range is
// the complete unsigned range of the unbound M and the Table-index proof
// closes only under a legal binding.
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Conversion/ACIRToGFIR.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <memory>
#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

// The shared static/type vocabulary of the minimal design. `owner` is the
// Counter symbol spelling: the qualified source-unit form or the plain
// routing form. All texts reuse the working IR of the transaction suites
// (the TransactionEffectsTest / lit transaction-surface fixture), adapted
// to one scalar Byte state, one Table state and one Method whose formal
// payload width is the symbolic parameter W under a fixed-bits constraint.
std::string vocabulary(StringRef owner) {
  std::string origin =
      "origin = {site = {definition = @" + owner.str() +
      ", ast_path = []}, expansion = []}, location = {path = "
      "\"transactions.py\", line = 1 : i64, column = 1 : i64, end_line = 1 : "
      "i64, end_column = 2 : i64}";
  return "#w1 = #ac.static_expr<{kind = \"literal\", value = {kind = "
         "\"integer\", value = #ac.math_int<1>}, " +
         origin +
         "}>\n"
         "#w3 = #ac.static_expr<{kind = \"literal\", value = {kind = "
         "\"integer\", value = #ac.math_int<3>}, " +
         origin +
         "}>\n"
         "#w4 = #ac.static_expr<{kind = \"literal\", value = {kind = "
         "\"integer\", value = #ac.math_int<4>}, " +
         origin +
         "}>\n"
         "#w8 = #ac.static_expr<{kind = \"literal\", value = {kind = "
         "\"integer\", value = #ac.math_int<8>}, " +
         origin +
         "}>\n"
         "#w0 = #ac.static_expr<{kind = \"literal\", value = {kind = "
         "\"integer\", value = #ac.math_int<0>}, " +
         origin +
         "}>\n"
         "#wW = #ac.static_expr<{kind = \"reference\", ref = {kind = "
         "\"parameter\", owner = @" +
         owner.str() + ", name = \"W\"}, " + origin +
         "}>\n"
         "#wN = #ac.static_expr<{kind = \"reference\", ref = {kind = "
         "\"parameter\", owner = @" +
         owner.str() + ", name = \"N\"}, " + origin +
         "}>\n"
         "#wM = #ac.static_expr<{kind = \"reference\", ref = {kind = "
         "\"parameter\", owner = @" +
         owner.str() + ", name = \"M\"}, " + origin +
         "}>\n"
         "!b1 = !ac.bits<#w1>\n"
         "!b8 = !ac.bits<#w8>\n"
         "!bw = !ac.bits<#wW>\n"
         "!bM = !ac.bits<#wM>\n"
         "!tableN = !ac.table<[#wN], !b8>\n"
         "#counter = {site = {definition = @" + owner.str() +
         ", ast_path = []}, expansion = []}\n"
         "#entries = {site = {definition = @" + owner.str() +
         ", ast_path = [{kind = \"index\", value = 2 : i64}]}, expansion = "
         "[]}\n"
         "#left_instance = {site = {definition = @Root, ast_path = [{kind = "
         "\"index\", value = 1 : i64}]}, expansion = []}\n"
         "#right_instance = {site = {definition = @Root, ast_path = [{kind = "
         "\"index\", value = 2 : i64}]}, expansion = []}\n"
         "#root = {site = {definition = @Root, ast_path = []}, expansion = "
         "[]}\n"
         "#span = {path = \"transactions.py\", line = 1 : i64, column = 1 : "
         "i64, end_line = 1 : i64, end_column = 2 : i64}\n"
         "#byte_constraint = {kind = \"hardware\", type = !b8, source_kind = "
         "\"fixed_bits\"}\n"
         "#value_constraint = {kind = \"hardware\", type = !bw, source_kind = "
         "\"fixed_bits\"}\n";
}

// The Counter template: two states (one scalar, one Table) and one Method.
// `guardRead` is the guard region's state read result type; the mutated !b1
// form makes the definite payload disagreement the source stage owns. The
// parameters are the value width W, the Table extent N and the index width
// M; the Method's body truncates the value formal to !bM, reads the Table
// whole-owner and selects the lane with it, and writes the lane back
// through the indexed state write. The Method signature and its formal
// constraint stay the W-shaped contract of the scalar cases.
std::string counter(StringRef owner, StringRef guardRead = "!b8",
                    StringRef guardYieldType = "!b1",
                    StringRef guardYieldValue = "%path") {
  return std::string("\"ac.module\"() ({\n"
                     "^bb0(%data: !b8, %clk: !b1, %rst: !b1):\n"
                     "    %initial = \"ac.bits.constant\"() {value = #w1} : "
                     "() -> !b8\n"
                     "    %yes = \"ac.bits.constant\"() {value = #w1} : () -> "
                     "!b1\n"
                     "    %q = \"ac.state\"(%clk, %rst, %initial) "
                     "{instance_name = \"count\", occurrence = #counter} : "
                     "(!b1, !b1, !b8) -> !b8\n"
                     "    %fill = \"ac.bits.constant\"() {value = #w1} : () "
                     "-> !b8\n"
                     "    %blank = \"ac.table.splat\"(%fill) {shape = [#wN]} "
                     ": (!b8) -> !tableN\n"
                     "    %e = \"ac.state\"(%clk, %rst, %blank) "
                     "{instance_name = \"entries\", occurrence = #entries} : "
                     "(!b1, !b1, !tableN) -> !tableN\n"
                     "    \"ac.method\"(%yes) ({\n"
                     "    ^bb0(%value: !bw, %path: !b1):\n"
                     "      %current = \"ac.state.read\"(%path) {state = "
                     "\"count\", selectors = [], occurrence = #counter} : "
                     "(!b1) -> ") +
         guardRead.str() +
         "\n"
         "      \"ac.yield\"(" + guardYieldValue.str() + ") : (" +
         guardYieldType.str() + ") -> ()\n"
         "    }, {\n"
         "    ^bb0(%value: !bw, %path: !b1):\n"
         "      %old = \"ac.state.read\"(%path) {state = \"count\", "
         "selectors = [], occurrence = #counter} : (!b1) -> !b8\n"
         "      \"ac.state.write\"(%path, %old) {state = \"count\", "
         "selectors = [], occurrence = #counter} : (!b1, !b8) -> ()\n"
         "      %index = \"ac.bits.resize\"(%value) {mode = \"trunc\"} : "
         "(!bw) -> !bM\n"
         "      %table = \"ac.state.read\"(%path) {state = \"entries\", "
         "selectors = [], occurrence = #counter} : (!b1) -> !tableN\n"
         "      %cur, %ok = \"ac.table.get\"(%table, %index) : (!tableN, "
         "!bM) -> (!b8, !b1)\n"
         "      %one = \"ac.bits.constant\"() {value = #w1} : () -> !b8\n"
         "      %next = \"ac.bits.binary\"(%cur, %one) {opcode = \"add\"} : "
         "(!b8, !b8) -> !b8\n"
         "      \"ac.state.write\"(%path, %next, %index) {state = \"entries\", "
         "selectors = [{index = 0 : i64}], occurrence = #counter} : "
         "(!b1, !b8, !bM) -> ()\n"
         "      \"ac.yield\"(%old) : (!b8) -> ()\n"
         "    }) {sym_name = \"add\", function_type = (!bw) -> !b8, "
         "argument_names = [\"value\"], ac.parameters = [{name = \"value\", "
         "binding = \"positional_or_keyword\", constraint = "
         "#value_constraint, default = {present = false}, origin = "
         "#counter, location = #span}], ac.result_constraints = "
         "[#byte_constraint], ac.return_form = \"single\", "
         "ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()\n"
         "    \"ac.yield\"(%q) : (!b8) -> ()\n"
         "  }) {sym_name = \"" +
         owner.str() +
         "\", source_owner = {package = \"\", path = \"transactions.py\"}, "
         "parameters = [{name = \"W\", type = !ac.math_int}, "
         "{name = \"N\", type = !ac.math_int}, {name = \"M\", type = "
         "!ac.math_int}], type_parameters = [], function_type = (!b8, !b1, "
         "!b1) -> !b8, input_names = [\"data\", \"pyc_clk\", \"pyc_rst\"], "
         "output_names = [\"out\"], ac.domain_inputs = {clock = 1 : i64, "
         "reset = 2 : i64}} : () -> ()\n";
}

// The Root consumer with two Counter instances and one transaction calling
// the Method through both receivers. The instance parameters bind the
// callee's declared W, N and M positionally, so every binding list names all
// three; the Table cases keep W = #w8 and the extent N = #w8 fixed and
// change only the index-relevant M binding (#w3 legal, #w4 out of range),
// while the scalar cases bind a legal extent and index around the W binding
// under test.
std::string root(StringRef wBinding, StringRef nBinding, StringRef mBinding) {
  auto instance = [&](const char *name, const char *occurrence) {
    return std::string("    %") + name + " = \"ac.instance\"(%data, %clk, " +
           "%rst) {instance_name = \"" + name + "\", callee = @Counter, " +
           "parameters = [" + wBinding.str() + ", " + nBinding.str() + ", " +
           mBinding.str() + "], type_arguments = [], " +
           "occurrence = " + occurrence + "} : (!b8, !b1, !b1) -> !b8\n";
  };
  return std::string("\"ac.module\"() ({\n"
                     "^bb0(%data: !b8, %clk: !b1, %rst: !b1):\n"
                     "    %yes = \"ac.bits.constant\"() {value = #w1} : () -> "
                     "!b1\n") +
         instance("left", "#left_instance") +
         instance("right", "#right_instance") +
         "    %answer = \"ac.transaction\"(%clk, %rst, %yes, %data) ({\n"
         "    ^bb0(%path: !b1, %value: !b8):\n"
         "      \"ac.yield\"(%path) : (!b1) -> ()\n"
         "    }, {\n"
         "    ^bb0(%path: !b1, %value: !b8):\n"
         "      %a = \"ac.method.call\"(%path, %value) {callee = "
         "@Counter::@add, instance_path = [\"left\"], occurrence = #root} : "
         "(!b1, !b8) -> !b8\n"
         "      %b = \"ac.method.call\"(%path, %value) {callee = "
         "@Counter::@add, instance_path = [\"right\"], occurrence = #root} : "
         "(!b1, !b8) -> !b8\n"
         "      \"ac.yield\"(%b) : (!b8) -> ()\n"
         "    }) {name = \"step\", declaration = #root, occurrence = #root, "
         "ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> !b8\n"
         "    \"ac.yield\"(%answer) : (!b8) -> ()\n"
         "  }) {sym_name = \"Root\", source_owner = {package = \"\", path = "
         "\"transactions.py\"}, parameters = [], type_parameters = [], "
         "function_type = (!b8, !b1, !b1) -> !b8, input_names = [\"data\", "
         "\"pyc_clk\", \"pyc_rst\"], output_names = [\"answer\"], "
         "ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()\n"
         "  \"ac.system\"() {entry = {callee = @Root, parameters = [], "
         "type_arguments = []}, domain = \"default\"} : () -> ()\n";
}

class TransactionSurfaceTiersTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    context.loadDialect<acir::gfir::GFIRDialect>();
  }
  DictionaryAttr owner(StringRef path = "transactions.py") {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("")),
                                b.getNamedAttr("path", b.getStringAttr(path))});
  }
  DictionaryAttr origin(StringRef symbol) {
    Builder b(&context);
    return b.getDictionaryAttr(
        {b.getNamedAttr(
             "site", b.getDictionaryAttr(
                         {b.getNamedAttr("definition", FlatSymbolRefAttr::get(
                                                           &context, symbol)),
                          b.getNamedAttr("ast_path", b.getArrayAttr({}))})),
         b.getNamedAttr("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr site() {
    Builder b(&context);
    NamedAttrList span;
    span.append("path", b.getStringAttr("transactions.py"));
    for (StringRef key : {"line", "column", "end_line", "end_column"})
      span.append(key, b.getI64IntegerAttr(key == "end_column" ? 2 : 1));
    return b.getDictionaryAttr(
        {b.getNamedAttr("ast_path", b.getArrayAttr({})),
         b.getNamedAttr("location", span.getDictionary(&context))});
  }
  void capture() {
    diagnostics.clear();
    handler = std::make_unique<ScopedDiagnosticHandler>(
        &context, [&](Diagnostic &error) {
          llvm::raw_string_ostream stream(diagnostics);
          error.print(stream);
          return success();
        });
  }

  // The source-unit form: forge the envelope exactly as published source
  // bodies carry it (the SourceInterfacePassTest / SourceSymbolicBoundary
  // pattern).
  OwningOpRef<mlir::ModuleOp> parseEnvelope(const std::string &content) {
    auto unit = parseSourceString<mlir::ModuleOp>(
        vocabulary("transactions.Counter") + "module {\n" + content + "\n}",
        ParserConfig(&context, /*verifyAfterParse=*/false));
    if (!unit)
      return {};
    Builder b(&context);
    (*unit)->setAttr("ac.stage", b.getStringAttr("source"));
    (*unit)->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
    (*unit)->setAttr("ac.source_owner", owner());
    SmallVector<Attribute> interfaces{owner()};
    (*unit)->setAttr("ac.interfaces", b.getArrayAttr(interfaces));
    SmallVector<Attribute> exports;
    for (Operation &op : unit->getBody()->getOperations()) {
      auto symbol = SymbolTable::getSymbolName(&op);
      if (!symbol)
        continue;
      op.setAttr("ac.origin", origin(symbol.getValue()));
      op.setAttr("ac.declaration_role", b.getStringAttr("definition"));
      std::string name = symbol.getValue().rsplit('.').second.str();
      exports.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr(name)),
           b.getNamedAttr("target",
                          FlatSymbolRefAttr::get(&context, symbol.getValue())),
           b.getNamedAttr("site", site())}));
    }
    (*unit)->setAttr("ac.exports", b.getArrayAttr(exports));
    (*unit)->setAttr("ac.import_bindings", b.getArrayAttr({}));
    return unit;
  }
  OwningOpRef<mlir::ModuleOp> parsePlain(const std::string &content) {
    // The routing preparation consumes the merged source-units record, as
    // the lit transaction fixtures carry it.
    return parseSourceString<mlir::ModuleOp>(
        vocabulary("Counter") +
            "module attributes {ac.source_units = [{package = \"\", path = "
            "\"transactions.py\"}]} {\n" +
            content + "\n}",
        ParserConfig(&context, /*verifyAfterParse=*/false));
  }
  LogicalResult route(mlir::ModuleOp file) {
    capture();
    PassManager manager(&context);
    manager.addPass(acir::createRouteTransactionsPass());
    return manager.run(file);
  }
  std::unique_ptr<ScopedDiagnosticHandler> handler;
};

// Case 1: the symbolic template (two states and a Method, no instances)
// publishes standalone: native verification and the source-stage entry both
// accept it with the width obligation of the unbound value parameter
// deferred.
TEST_F(TransactionSurfaceTiersTest, SymbolicTemplatePublishesAtTheSourceStage) {
  capture();
  auto unit = parseEnvelope(counter("transactions.Counter"));
  ASSERT_TRUE(unit) << diagnostics;
  EXPECT_TRUE(succeeded(mlir::verify(*unit))) << diagnostics;
  auto error = [&] { return unit->emitError(); };
  EXPECT_TRUE(succeeded(ac::verifySourceBodyStructure(*unit, error)))
      << diagnostics;
}

// Case 2: a definite fact — the state read result disagreeing with the
// selected state payload — is rejected at the source stage, independent of
// any binding.
TEST_F(TransactionSurfaceTiersTest,
       DefinitePayloadErrorIsRejectedAtTheSourceStage) {
  capture();
  auto unit =
      parseEnvelope(counter("transactions.Counter", /*guardRead=*/"!b1"));
  ASSERT_TRUE(unit) << diagnostics;
  EXPECT_TRUE(succeeded(mlir::verify(*unit))) << diagnostics;
  auto error = [&] { return unit->emitError(); };
  EXPECT_TRUE(failed(ac::verifySourceBodyStructure(*unit, error)));
  EXPECT_NE(diagnostics.find("state read result must match selected "
                             "payload type"),
            std::string::npos)
      << diagnostics;
}

// Case 3: the full design's instances supply the legal W = 8 actual (with a
// legal extent and index binding), the symbolic Method constraint closes at
// eight bits and routing completes.
TEST_F(TransactionSurfaceTiersTest, LegalActualBindingCompletesRouting) {
  auto file = parsePlain(counter("Counter") + root("#w8", "#w8", "#w3"));
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(route(*file))) << diagnostics;
}

// Case 4: the illegal W = 0 actual leaves the Method parameter constraint's
// symbolic width without a legal finite resolution, and the routing
// preparation rejects the design at the bound Method payload with the
// Method-family diagnostic ('ac.method' op ...).
TEST_F(TransactionSurfaceTiersTest,
       IllegalActualBindingIsRejectedAtTheRoutingPreparation) {
  auto file = parsePlain(counter("Counter") + root("#w0", "#w8", "#w3"));
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(failed(route(*file))) << diagnostics;
  EXPECT_NE(diagnostics.find("hardware static value must be positive and "
                             "fit u64"),
            std::string::npos)
      << diagnostics;
}

// Case 5: the carrier grammar text is canonical — a non-Bits control
// carrier cannot parse (the ODS constrains control operands to Bits), so
// the staged row's re-guard is exercised through a directly mutated
// transaction: the clock operand swapped to the struct-typed packet value.
// The batch-1 tolerance only defers width-dependent checks on legal Bits
// carriers; the canonical width-one rejection stays stateControlPredicate's.
TEST_F(TransactionSurfaceTiersTest, NonBitsCarrierKeepsTheCanonicalText) {
  capture();
  auto unit = parsePlain(root("#w8", "#w8", "#w3"));
  ASSERT_TRUE(unit);
  ac::SourceTransactionOp transaction;
  unit->walk([&](mlir::Operation *op) {
    if (auto candidate = llvm::dyn_cast<ac::SourceTransactionOp>(op))
      transaction = candidate;
  });
  ASSERT_TRUE(transaction);
  // The design's carriers are all Bits; add a struct block argument to the
  // root and swap it in as the transaction clock (ODS would reject it at
  // verification, which is exactly why only the direct staged call reaches
  // the canonical carrier text).
  auto *rootModule = transaction->getParentOp();
  mlir::Value packet = rootModule->getRegion(0).front().addArgument(
      ac::StructType::get(&context,
                          mlir::StringAttr::get(&context, "Packet")),
      transaction.getLoc());
  transaction->setOperand(0, packet);
  diagnostics.clear();
  ac::HardwareAnalysis analysis(*unit);
  EXPECT_TRUE(failed(analysis.verifyTransactionSurfaceOperation(
      transaction, {}, /*requireResolved=*/false)));
  EXPECT_NE(diagnostics.find("transaction clock requires width-one Bits"),
            std::string::npos)
      << diagnostics;
}

// Case 6: a Table-indexed state access with a legal unbound parameter — the
// source stage checks the carrier, ordinal and Bits index grammar and
// DEFERS the width evaluation instead of rejecting; an illegal actual
// binding is rejected at the routing preparation.

// The extended Counter design publishes standalone as a source unit: the
// Table carrier, selector shape and Bits index grammar admit the design
// while the unbound extent N and index width M defer their width
// evaluations.
TEST_F(TransactionSurfaceTiersTest,
       TableIndexedStateDefersTheWidthAtTheSourceStage) {
  capture();
  auto unit = parseEnvelope(counter("transactions.Counter"));
  ASSERT_TRUE(unit) << diagnostics;
  EXPECT_TRUE(succeeded(mlir::verify(*unit))) << diagnostics;
  auto error = [&] { return unit->emitError(); };
  EXPECT_TRUE(succeeded(ac::verifySourceBodyStructure(*unit, error)))
      << diagnostics;
  // Deferral is silent: a legal unbound index width must not emit the
  // evaluator's unresolved-expression diagnostic on its way to success.
  EXPECT_TRUE(diagnostics.empty())
      << "deferred index widths must not emit diagnostics: " << diagnostics;
}

// The instances bind W = 8 (the scalar payload contract), the extent
// N = 8 and the index width M = 3: the truncated value formal covers
// exactly the eight lanes, the instance signature matches the callee, and
// the routing preparation completes.
TEST_F(TransactionSurfaceTiersTest,
       TableIndexedStateRoutesWithTheMatchingActualBinding) {
  capture();
  auto unit = parsePlain(counter("Counter") + root("#w8", "#w8", "#w3"));
  ASSERT_TRUE(unit) << diagnostics;
  EXPECT_TRUE(succeeded(route(*unit))) << diagnostics;
}

// Only the index-relevant binding changes (M = #w4): four bits can address
// sixteen lanes, so the checked index range exceeds the bound extent of
// eight, and the routing preparation rejects with the index-range check's
// own text — proving the rejection is the Table index proof and not a
// payload or signature disagreement.
TEST_F(TransactionSurfaceTiersTest,
       TableIndexedStateRejectsAnOutOfRangeActualBinding) {
  capture();
  auto unit = parsePlain(counter("Counter") + root("#w8", "#w8", "#w4"));
  ASSERT_TRUE(unit) << diagnostics;
  EXPECT_TRUE(failed(route(*unit))) << diagnostics;
  EXPECT_NE(diagnostics.find("checked Table index complete unsigned range "
                             "is not proven within bound extent"),
            std::string::npos)
      << diagnostics;
}

} // namespace
