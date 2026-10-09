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
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
using namespace mlir;

const char *preamble = R"mlir(
#w0 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!t2 = !ac.table<[#w2], !b8>
!t3 = !ac.table<[#w3], !b8>
!pair2 = !ac.table<[#w2], !ac.struct<"types.Pair">>
!pair3 = !ac.table<[#w3], !ac.struct<"types.Pair">>
)mlir";
const char *records = R"mlir(
ac.struct "types.Pair" fields [{name = "left", type = !b8}, {name = "right", type = !b1}]
ac.struct "types.Other" fields [{name = "left", type = !b8}, {name = "right", type = !b1}]
)mlir";

struct Endpoint {
  unsigned port;
  std::vector<std::string> path;
};
struct Row {
  Endpoint output;
  std::vector<Endpoint> inputs;
};

class TableDependencySummaryTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  OwningOpRef<mlir::ModuleOp> parse(StringRef body, bool verify = true) {
    return parseSourceString<mlir::ModuleOp>(
        std::string(preamble) + "module {" + records + body.str() + "}",
        ParserConfig(&context, verify));
  }
  std::string declaration(StringRef signature,
                          StringRef typeParameters = "[]") {
    return "\"ac.module.import\"() {sym_name = \"unit.Declared\", "
           "source_owner = {package = \"\", path = \"unit.py\"}, "
           "parameters = [], type_parameters = " +
           typeParameters.str() + ", function_type = " + signature.str() +
           ", input_names = [\"input\"], output_names = [\"output\"], "
           "dependency_summary = [{output = {port = 0 : i64, path = []}, "
           "inputs = []}]} : () -> ()\n";
  }
  ac::ModuleImportOp imported(mlir::ModuleOp unit) {
    return cast<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(unit, "unit.Declared"));
  }
  ac::StructType record(StringRef name) {
    return ac::StructType::get(&context, StringAttr::get(&context, name));
  }
  ac::StaticExprAttr integer(StringRef value) {
    return cast<ac::StaticExprAttr>(parseAttribute(
        R"mlir(#ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Declared, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<)mlir" +
            value.str() + ">}}>",
        &context));
  }
  ArrayAttr path(const std::vector<std::string> &names) {
    Builder b(&context);
    SmallVector<Attribute> attrs;
    for (const auto &name : names)
      attrs.push_back(b.getStringAttr(name));
    return b.getArrayAttr(attrs);
  }
  DictionaryAttr endpoint(const Endpoint &value) {
    Builder b(&context);
    return b.getDictionaryAttr(
        {b.getNamedAttr("port", b.getI64IntegerAttr(value.port)),
         b.getNamedAttr("path", path(value.path))});
  }
  void setSummary(ac::ModuleImportOp declaration,
                  const std::vector<Row> &rows) {
    Builder b(&context);
    SmallVector<Attribute> entries;
    for (const Row &row : rows) {
      SmallVector<Attribute> inputs;
      for (const auto &input : row.inputs)
        inputs.push_back(endpoint(input));
      entries.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("output", endpoint(row.output)),
           b.getNamedAttr("inputs", b.getArrayAttr(inputs))}));
    }
    declaration->setAttr("dependency_summary", b.getArrayAttr(entries));
  }
  void expectRows(mlir::ModuleOp unit, ac::ModuleImportOp declaration,
                  const std::vector<Row> &expected) {
    ac::HardwareAnalysis analysis(unit);
    auto rows = analysis.getImportDependencies(declaration);
    ASSERT_TRUE(succeeded(rows)) << diagnostics;
    ASSERT_EQ(rows->size(), expected.size());
    for (auto [i, row] : llvm::enumerate(*rows)) {
      EXPECT_EQ(row.output.port, expected[i].output.port);
      EXPECT_EQ(ArrayAttr::get(&context,
                               SmallVector<Attribute>(row.output.path.begin(),
                                                      row.output.path.end())),
                path(expected[i].output.path));
      ASSERT_EQ(row.inputs.size(), expected[i].inputs.size());
      for (auto [j, input] : llvm::enumerate(row.inputs)) {
        EXPECT_EQ(input.port, expected[i].inputs[j].port);
        EXPECT_EQ(
            ArrayAttr::get(&context, SmallVector<Attribute>(input.path.begin(),
                                                            input.path.end())),
            path(expected[i].inputs[j].path));
      }
    }
  }
  DictionaryAttr owner(StringRef source) {
    Builder b(&context);
    return b.getDictionaryAttr(
        {b.getNamedAttr("package", b.getStringAttr("")),
         b.getNamedAttr("path", b.getStringAttr(source))});
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
  LogicalResult extract(mlir::ModuleOp unit) {
    Builder b(&context);
    unit->setAttr("ac.stage", b.getStringAttr("source"));
    unit->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
    unit->setAttr("ac.source_owner", owner("unit.py"));
    unit->setAttr("ac.interfaces",
                  b.getArrayAttr({owner("unit.py"), owner("types.py")}));
    unit->setAttr("ac.import_bindings", b.getArrayAttr({}));
    SmallVector<Attribute> exports;
    for (Operation &op : unit.getBody()->getOperations()) {
      auto symbol = SymbolTable::getSymbolName(&op);
      op.setAttr("ac.origin", origin(symbol.getValue()));
      if (isa<ac::StructOp>(op)) {
        op.setAttr("ac.source_owner", owner("types.py"));
        op.setAttr("ac.declaration_role", b.getStringAttr("import_snapshot"));
      } else if (isa<ac::ModuleOp>(op)) {
        op.setAttr("ac.declaration_role", b.getStringAttr("definition"));
        NamedAttrList location;
        location.append("path", b.getStringAttr("unit.py"));
        for (StringRef key : {"line", "column", "end_line", "end_column"})
          location.append(key,
                          b.getI64IntegerAttr(key == "end_column" ? 2 : 1));
        exports.push_back(b.getDictionaryAttr(
            {b.getNamedAttr(
                 "name", b.getStringAttr(symbol.getValue().rsplit('.').second)),
             b.getNamedAttr(
                 "target", FlatSymbolRefAttr::get(&context, symbol.getValue())),
             b.getNamedAttr(
                 "site", b.getDictionaryAttr(
                             {b.getNamedAttr("ast_path", b.getArrayAttr({})),
                              b.getNamedAttr("location", location.getDictionary(
                                                             &context))}))}));
      }
    }
    unit->setAttr("ac.exports", b.getArrayAttr(exports));
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return success();
    });
    PassManager pm(&context);
    pm.addPass(acir::createExtractSourceInterfacePass());
    return pm.run(unit);
  }
};

TEST_F(TableDependencySummaryTest,
       BitsPayloadRelationsKeepOriginalTableEndpoints) {
  for (StringRef signature : {"(!t3) -> !b8", "(!b8) -> !t3", "(!t2) -> !t3"}) {
    SCOPED_TRACE(signature.str());
    auto unit = parse(declaration(signature));
    ASSERT_TRUE(unit);
    const std::vector<Row> expected{{{0, {}}, {{0, {}}}}};
    setSummary(imported(*unit), expected);
    ASSERT_TRUE(succeeded(mlir::verify(*unit)));
    expectRows(*unit, imported(*unit), expected);
  }
}

TEST_F(TableDependencySummaryTest,
       NominalAggregatePrefixesExpandOnlyCorrespondingFields) {
  for (StringRef signature :
       {"(!pair3) -> !ac.struct<\"types.Pair\">",
        "(!ac.struct<\"types.Pair\">) -> !pair2", "(!pair3) -> !pair2"}) {
    SCOPED_TRACE(signature.str());
    auto unit = parse(declaration(signature));
    ASSERT_TRUE(unit);
    setSummary(imported(*unit), {{{0, {}}, {{0, {}}}}});
    ASSERT_TRUE(succeeded(mlir::verify(*unit)));
    expectRows(
        *unit, imported(*unit),
        {{{0, {"left"}}, {{0, {"left"}}}}, {{0, {"right"}}, {{0, {"right"}}}}});
  }
}

TEST_F(TableDependencySummaryTest,
       OpaqueFormalIdentityRemainsStrictAfterTablePeeling) {
  for (bool matching : {true, false}) {
    auto unit = parse(declaration(
        matching ? "(!ac.table<[#w3], !ac.type_param<@unit.Declared, \"T\">>) "
                   "-> !ac.type_param<@unit.Declared, \"T\">"
                 : "(!ac.table<[#w3], !ac.type_param<@unit.Declared, \"T\">>) "
                   "-> !ac.type_param<@unit.Declared, \"U\">",
        "[\"T\", \"U\"]"));
    ASSERT_TRUE(unit);
    setSummary(imported(*unit), {{{0, {}}, {{0, {}}}}});
    ac::HardwareAnalysis analysis(*unit);
    EXPECT_EQ(succeeded(analysis.getImportDependencies(imported(*unit))),
              matching);
  }
}

TEST_F(TableDependencySummaryTest,
       InvalidLogicalAggregateRelationsAndCoverageStayRejected) {
  for (unsigned mutation = 0; mutation != 6; ++mutation) {
    SCOPED_TRACE(mutation);
    StringRef signature =
        mutation == 0   ? "(!pair3) -> !ac.struct<\"types.Other\">"
        : mutation == 1 ? "(!pair3) -> !b8"
                        : "(!pair3) -> !ac.struct<\"types.Pair\">";
    auto unit = parse(declaration(signature));
    ASSERT_TRUE(unit);
    std::vector<Row> rows{{{0, {}}, {{0, {}}}}};
    if (mutation == 2)
      rows = {{{0, {"left"}}, {{0, {"missing"}}}}, {{0, {"right"}}, {}}};
    if (mutation == 3)
      rows = {{{0, {"left"}}, {{0, {"left"}}, {0, {"left"}}}},
              {{0, {"right"}}, {}}};
    if (mutation == 4)
      rows.push_back({{0, {"left"}}, {}});
    if (mutation == 5)
      rows = {{{0, {"left"}}, {{0, {"left"}}}}};
    setSummary(imported(*unit), rows);
    ac::HardwareAnalysis analysis(*unit);
    EXPECT_TRUE(failed(analysis.getImportDependencies(imported(*unit))));
  }
}

TEST_F(TableDependencySummaryTest,
       ShapeChangingViewExtractsTruthfulCompactSummary) {
  auto unit = parse(R"mlir(
    "ac.module"() ({
    ^bb0(%input: !t3):
      %out = "ac.table.view"(%input) {kind = "slice", parameters = {offsets = [#w0], sizes = [#w2], strides = [#w1]}} : (!t3) -> !t2
      "ac.yield"(%out) : (!t2) -> ()
    }) {sym_name = "unit.View", source_owner = {package = "", path = "unit.py"}, parameters = [], type_parameters = [], function_type = (!t3) -> !t2, input_names = ["input"], output_names = ["out"]} : () -> ()
  )mlir");
  ASSERT_TRUE(unit);
  auto module =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*unit, "unit.View"));
  auto signature = module.getFunctionTypeAttr();
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  auto result =
      cast<ac::ModuleImportOp>(SymbolTable::lookupSymbolIn(*unit, "unit.View"));
  EXPECT_EQ(result.getFunctionTypeAttr(), signature);
  expectRows(*unit, result, {{{0, {}}, {{0, {}}}}});
}

TEST_F(TableDependencySummaryTest,
       TableStructGetHasExactFieldAndIndexDependencies) {
  auto unit = parse(R"mlir(
    "ac.module"() ({
    ^bb0(%input: !pair3, %index: !b8):
      %picked, %valid = "ac.table.get"(%input, %index) : (!pair3, !b8) -> (!ac.struct<"types.Pair">, !b1)
      %left = ac.struct.get %picked["left"] : (!ac.struct<"types.Pair">) -> !b8
      "ac.yield"(%left, %valid) : (!b8, !b1) -> ()
    }) {sym_name = "unit.Project", source_owner = {package = "", path = "unit.py"}, parameters = [], type_parameters = [], function_type = (!pair3, !b8) -> (!b8, !b1), input_names = ["input", "index"], output_names = ["left", "valid"]} : () -> ()
  )mlir");
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  auto result = cast<ac::ModuleImportOp>(
      SymbolTable::lookupSymbolIn(*unit, "unit.Project"));
  expectRows(*unit, result,
             {{{0, {}}, {{0, {"left"}}, {1, {}}}}, {{1, {}}, {{1, {}}}}});
}

TEST_F(TableDependencySummaryTest,
       ViewAndInstanceShapeRulesRemainIndependentAndStrict) {
  auto unit = parse(declaration("(!t2) -> !t2") + R"mlir(
    "ac.module"() ({
    ^bb0(%correct: !t2, %wrong: !t3):
      %out = "ac.instance"(%correct) {instance_name = "child", callee = @unit.Declared, parameters = [], type_arguments = [], occurrence = {site = {definition = @unit.Parent, ast_path = []}, expansion = []}} : (!t2) -> !t2
      "ac.yield"(%out) : (!t2) -> ()
    }) {sym_name = "unit.Parent", source_owner = {package = "", path = "unit.py"}, parameters = [], type_parameters = [], function_type = (!t2, !t3) -> !t2, input_names = ["correct", "wrong"], output_names = ["out"]} : () -> ()
  )mlir");
  ASSERT_TRUE(unit);
  auto module =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*unit, "unit.Parent"));
  auto instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  instance->setOperand(0, module.getBody().front().getArgument(1));
  EXPECT_TRUE(failed(mlir::verify(*unit)));
  auto malformed = parse(R"mlir(
    "ac.module"() ({
    ^bb0(%input: !t3):
      %out = "ac.table.view"(%input) {kind = "reshape", parameters = {shape = [#w2]}} : (!t3) -> !t2
      "ac.yield"(%out) : (!t2) -> ()
    }) {sym_name = "unit.BadView", source_owner = {package = "", path = "unit.py"}, parameters = [], type_parameters = [], function_type = (!t3) -> !t2, input_names = ["input"], output_names = ["out"]} : () -> ()
  )mlir");
  EXPECT_FALSE(malformed);
}

TEST_F(TableDependencySummaryTest,
       NestedTablesKeepCompactLeavesAndMostSignificantFieldOrder) {
  auto unit = parse(R"mlir(
    "ac.enum"() {sym_name = "types.Mode", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<5>}]} : () -> ()
    ac.struct "types.Cell" fields [{name = "header", type = !b1}, {name = "lanes", type = !t3}]
    ac.struct "types.Envelope" fields [{name = "prefix", type = !b8}, {name = "cells", type = !ac.table<[#w3], !ac.struct<"types.Cell">>}, {name = "suffix", type = !b1}]
    ac.struct "types.Shared" fields [{name = "first", type = !ac.struct<"types.Cell">}, {name = "second", type = !ac.struct<"types.Cell">}]
    ac.struct "types.Modes" fields [{name = "modes", type = !ac.table<[#w3], !ac.enum<"types.Mode">>}]
  )mlir");
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(mlir::verify(*unit)));
  ac::HardwareAnalysis analysis(*unit);
  auto envelope = record("types.Envelope");
  auto leaves = analysis.getLeaves(envelope, {}, *unit);
  ASSERT_TRUE(succeeded(leaves));
  ASSERT_EQ(leaves->size(), 3u);
  const std::vector<std::string> names{"prefix", "cells", "suffix"};
  const uint64_t lows[]{76, 1, 0}, widths[]{8, 75, 1};
  for (unsigned i = 0; i != 3; ++i) {
    ASSERT_EQ((*leaves)[i].path.size(), 1u);
    EXPECT_EQ((*leaves)[i].path.front().getValue(), names[i]);
    EXPECT_EQ((*leaves)[i].low, lows[i]);
    EXPECT_EQ((*leaves)[i].width, widths[i]);
  }
  EXPECT_TRUE(isa<ac::TableType>((*leaves)[1].type));
  auto width = analysis.getPackedWidth(envelope, {}, *unit);
  ASSERT_TRUE(succeeded(width));
  EXPECT_EQ(*width, 84u); // 8 + 3 * (1 + 3 * 8) + 1, crossing 64 bits.
  auto shared = analysis.getPackedWidth(
      record("types.Shared"), {}, *unit);
  ASSERT_TRUE(succeeded(shared));
  EXPECT_EQ(*shared, 50u); // Repeated acyclic children are not recursion.
  auto modes = analysis.getPackedWidth(
      record("types.Modes"), {}, *unit);
  ASSERT_TRUE(succeeded(modes));
  EXPECT_EQ(*modes, 9u);
}

TEST_F(TableDependencySummaryTest,
       AlternatingStructTablePathsRetainExactDependencyLeaves) {
  auto unit = parse(R"mlir(
    ac.struct "types.Cell" fields [{name = "header", type = !b1}, {name = "lanes", type = !t3}]
    ac.struct "types.Envelope" fields [{name = "prefix", type = !b8}, {name = "cells", type = !ac.table<[#w3], !ac.struct<"types.Cell">>}, {name = "suffix", type = !b1}]
  )mlir" + declaration("(!ac.table<[#w2], !ac.struct<\"types.Envelope\">>) -> !ac.struct<\"types.Envelope\">"));
  ASSERT_TRUE(unit);
  setSummary(imported(*unit), {{{0, {}}, {{0, {}}}}});
  ASSERT_TRUE(succeeded(mlir::verify(*unit)));
  expectRows(*unit, imported(*unit),
             {{{0, {"prefix"}}, {{0, {"prefix"}}}},
              {{0, {"cells", "header"}}, {{0, {"cells", "header"}}}},
              {{0, {"cells", "lanes"}}, {{0, {"cells", "lanes"}}}},
              {{0, {"suffix"}}, {{0, {"suffix"}}}}});
  ac::HardwareAnalysis analysis(*unit);
  auto table = imported(*unit).getFunctionType().getInput(0);
  auto width = analysis.getPackedWidth(table, {}, *unit);
  ASSERT_TRUE(succeeded(width));
  EXPECT_EQ(*width, 168u); // Enclosing multiplicity two, field extent three.
  auto leaves = analysis.getLeaves(table, {}, *unit);
  ASSERT_TRUE(succeeded(leaves));
  ASSERT_EQ(leaves->size(), 1u);
  EXPECT_TRUE(leaves->front().path.empty());
  EXPECT_EQ(leaves->front().width, 168u);
}

TEST_F(TableDependencySummaryTest,
       CyclesThroughTablesAndUnresolvedNominalsFailLayout) {
  for (StringRef fields : {
           "ac.struct \"types.Cycle\" fields [{name = \"entries\", type = !ac.table<[#w3], !ac.struct<\"types.Cycle\">>}]",
           "ac.struct \"types.Cycle\" fields [{name = \"entries\", type = !ac.table<[#w3], !ac.struct<\"types.Child\">>}]\nac.struct \"types.Child\" fields [{name = \"parent\", type = !ac.struct<\"types.Cycle\">}]",
           "ac.struct \"types.Cycle\" fields [{name = \"entries\", type = !ac.table<[#w3], !ac.struct<\"types.Missing\">>}]"}) {
    SCOPED_TRACE(fields.str());
    auto unit = parse(fields, false);
    ASSERT_TRUE(unit);
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return success();
    });
    ac::HardwareAnalysis analysis(*unit);
    EXPECT_TRUE(failed(mlir::verify(*unit)));
    EXPECT_TRUE(failed(analysis.getPackedWidth(
        record("types.Cycle"), {}, *unit)));
    EXPECT_TRUE(failed(analysis.getLeaves(
        record("types.Cycle"), {}, *unit)));
    EXPECT_TRUE(diagnostics.find("recursive packed struct layout") !=
                    std::string::npos ||
                diagnostics.find("unresolved nominal hardware struct") !=
                    std::string::npos)
        << diagnostics;
  }
}

TEST_F(TableDependencySummaryTest,
       InvalidNestedExtentsAndCheckedLayoutArithmeticRejectBeforeEmission) {
  Builder b(&context);
  auto byte = ac::BitsType::get(&context, integer("8"));
  auto huge = ac::BitsType::get(&context, integer("18446744073709551615"));
  const std::vector<std::vector<Type>> cases{
      {ac::TableType::get(&context, b.getArrayAttr({integer("0")}), byte)},
      {ac::TableType::get(&context, b.getArrayAttr({integer("-1")}), byte)},
      {ac::TableType::get(&context,
                         b.getArrayAttr({integer("9223372036854775807"),
                                         integer("3")}), byte)},
      {ac::TableType::get(&context, b.getArrayAttr({integer("3")}), huge)},
      {huge, byte}};
  for (auto [i, types] : llvm::enumerate(cases)) {
    SCOPED_TRACE(i);
    auto unit = parse("");
    ASSERT_TRUE(unit);
    auto declaration = cast<ac::StructOp>(
        SymbolTable::lookupSymbolIn(*unit, "types.Pair"));
    SmallVector<Attribute> fields;
    for (auto [j, type] : llvm::enumerate(types))
      fields.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr("field" + std::to_string(j))),
           b.getNamedAttr("type", TypeAttr::get(type))}));
    declaration->setAttr("fields", b.getArrayAttr(fields));
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return success();
    });
    EXPECT_TRUE(failed(mlir::verify(*unit)));
    ac::HardwareAnalysis analysis(*unit);
    EXPECT_TRUE(failed(analysis.getPackedWidth(record("types.Pair"), {}, *unit)));
    EXPECT_TRUE(failed(analysis.getLeaves(record("types.Pair"), {}, *unit)));
    EXPECT_TRUE(diagnostics.find("positive") != std::string::npos ||
                diagnostics.find("exceeds signed 64 bits") != std::string::npos ||
                diagnostics.find("overflow") != std::string::npos)
        << diagnostics;
  }
}
} // namespace
