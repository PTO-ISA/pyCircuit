#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
using namespace mlir;

std::string literal(unsigned width, unsigned line = 1) {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", value = #ac.math_int<" +
         std::to_string(width) + ">}, origin = {site = {definition = @unit.Fields, ast_path = []}, expansion = []}, location = {path = \"unit.py\", line = " +
         std::to_string(line) + " : i64, column = 1 : i64, end_line = " + std::to_string(line) + " : i64, end_column = 2 : i64}}>";
}
std::string preamble() {
  std::string text;
  for (unsigned width : {0u, 1u, 2u, 3u, 5u, 8u, 13u, 65u}) {
    text += "#w" + std::to_string(width) + " = " + literal(width) + "\n";
    if (width)
      text += "!b" + std::to_string(width) + " = !ac.bits<#w" + std::to_string(width) + ">\n";
  }
  return text + "#alternate = " + literal(8, 17) + "\n!other = !ac.bits<#alternate>\n";
}
std::string parcel() {
  return R"mlir(
  ac.struct "unit.Parcel" fields [{name = "omega", type = !b3}, {name = "alpha", type = !b8}, {name = "middle", type = !b1}] {ac.source_owner = {package = "", path = "unit.py"}}
  )mlir";
}
std::string module(StringRef name, StringRef body, StringRef signature,
                   StringRef inputs, StringRef outputs,
                   StringRef parameters = "[]",
                   StringRef typeParameters = "[]") {
  return "\"ac.module\"() ({" + body.str() + "}) {sym_name = \"unit." +
         name.str() +
         "\", source_owner = {package = \"\", path = \"unit.py\"}, parameters "
         "= " +
         parameters.str() + ", type_parameters = " + typeParameters.str() +
         ", function_type = " + signature.str() +
         ", input_names = " + inputs.str() +
         ", output_names = " + outputs.str() + "} : () -> ()\n";
}
class RecordProjectionSimplificationTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  DictionaryAttr owner(StringRef path = "unit.py") {
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
  DictionaryAttr site(StringRef path = "unit.py") {
    Builder b(&context);
    NamedAttrList span;
    span.append("path", b.getStringAttr(path));
    for (StringRef key : {"line", "column", "end_line", "end_column"})
      span.append(key, b.getI64IntegerAttr(key == "end_column" ? 2 : 1));
    return b.getDictionaryAttr(
        {b.getNamedAttr("ast_path", b.getArrayAttr({})),
         b.getNamedAttr("location", span.getDictionary(&context))});
  }
  OwningOpRef<mlir::ModuleOp> parse(StringRef content) {
    auto unit = parseSourceString<mlir::ModuleOp>(
        preamble() + "module {\n" + content.str() + "\n}", &context);
    if (!unit)
      return {};
    Builder b(&context);
    (*unit)->setAttr("ac.stage", b.getStringAttr("source"));
    (*unit)->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
    (*unit)->setAttr("ac.source_owner", owner());
    SmallVector<Attribute> owners{owner()};
    std::vector<std::pair<std::string, Attribute>> exports;
    for (Operation &op : unit->getBody()->getOperations()) {
      auto symbol = SymbolTable::getSymbolName(&op);
      if (!symbol)
        continue;
      const bool local = isa<ac::ModuleOp>(op) ||
                         (isa<ac::StructOp, ac::EnumOp>(op) &&
                          op.getAttrOfType<DictionaryAttr>("ac.source_owner") ==
                              owner());
      op.setAttr("ac.origin", origin(symbol.getValue()));
      op.setAttr("ac.declaration_role",
                 b.getStringAttr(local ? "definition" : "import_snapshot"));
      if (local) {
        std::string name = symbol.getValue().rsplit('.').second.str();
        exports.emplace_back(
            name,
            b.getDictionaryAttr(
                {b.getNamedAttr("name", b.getStringAttr(name)),
                 b.getNamedAttr("target", FlatSymbolRefAttr::get(
                                              &context, symbol.getValue())),
                 b.getNamedAttr("site", site())}));
      } else if (!op.hasAttr("primitive_kind")) {
        auto importedOwner = op.getAttrOfType<DictionaryAttr>("source_owner");
        if (!importedOwner)
          importedOwner = op.getAttrOfType<DictionaryAttr>("ac.source_owner");
        if (importedOwner &&
            !llvm::is_contained(owners, Attribute(importedOwner)))
          owners.push_back(importedOwner);
      }
    }
    std::sort(exports.begin(), exports.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    SmallVector<Attribute> rows;
    for (const auto &entry : exports)
      rows.push_back(entry.second);
    (*unit)->setAttr("ac.interfaces", b.getArrayAttr(owners));
    (*unit)->setAttr("ac.exports", b.getArrayAttr(rows));
    (*unit)->setAttr("ac.import_bindings", b.getArrayAttr({}));
    return unit;
  }
  std::string print(mlir::ModuleOp unit) {
    std::string result;
    llvm::raw_string_ostream out(result);
    unit.print(out, OpPrintingFlags().enableDebugInfo());
    return result;
  }
  LogicalResult simplify(mlir::ModuleOp unit) {
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return success();
    });
    PassManager manager(&context);
    manager.addPass(acir::createSimplifyRecordWiresPass());
    return manager.run(unit);
  }
  ac::ModuleOp definition(mlir::ModuleOp unit, StringRef name) {
    return cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(unit, name));
  }
  std::map<std::string, ArrayAttr> dependencies(mlir::ModuleOp unit) {
    ac::HardwareAnalysis analysis(unit);
    std::map<std::string, ArrayAttr> result;
    for (auto op : analysis.getDefinitions()) {
      ac::HardwareBindings binding;
      binding.owner = op;
      auto facts = analysis.analyzeModule(op, binding);
      EXPECT_TRUE(succeeded(facts));
      if (succeeded(facts))
        result.emplace(op.getSymName().str(),
                       ac::serializeOutputDependencies(&context, facts->dependencies));
    }
    return result;
  }
  void noScaffolding(mlir::ModuleOp unit) {
    unit.walk([&](Operation *op) {
      if (auto get = dyn_cast<ac::StructGetOp>(op)) {
        EXPECT_FALSE(get.getValue().getDefiningOp<ac::StructCreateOp>());
        EXPECT_FALSE(get.getResult().use_empty());
      }
      if (auto create = dyn_cast<ac::StructCreateOp>(op))
        EXPECT_FALSE(create.getResult().use_empty());
    });
  }
  void preservedAndIdempotent(mlir::ModuleOp unit,
                            const std::map<std::string, ArrayAttr> &before) {
    EXPECT_EQ(dependencies(unit), before);
    noScaffolding(unit);
    auto once = print(unit);
    ASSERT_TRUE(succeeded(simplify(unit))) << diagnostics;
    EXPECT_EQ(print(unit), once);
  }
};

std::string fieldsModule(StringRef name = "Fields") {
  return module(name, R"mlir(^bb0(%a: !b8, %b: !b3, %c: !b1):
    %record = ac.struct.create(%b, %a, %c) : (!b3, !b8, !b1) -> !ac.struct<"unit.Parcel">
    %a1 = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8
    %b1 = ac.struct.get %record["omega"] : (!ac.struct<"unit.Parcel">) -> !b3
    %a2 = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8
    "ac.yield"(%a1, %b1, %a2, %record) : (!b8, !b3, !b8, !ac.struct<"unit.Parcel">) -> ()
  )mlir", "(!b8, !b3, !b1) -> (!b8, !b3, !b8, !ac.struct<\"unit.Parcel\">)",
                "[\"a\", \"b\", \"c\"]", "[\"first\", \"second\", \"repeat\", \"snapshot\"]");
}

TEST_F(RecordProjectionSimplificationTest, DeclarationOrderFanoutAndSnapshot) {
  auto unit = parse(parcel() + fieldsModule());
  ASSERT_TRUE(unit);
  auto op = definition(*unit, "unit.Fields");
  auto attrs = op->getAttrDictionary();
  auto packageAttrs = (*unit)->getAttrDictionary();
  auto *record = &op.getBody().front().front();
  auto recordAttrs = record->getAttrDictionary();
  auto before = dependencies(*unit);
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  auto yield = cast<ac::YieldOp>(op.getBody().front().back());
  EXPECT_EQ(yield.getValues()[0], op.getBody().front().getArgument(0));
  EXPECT_EQ(yield.getValues()[1], op.getBody().front().getArgument(1));
  EXPECT_EQ(yield.getValues()[2], op.getBody().front().getArgument(0));
  EXPECT_EQ(yield.getValues()[3].getDefiningOp(), record);
  EXPECT_EQ(record->getAttrDictionary(), recordAttrs);
  EXPECT_EQ(op->getAttrDictionary(), attrs);
  EXPECT_EQ((*unit)->getAttrDictionary(), packageAttrs);
  preservedAndIdempotent(*unit, before);
}

TEST_F(RecordProjectionSimplificationTest, NestedNominalEnumAndForwardGraphOrder) {
  const char *declarations = R"mlir(
  "ac.enum"() {sym_name = "unit.State", width = #ac.math_int<5>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}], ac.source_owner = {package = "", path = "unit.py"}} : () -> ()
  ac.struct "unit.Inner" fields [{name = "state", type = !ac.enum<"unit.State">}, {name = "payload", type = !b13}] {ac.source_owner = {package = "", path = "unit.py"}}
  ac.struct "unit.Outer" fields [{name = "tail", type = !b1}, {name = "inner", type = !ac.struct<"unit.Inner">}] {ac.source_owner = {package = "", path = "unit.py"}}
  )mlir";
  auto unit = parse(std::string(declarations) + module("Nested", R"mlir(^bb0(%state: !ac.enum<"unit.State">, %p: !b13, %tail: !b1):
    %nested = ac.struct.get %outer["inner"] : (!ac.struct<"unit.Outer">) -> !ac.struct<"unit.Inner">
    %payload = ac.struct.get %nested["payload"] : (!ac.struct<"unit.Inner">) -> !b13
    %enum = ac.struct.get %nested["state"] : (!ac.struct<"unit.Inner">) -> !ac.enum<"unit.State">
    %outer = ac.struct.create(%tail, %inner) : (!b1, !ac.struct<"unit.Inner">) -> !ac.struct<"unit.Outer">
    %inner = ac.struct.create(%state, %p) : (!ac.enum<"unit.State">, !b13) -> !ac.struct<"unit.Inner">
    "ac.yield"(%payload, %enum, %outer) : (!b13, !ac.enum<"unit.State">, !ac.struct<"unit.Outer">) -> ()
  )mlir", "(!ac.enum<\"unit.State\">, !b13, !b1) -> (!b13, !ac.enum<\"unit.State\">, !ac.struct<\"unit.Outer\">)", "[\"state\", \"p\", \"tail\"]", "[\"payload\", \"enum\", \"snapshot\"]"));
  ASSERT_TRUE(unit);
  auto before = dependencies(*unit);
  auto op = definition(*unit, "unit.Nested");
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  auto yield = cast<ac::YieldOp>(op.getBody().front().back());
  EXPECT_EQ(yield.getValues()[0], op.getBody().front().getArgument(1));
  EXPECT_EQ(yield.getValues()[1], op.getBody().front().getArgument(0));
  EXPECT_EQ(yield.getValues()[2].getDefiningOp()->getName().getStringRef(), "ac.struct.create");
  preservedAndIdempotent(*unit, before);
}

TEST_F(RecordProjectionSimplificationTest, EquivalentProvenancePreservesProducerAndExactResult) {
  auto unit = parse(parcel() + module("Provenance", R"mlir(^bb0(%a: !other, %b: !b3, %c: !b1):
    %scalar = "ac.bits.unary"(%a) {opcode = "not", retained = "source-kind-boundary"} : (!other) -> !other
    %record = ac.struct.create(%b, %scalar, %c) : (!b3, !other, !b1) -> !ac.struct<"unit.Parcel">
    %get = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8 loc("unit.py":31:9)
    %again = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8 loc("unit.py":32:9)
    "ac.yield"(%get, %again) : (!b8, !b8) -> ()
  )mlir", "(!other, !b3, !b1) -> (!b8, !b8)", "[\"a\", \"b\", \"c\"]", "[\"get\", \"again\"]"));
  ASSERT_TRUE(unit);
  auto op = definition(*unit, "unit.Provenance");
  auto *producer = &op.getBody().front().front();
  Type originalType = producer->getResult(0).getType();
  auto attrs = producer->getAttrDictionary();
  auto yield = cast<ac::YieldOp>(op.getBody().front().back());
  Type exactType = yield.getValues()[0].getType();
  EXPECT_NE(exactType, originalType);
  ASSERT_TRUE(ac::areEquivalentHardwareTypes(exactType, originalType));
  auto before = dependencies(*unit);
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  auto extract = yield.getValues()[0].getDefiningOp<ac::BitsExtractOp>();
  ASSERT_TRUE(extract);
  EXPECT_EQ(extract.getInput(), producer->getResult(0));
  EXPECT_EQ(extract.getResult().getType(), exactType);
  EXPECT_EQ(yield.getValues()[0], yield.getValues()[1]);
  EXPECT_EQ(producer->getResult(0).getType(), originalType);
  EXPECT_EQ(producer->getAttrDictionary(), attrs);
  auto zeroTree = extract.getLow().getTree();
  auto fieldWidth = cast<ac::BitsType>(exactType).getWidth().getTree();
  EXPECT_EQ(zeroTree.get("origin"), fieldWidth.get("origin"));
  EXPECT_EQ(zeroTree.get("location"), fieldWidth.get("location"));
  ac::HardwareAnalysis analysis(*unit);
  auto low = analysis.evaluateStatic(extract.getLow(), {}, extract);
  ASSERT_TRUE(succeeded(low));
  EXPECT_EQ(cast<ac::MathIntAttr>(*low).getCanonicalValue(), "0");
  EXPECT_TRUE(extract.getLoc() == FileLineColLoc::get(&context, "unit.py", 31, 9) ||
              extract.getLoc() == FileLineColLoc::get(&context, "unit.py", 32, 9));
  preservedAndIdempotent(*unit, before);
}

TEST_F(RecordProjectionSimplificationTest, InvalidSourceEnvelopesRejectBeforeRewriting) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto unit = parse(parcel() + fieldsModule());
    ASSERT_TRUE(unit);
    Builder b(&context);
    if (mutation == 0) (*unit)->removeAttr("ac.source_owner");
    if (mutation == 1) (*unit)->setAttr("ac.stage", b.getStringAttr("final"));
    if (mutation == 2) (*unit)->setAttr("ac.interfaces", b.getArrayAttr({owner(), owner()}));
    if (mutation == 3) definition(*unit, "unit.Fields")->setAttr("source_owner", owner("foreign.py"));
    auto before = print(*unit);
    EXPECT_TRUE(failed(simplify(*unit)));
    EXPECT_EQ(print(*unit), before);
  }
}

TEST_F(RecordProjectionSimplificationTest, MalformedFieldArityAndWidthFailWithoutMutation) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    auto unit = parse(parcel() + fieldsModule());
    ASSERT_TRUE(unit);
    auto op = definition(*unit, "unit.Fields");
    auto create = cast<ac::StructCreateOp>(op.getBody().front().front());
    auto get = *op.getBody().front().getOps<ac::StructGetOp>().begin();
    if (mutation == 0) get->setAttr("field", StringAttr::get(&context, "absent"));
    if (mutation == 1) create->eraseOperand(2);
    if (mutation == 2) create->setOperand(1, op.getBody().front().getArgument(1));
    auto before = print(*unit);
    EXPECT_TRUE(failed(simplify(*unit)));
    EXPECT_EQ(print(*unit), before);
  }
}

TEST_F(RecordProjectionSimplificationTest, EquallyEvaluatedWidthsDoNotEstablishEquivalence) {
  auto unit = parse(parcel() + fieldsModule());
  ASSERT_TRUE(unit);
  Builder b(&context);
  auto original = cast<ac::BitsType>(definition(*unit, "unit.Fields").getBody().front().getArgument(0).getType());
  auto tree = original.getWidth().getTree();
  auto four = cast<ac::StaticExprAttr>(parseAttribute(literal(4), &context));
  auto expression = ac::StaticExprAttr::get(&context, b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("binary")),
      b.getNamedAttr("operator", b.getStringAttr("add")),
      b.getNamedAttr("lhs", four), b.getNamedAttr("rhs", four),
      b.getNamedAttr("origin", tree.get("origin")),
      b.getNamedAttr("location", tree.get("location"))}));
  auto evaluatedEight = ac::BitsType::get(&context, expression);
  EXPECT_FALSE(ac::areEquivalentHardwareTypes(original, evaluatedEight));
  ac::HardwareAnalysis analysis(*unit);
  auto width = analysis.getPackedWidth(evaluatedEight, {}, *unit);
  ASSERT_TRUE(succeeded(width));
  EXPECT_EQ(*width, 8u);
  auto op = definition(*unit, "unit.Fields");
  op.getBody().front().getArgument(0).setType(evaluatedEight);
  SmallVector<Type> inputs(op.getFunctionType().getInputs().begin(),
                           op.getFunctionType().getInputs().end());
  inputs[0] = evaluatedEight;
  op->setAttr("function_type", TypeAttr::get(FunctionType::get(&context, inputs, op.getFunctionType().getResults())));
  auto before = print(*unit);
  EXPECT_TRUE(failed(simplify(*unit)));
  EXPECT_EQ(print(*unit), before);
}

TEST_F(RecordProjectionSimplificationTest, EqualPackedWidthNeverSubstitutesNominalEnum) {
  auto unit = parse(R"mlir(
    "ac.enum"() {sym_name = "unit.Left", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}], ac.source_owner = {package = "", path = "unit.py"}} : () -> ()
    "ac.enum"() {sym_name = "unit.Right", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}], ac.source_owner = {package = "", path = "unit.py"}} : () -> ()
    ac.struct "unit.Nominal" fields [{name = "value", type = !ac.enum<"unit.Left">}] {ac.source_owner = {package = "", path = "unit.py"}}
  )mlir" + module("Nominals", R"mlir(^bb0(%left: !ac.enum<"unit.Left">, %right: !ac.enum<"unit.Right">):
    %r = ac.struct.create(%left) : (!ac.enum<"unit.Left">) -> !ac.struct<"unit.Nominal">
    %get = ac.struct.get %r["value"] : (!ac.struct<"unit.Nominal">) -> !ac.enum<"unit.Left">
    "ac.yield"(%get) : (!ac.enum<"unit.Left">) -> ()
  )mlir", "(!ac.enum<\"unit.Left\">, !ac.enum<\"unit.Right\">) -> !ac.enum<\"unit.Left\">", "[\"left\", \"right\"]", "[\"get\"]"));
  ASSERT_TRUE(unit);
  auto op = definition(*unit, "unit.Nominals");
  auto create = cast<ac::StructCreateOp>(op.getBody().front().front());
  EXPECT_FALSE(ac::areEquivalentHardwareTypes(op.getBody().front().getArgument(0).getType(), op.getBody().front().getArgument(1).getType()));
  create->setOperand(0, op.getBody().front().getArgument(1));
  auto before = print(*unit);
  EXPECT_TRUE(failed(simplify(*unit)));
  EXPECT_EQ(print(*unit), before);
}

TEST_F(RecordProjectionSimplificationTest, EveryDefinitionPreflightsBeforeAnyRewriteBothOrders) {
  const auto scalarCycle = module("ScalarCycle", R"mlir(^bb0(%a: !b8):
    %unused = "ac.bits.unary"(%unused) {opcode = "not"} : (!b8) -> !b8
    "ac.yield"(%a) : (!b8) -> ()
  )mlir", "(!b8) -> !b8", "[\"a\"]", "[\"out\"]");
  const auto recordCycle = module("RecordCycle", R"mlir(^bb0(%a: !b8, %b: !b3, %c: !b1):
    %unused = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8
    %record = ac.struct.create(%b, %unused, %c) : (!b3, !b8, !b1) -> !ac.struct<"unit.Parcel">
    "ac.yield"(%a) : (!b8) -> ()
  )mlir", "(!b8, !b3, !b1) -> !b8", "[\"a\", \"b\", \"c\"]", "[\"out\"]");
  for (const auto &bad : {scalarCycle, recordCycle}) {
    for (bool reverse : {false, true}) {
      SCOPED_TRACE(reverse);
      auto unit = parse(parcel() + (reverse ? bad + fieldsModule() : fieldsModule() + bad));
      ASSERT_TRUE(unit);
      ASSERT_TRUE(succeeded(mlir::verify(*unit)));
      auto *good = SymbolTable::lookupSymbolIn(*unit, "unit.Fields");
      auto before = print(*unit);
      EXPECT_TRUE(failed(simplify(*unit)));
      EXPECT_EQ(print(*unit), before);
      EXPECT_EQ(SymbolTable::lookupSymbolIn(*unit, "unit.Fields"), good);
      EXPECT_FALSE(diagnostics.empty());
    }
  }
}

TEST_F(RecordProjectionSimplificationTest, OriginalUnusedScalarsExtractsRulesAndEffectsSurvive) {
  auto unit = parse(parcel() + module("Retain", R"mlir(^bb0(%a: !b8, %b: !b3, %on: !b1):
    %unused = "ac.bits.unary"(%a) {opcode = "not"} : (!b8) -> !b8
    %boundary = "ac.bits.extract"(%a) {low = #w0} : (!b8) -> !b8
    %unused_rule = "ac.rule"(%a) ({
    ^bb0(%x: !b8):
      %not = "ac.bits.unary"(%x) {opcode = "not"} : (!b8) -> !b8
      "ac.yield"(%not) : (!b8) -> ()
    }) {name = "still_checked", occurrence = {site = {definition = @unit.Retain, ast_path = []}, expansion = []}} : (!b8) -> !b8
    %check:2 = "ac.rule"(%on) ({
    ^bb0(%predicate: !b1):
      "ac.yield"(%predicate, %predicate) : (!b1, !b1) -> ()
    }) {name = "actual_check", occurrence = {site = {definition = @unit.Retain, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, ac.required_checks = [{id = {registration = {site = {definition = @unit.Retain, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, check = {site = {definition = @unit.Retain, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, obligation = 0 : i64}, kind = "assert", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}]} : (!b1) -> (!b1, !b1)
    %r = ac.struct.create(%b, %a, %on) : (!b3, !b8, !b1) -> !ac.struct<"unit.Parcel">
    %get = ac.struct.get %r["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8
    "ac.observe"(%on, %get) {kind = "report", spec = {name = "retained"}} : (!b1, !b8) -> ()
    "ac.expect"(%check#0, %check#1) {kind = "assert", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, ac.check_id = {registration = {site = {definition = @unit.Retain, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, check = {site = {definition = @unit.Retain, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, obligation = 0 : i64}} : (!b1, !b1) -> ()
    "ac.yield"(%get) : (!b8) -> ()
  )mlir", "(!b8, !b3, !b1) -> !b8", "[\"a\", \"b\", \"on\"]", "[\"get\"]"));
  ASSERT_TRUE(unit);
  auto op = definition(*unit, "unit.Retain");
  auto unusedRule = *op.getBody().front().getOps<ac::RuleOp>().begin();
  EXPECT_TRUE(unusedRule.getResult(0).use_empty());
  auto expect = *op.getBody().front().getOps<ac::SourceExpectOp>().begin();
  EXPECT_NE(expect.getCondition(), expect.getPath());
  SmallVector<Operation *> retained;
  op.walk([&](Operation *item) {
    if (isa<ac::BitsUnaryOp, ac::BitsExtractOp, ac::RuleOp, ac::SourceObserveOp, ac::SourceExpectOp>(item))
      retained.push_back(item);
  });
  SmallVector<DictionaryAttr> attributes;
  for (auto *item : retained) attributes.push_back(item->getAttrDictionary());
  auto before = dependencies(*unit);
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  SmallVector<Operation *> after;
  op.walk([&](Operation *item) {
    if (isa<ac::BitsUnaryOp, ac::BitsExtractOp, ac::RuleOp, ac::SourceObserveOp, ac::SourceExpectOp>(item))
      after.push_back(item);
  });
  EXPECT_EQ(after, retained);
  EXPECT_TRUE(unusedRule.getResult(0).use_empty());
  for (auto [index, item] : llvm::enumerate(after)) EXPECT_EQ(item->getAttrDictionary(), attributes[index]);
  auto observe = *op.getBody().front().getOps<ac::SourceObserveOp>().begin();
  EXPECT_EQ(observe->getOperand(1), op.getBody().front().getArgument(0));
  auto yield = cast<ac::YieldOp>(op.getBody().front().back());
  EXPECT_EQ(yield.getValues()[0], op.getBody().front().getArgument(0));
  preservedAndIdempotent(*unit, before);
}

TEST_F(RecordProjectionSimplificationTest, OldQFeedbackIsATemporalCutAndInstanceIsRetained) {
  const char *storage = R"mlir(
  "ac.module.import"() {sym_name = "gfsim.dff.Dff", source_owner = {package = "gfsim", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@gfsim.dff.Dff, "T">, !ac.type_param<@gfsim.dff.Dff, "T">) -> !ac.type_param<@gfsim.dff.Dff, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  )mlir";
  auto unit = parse(std::string(storage) + parcel() + module("Feedback", R"mlir(^bb0(%clk: !b1, %rst: !b1, %init: !b8, %tag: !b3):
    %record = ac.struct.create(%tag, %q, %clk) : (!b3, !b8, !b1) -> !ac.struct<"unit.Parcel">
    %d = ac.struct.get %record["alpha"] : (!ac.struct<"unit.Parcel">) -> !b8
    %q = "ac.instance"(%clk, %rst, %d, %init) {instance_name = "register", callee = @gfsim.dff.Dff, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @unit.Feedback, ast_path = []}, expansion = []}} : (!b1, !b1, !b8, !b8) -> !b8
    "ac.yield"(%q) : (!b8) -> ()
  )mlir", "(!b1, !b1, !b8, !b3) -> !b8", "[\"clk\", \"rst\", \"init\", \"tag\"]", "[\"q\"]"));
  ASSERT_TRUE(unit);
  auto op = definition(*unit, "unit.Feedback");
  auto instance = *op.getBody().front().getOps<ac::InstanceOp>().begin();
  auto attrs = instance->getAttrDictionary();
  auto *import = SymbolTable::lookupSymbolIn(*unit, "gfsim.dff.Dff");
  auto importAttrs = import->getAttrDictionary();
  auto before = dependencies(*unit);
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  EXPECT_EQ(instance->getOperand(2), instance->getResult(0));
  EXPECT_EQ(instance->getAttrDictionary(), attrs);
  EXPECT_EQ(import->getAttrDictionary(), importAttrs);
  auto summary = before.at("unit.Feedback");
  ASSERT_FALSE(summary.empty());
  EXPECT_TRUE(cast<DictionaryAttr>(summary[0]).getAs<ArrayAttr>("inputs").empty());
  preservedAndIdempotent(*unit, before);
}

TEST_F(RecordProjectionSimplificationTest, ExtractionReuseStaysInsideIsolatedRules) {
  std::string body = "^bb0(%a: !other):\n";
  for (unsigned i = 0; i < 2; ++i) {
    const auto n = std::to_string(i);
    body += "%r" + n + ":2 = \"ac.rule\"(%a) ({\n"
            "^bb0(%x: !other):\n"
            "%record = ac.struct.create(%x) : (!other) -> !ac.struct<\"unit.One\">\n"
            "%first = ac.struct.get %record[\"value\"] : (!ac.struct<\"unit.One\">) -> !b8\n"
            "%second = ac.struct.get %record[\"value\"] : (!ac.struct<\"unit.One\">) -> !b8\n"
            "\"ac.yield\"(%first, %second) : (!b8, !b8) -> ()\n"
            "}) {name = \"rule" + n + "\", occurrence = {site = {definition = @unit.Isolated, ast_path = []}, expansion = []}} : (!other) -> (!b8, !b8)\n";
  }
  body += "\"ac.yield\"(%r0#0, %r0#1, %r1#0, %r1#1) : (!b8, !b8, !b8, !b8) -> ()\n";
  auto unit = parse(R"mlir(
    ac.struct "unit.One" fields [{name = "value", type = !b8}] {ac.source_owner = {package = "", path = "unit.py"}}
  )mlir" + module("Isolated", body, "(!other) -> (!b8, !b8, !b8, !b8)", "[\"a\"]", "[\"a0\", \"a1\", \"b0\", \"b1\"]"));
  ASSERT_TRUE(unit);
  auto before = dependencies(*unit);
  auto op = definition(*unit, "unit.Isolated");
  SmallVector<ac::RuleOp> rules;
  for (auto rule : op.getBody().front().getOps<ac::RuleOp>()) rules.push_back(rule);
  ASSERT_EQ(rules.size(), 2u);
  ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
  SmallVector<Value> values;
  for (auto rule : rules) {
    auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
    for (auto value : yield.getValues()) {
      auto extract = value.getDefiningOp<ac::BitsExtractOp>();
      ASSERT_TRUE(extract);
      EXPECT_EQ(extract->getParentOp(), rule.getOperation());
      EXPECT_EQ(extract.getInput(), rule.getBody().front().getArgument(0));
    }
    // Reuse is permitted only if dominance allows it. A later extraction
    // cannot satisfy an earlier SSA projection, regardless of visit order.
    values.push_back(yield.getValues()[0]);
  }
  EXPECT_NE(values[0], values[1]);
  EXPECT_TRUE(succeeded(mlir::verify(*unit)));
  preservedAndIdempotent(*unit, before);
}

// Nested provenance chains cause newly created extracts' inputs to change
// under RAUW. Dead branches can erase cache entries before other users visit
// the same producer. Both graph text orders must reach the same input identity.
TEST_F(RecordProjectionSimplificationTest, NewExtractionCacheSurvivesMutationAndDeadBranchRemoval) {
  for (bool reverse : {false, true}) {
    std::vector<std::string> steps;
    std::string previous = "%a", previousType = "!other";
    for (unsigned i = 0; i < 9; ++i) {
      auto n = std::to_string(i);
      auto outputType = i % 2 ? "!b8" : "!other";
      steps.push_back("%record" + n + " = ac.struct.create(" + previous + ") : (" + previousType + ") -> !ac.struct<\"unit.One\">\n"
                      "%get" + n + " = ac.struct.get %record" + n + "[\"value\"] : (!ac.struct<\"unit.One\">) -> " + outputType + "\n");
      previous = "%get" + n;
      previousType = outputType;
    }
    steps.push_back("%dead_record = ac.struct.create(%get3) : (!b8) -> !ac.struct<\"unit.One\">\n"
                    "%dead = ac.struct.get %dead_record[\"value\"] : (!ac.struct<\"unit.One\">) -> !other\n");
    // A live downstream projection discards a different field. Depending on
    // graph worklist order, its now-dead producer has already become an
    // identity extract; listener removal must not retain a dangling cache key.
    steps.push_back("%drop_record = ac.struct.create(%get3) : (!b8) -> !ac.struct<\"unit.One\">\n"
                    "%drop = ac.struct.get %drop_record[\"value\"] : (!ac.struct<\"unit.One\">) -> !other\n"
                    "%branch_record = ac.struct.create(%drop, %a) : (!other, !other) -> !ac.struct<\"unit.Branch\">\n"
                    "%kept = ac.struct.get %branch_record[\"kept\"] : (!ac.struct<\"unit.Branch\">) -> !other\n");
    std::string body = "^bb0(%a: !other):\n";
    if (reverse) std::reverse(steps.begin(), steps.end());
    for (const auto &step : steps) body += step;
    body += "\"ac.yield\"(%get8, %get3, %kept) : (!other, !b8, !other) -> ()\n";
    auto unit = parse(R"mlir(
      ac.struct "unit.One" fields [{name = "value", type = !b8}] {ac.source_owner = {package = "", path = "unit.py"}}
      ac.struct "unit.Branch" fields [{name = "drop", type = !b8}, {name = "kept", type = !b8}] {ac.source_owner = {package = "", path = "unit.py"}}
    )mlir" + module("Cache", body, "(!other) -> (!other, !b8, !other)", "[\"a\"]", "[\"end\", \"middle\", \"kept\"]"));
    ASSERT_TRUE(unit);
    auto before = dependencies(*unit);
    auto op = definition(*unit, "unit.Cache");
    ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
    auto yield = cast<ac::YieldOp>(op.getBody().front().back());
    for (auto value : yield.getValues()) {
      unsigned traversed = 0;
      while (auto extract = value.getDefiningOp<ac::BitsExtractOp>()) {
        EXPECT_TRUE(ac::areEquivalentHardwareTypes(extract.getInput().getType(), extract.getResult().getType()));
        EXPECT_FALSE(extract.getResult().use_empty());
        value = extract.getInput();
        ASSERT_LT(++traversed, 16u);
      }
      EXPECT_EQ(value, op.getBody().front().getArgument(0));
    }
    op.walk([&](ac::BitsExtractOp extract) { EXPECT_FALSE(extract.getResult().use_empty()); });
    preservedAndIdempotent(*unit, before);
  }
}

TEST_F(RecordProjectionSimplificationTest, VariedUpdateFamiliesRetainOnlyObservableSnapshots) {
  for (unsigned size : {2u, 7u, 19u}) {
    for (bool reverse : {false, true}) {
      SCOPED_TRACE(size);
      SCOPED_TRACE(reverse);
      std::vector<unsigned> order(size);
      std::iota(order.begin(), order.end(), 0);
      std::rotate(order.begin(), order.begin() + size / 2, order.end());
      if (reverse) std::reverse(order.begin(), order.end());
      const unsigned widths[] = {1, 3, 5, 8, 13, 65};
      auto type = [&](unsigned field) { return "!b" + std::to_string(widths[(field * 5 + size) % 6]); };
      auto name = [&](unsigned field) { return "field_" + std::to_string((field * 17 + 11) % 101) + "_" + std::to_string(field); };
      std::string declaration = "ac.struct \"unit.Varied\" fields [", inputTypes, inputNames, arguments;
      for (unsigned ordinal = 0; ordinal < size; ++ordinal) {
        unsigned field = order[ordinal];
        if (ordinal) declaration += ", ";
        declaration += "{name = \"" + name(field) + "\", type = " + type(field) + "}";
      }
      declaration += "] {ac.source_owner = {package = \"\", path = \"unit.py\"}}\n";
      for (unsigned i = 0; i < size * 2; ++i) {
        if (i) { inputTypes += ", "; inputNames += ", "; arguments += ", "; }
        inputTypes += type(i % size);
        inputNames += "\"arg" + std::to_string(i) + "\"";
        arguments += "%a" + std::to_string(i) + ": " + type(i % size);
      }
      std::string body = "^bb0(" + arguments + "):\n";
      std::vector<unsigned> sources(size);
      std::iota(sources.begin(), sources.end(), 0);
      std::vector<std::vector<unsigned>> saved;
      std::vector<unsigned> savedVersions;
      auto create = [&](unsigned version, const std::vector<std::string> &operands) {
        std::string operandsText, typesText;
        for (unsigned ordinal = 0; ordinal < size; ++ordinal) {
          if (ordinal) { operandsText += ", "; typesText += ", "; }
          operandsText += operands[order[ordinal]];
          typesText += type(order[ordinal]);
        }
        body += "%record" + std::to_string(version) + " = ac.struct.create(" + operandsText + ") : (" + typesText + ") -> !ac.struct<\"unit.Varied\">\n";
      };
      std::vector<std::string> operands;
      for (unsigned i = 0; i < size; ++i) operands.push_back("%a" + std::to_string(i));
      create(0, operands);
      saved.push_back(sources); savedVersions.push_back(0);
      for (unsigned version = 1; version <= size * 5; ++version) {
        unsigned changed = order[(version - 1) % size];
        for (unsigned field = 0; field < size; ++field) {
          auto value = "%old" + std::to_string(version) + "_" + std::to_string(field);
          body += value + " = ac.struct.get %record" + std::to_string(version - 1) + "[\"" + name(field) + "\"] : (!ac.struct<\"unit.Varied\">) -> " + type(field) + "\n";
          operands[field] = value;
        }
        sources[changed] = size + changed;
        operands[changed] = "%a" + std::to_string(size + changed);
        create(version, operands);
        if (version == size / 2 || version == size * 5) {
          saved.push_back(sources); savedVersions.push_back(version);
        }
      }
      std::string outputs, outputNames, outputTypes;
      for (auto [i, version] : llvm::enumerate(savedVersions)) {
        if (i) { outputs += ", "; outputNames += ", "; outputTypes += ", "; }
        outputs += "%record" + std::to_string(version);
        outputNames += "\"snapshot" + std::to_string(i) + "\"";
        outputTypes += "!ac.struct<\"unit.Varied\">";
      }
      body += "\"ac.yield\"(" + outputs + ") : (" + outputTypes + ") -> ()\n";
      auto unit = parse(declaration + module("Updates", body, "(" + inputTypes + ") -> (" + outputTypes + ")", "[" + inputNames + "]", "[" + outputNames + "]"));
      ASSERT_TRUE(unit);
      auto before = dependencies(*unit);
      auto op = definition(*unit, "unit.Updates");
      ASSERT_TRUE(succeeded(simplify(*unit))) << diagnostics;
      auto yield = cast<ac::YieldOp>(op.getBody().front().back());
      ASSERT_EQ(yield.getValues().size(), saved.size());
      for (auto [snapshot, value] : llvm::enumerate(yield.getValues())) {
        auto record = value.getDefiningOp<ac::StructCreateOp>();
        ASSERT_TRUE(record);
        for (unsigned ordinal = 0; ordinal < size; ++ordinal)
          EXPECT_EQ(record.getValues()[ordinal], op.getBody().front().getArgument(saved[snapshot][order[ordinal]]));
      }
      unsigned retained = 0;
      op.walk([&](ac::StructCreateOp) { ++retained; });
      // Observable versions bound retention, independent of total assignments.
      EXPECT_LE(retained, saved.size());
      preservedAndIdempotent(*unit, before);
    }
  }
}
} // namespace
