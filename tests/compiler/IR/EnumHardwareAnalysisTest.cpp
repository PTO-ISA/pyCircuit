#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
using namespace mlir;

std::string literal(unsigned value) {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
         "value = #ac.math_int<" + std::to_string(value) +
         ">}, origin = {site = {definition = @top, ast_path = []}, "
         "expansion = []}, location = {path = \"top.py\", line = 1 : i64, "
         "column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>";
}
std::string preamble() {
  std::string out;
  for (unsigned width : {0u, 1u, 2u, 3u, 65u, 130u})
    out += "#w" + std::to_string(width) + " = " + literal(width) + "\n";
  for (unsigned width : {1u, 3u, 65u, 130u})
    out += "!b" + std::to_string(width) + " = !ac.bits<#w" +
           std::to_string(width) + ">\n";
  return out + R"mlir(
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
!table = !ac.table<[#w2, #w3], !outer>
)mlir";
}
const char *declarations = R"mlir(
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
)mlir";

struct Endpoint { unsigned port; std::vector<std::string> path; };
struct Row { Endpoint output; std::vector<Endpoint> inputs; };

class EnumHardwareAnalysisTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  OwningOpRef<mlir::ModuleOp> parse(StringRef body) {
    return parseSourceString<mlir::ModuleOp>(
        preamble() + "module {\n" + declarations + body.str() + "\n}",
        &context);
  }
  ac::EnumType nominal(StringRef name) {
    return ac::EnumType::get(&context, StringAttr::get(&context, name));
  }
  ac::StructType record(StringRef name) {
    return ac::StructType::get(&context, StringAttr::get(&context, name));
  }
  std::vector<std::string> names(ArrayRef<StringAttr> path) {
    std::vector<std::string> out;
    for (auto item : path) out.push_back(item.getValue().str());
    return out;
  }
  void expectRows(ArrayRef<ac::OutputDependency> actual,
                  const std::vector<Row> &expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (auto [i, row] : llvm::enumerate(actual)) {
      EXPECT_EQ(row.output.port, expected[i].output.port);
      EXPECT_EQ(names(row.output.path), expected[i].output.path);
      ASSERT_EQ(row.inputs.size(), expected[i].inputs.size());
      for (auto [j, input] : llvm::enumerate(row.inputs)) {
        EXPECT_EQ(input.port, expected[i].inputs[j].port);
        EXPECT_EQ(names(input.path), expected[i].inputs[j].path);
      }
    }
  }
  ArrayAttr path(const std::vector<std::string> &items) {
    Builder b(&context); SmallVector<Attribute> out;
    for (const auto &item : items) out.push_back(b.getStringAttr(item));
    return b.getArrayAttr(out);
  }
  DictionaryAttr endpoint(const Endpoint &value) {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("port", b.getI64IntegerAttr(value.port)),
                                b.getNamedAttr("path", path(value.path))});
  }
  void setSummary(ac::ModuleImportOp declaration, const std::vector<Row> &rows) {
    Builder b(&context); SmallVector<Attribute> out;
    for (const Row &row : rows) {
      SmallVector<Attribute> inputs;
      for (const auto &input : row.inputs) inputs.push_back(endpoint(input));
      out.push_back(b.getDictionaryAttr({b.getNamedAttr("output", endpoint(row.output)),
                                        b.getNamedAttr("inputs", b.getArrayAttr(inputs))}));
    }
    declaration->setAttr("dependency_summary", b.getArrayAttr(out));
  }
};

TEST_F(EnumHardwareAnalysisTest, NominalLeavesUseDeclaredWidthsAndFieldOrder) {
  auto unit = parse(""); ASSERT_TRUE(unit);
  ac::HardwareAnalysis analysis(*unit);
  auto a = nominal("alpha.State"), wide = nominal("types.Wide");
  auto outer = record("types.Outer");
  auto scalar = analysis.getLeaves(a, {}, *unit);
  ASSERT_TRUE(succeeded(scalar)); ASSERT_EQ(scalar->size(), 1u);
  EXPECT_EQ(scalar->front().type, a); EXPECT_TRUE(scalar->front().path.empty());
  EXPECT_EQ(scalar->front().low, 0u); EXPECT_EQ(scalar->front().width, 65u);
  auto leaves = analysis.getLeaves(outer, {}, *unit);
  ASSERT_TRUE(succeeded(leaves)); ASSERT_EQ(leaves->size(), 4u);
  // Declared MSB-first fields: 65 + (130 + 1) + 3 = 199, without padding.
  const std::vector<std::vector<std::string>> expectedPaths{
      {"tag"}, {"inner", "big"}, {"inner", "flag"}, {"tail"}};
  const uint64_t lows[] = {134, 4, 3, 0}, widths[] = {65, 130, 1, 3};
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(names((*leaves)[i].path), expectedPaths[i]);
    EXPECT_EQ((*leaves)[i].low, lows[i]); EXPECT_EQ((*leaves)[i].width, widths[i]);
  }
  EXPECT_EQ((*leaves)[0].type, a); EXPECT_EQ((*leaves)[1].type, wide);
  auto total = analysis.getPackedWidth(outer, {}, *unit);
  ASSERT_TRUE(succeeded(total)); EXPECT_EQ(*total, 199u);
  auto fields = analysis.getFieldPaths(outer, {}, *unit);
  ASSERT_TRUE(succeeded(fields)); ASSERT_EQ(fields->size(), 4u);
  for (unsigned i = 0; i < 4; ++i) EXPECT_EQ(names((*fields)[i]), expectedPaths[i]);
  auto shape = ArrayAttr::get(&context, {
      parseAttribute(literal(2), &context), parseAttribute(literal(3), &context)});
  auto table = ac::TableType::get(&context, shape, outer);
  auto packed = analysis.getLeaves(table, {}, *unit);
  ASSERT_TRUE(succeeded(packed)); ASSERT_EQ(packed->size(), 1u);
  EXPECT_EQ(packed->front().type, table); EXPECT_TRUE(packed->front().path.empty());
  EXPECT_EQ(packed->front().low, 0u); EXPECT_EQ(packed->front().width, 1194u);
}

TEST_F(EnumHardwareAnalysisTest, OneGenericDefinitionRetainsEachBindingIdentity) {
  auto unit = parse(R"mlir(
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@forward, "T">, %lanes: !ac.table<[#w2], !ac.type_param<@forward, "T">>):
    "ac.yield"(%value, %lanes) : (!ac.type_param<@forward, "T">, !ac.table<[#w2], !ac.type_param<@forward, "T">>) -> ()
  }) {sym_name = "forward", source_owner = {package = "", path = "forward.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@forward, "T">, !ac.table<[#w2], !ac.type_param<@forward, "T">>) -> (!ac.type_param<@forward, "T">, !ac.table<[#w2], !ac.type_param<@forward, "T">>), input_names = ["value", "lanes"], output_names = ["copy", "table_copy"]} : () -> ()
  "ac.module"() ({
  ^bb0(%a: !a, %b: !b, %wide: !wide, %ta: !ac.table<[#w2], !a>, %tb: !ac.table<[#w2], !b>, %tw: !ac.table<[#w2], !wide>):
    %x, %x_table = "ac.instance"(%a, %ta) {instance_name = "x", callee = @forward, parameters = [], type_arguments = [!a], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!a, !ac.table<[#w2], !a>) -> (!a, !ac.table<[#w2], !a>)
    %y, %y_table = "ac.instance"(%b, %tb) {instance_name = "y", callee = @forward, parameters = [], type_arguments = [!b], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b, !ac.table<[#w2], !b>) -> (!b, !ac.table<[#w2], !b>)
    %z, %z_table = "ac.instance"(%wide, %tw) {instance_name = "z", callee = @forward, parameters = [], type_arguments = [!wide], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!wide, !ac.table<[#w2], !wide>) -> (!wide, !ac.table<[#w2], !wide>)
    "ac.yield"(%x, %y, %z, %x_table, %y_table, %z_table) : (!a, !b, !wide, !ac.table<[#w2], !a>, !ac.table<[#w2], !b>, !ac.table<[#w2], !wide>) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b, !wide, !ac.table<[#w2], !a>, !ac.table<[#w2], !b>, !ac.table<[#w2], !wide>) -> (!a, !b, !wide, !ac.table<[#w2], !a>, !ac.table<[#w2], !b>, !ac.table<[#w2], !wide>), input_names = ["a", "b", "wide", "ta", "tb", "tw"], output_names = ["x", "y", "z", "tx", "ty", "tz"]} : () -> ()
)mlir"); ASSERT_TRUE(unit);
  ac::HardwareAnalysis analysis(*unit);
  auto forward = cast<ac::ModuleOp>(analysis.lookupDefinition("forward"));
  auto top = cast<ac::ModuleOp>(analysis.lookupDefinition("top"));
  SmallVector<ac::InstanceOp> instances;
  for (auto instance : top.getBody().front().getOps<ac::InstanceOp>())
    instances.push_back(instance);
  ASSERT_EQ(instances.size(), 3u);
  const Type expected[] = {nominal("alpha.State"), nominal("beta.State"), nominal("types.Wide")};
  const uint64_t widths[] = {65, 65, 130};
  for (unsigned i : {0u, 1u, 2u, 0u}) {
    auto bound = analysis.bindInstance(instances[i]); ASSERT_TRUE(succeeded(bound));
    auto resolved = analysis.resolveType(forward.getFunctionType().getInput(0), *bound, forward);
    ASSERT_TRUE(succeeded(resolved)); EXPECT_EQ(*resolved, expected[i]);
    auto width = analysis.getPackedWidth(forward.getFunctionType().getInput(0), *bound, forward);
    ASSERT_TRUE(succeeded(width)); EXPECT_EQ(*width, widths[i]);
    auto leaves = analysis.getLeaves(forward.getFunctionType().getInput(0), *bound, forward);
    ASSERT_TRUE(succeeded(leaves)); ASSERT_EQ(leaves->size(), 1u);
    EXPECT_EQ(leaves->front().type, expected[i]);
    auto table = analysis.resolveType(forward.getFunctionType().getInput(1), *bound, forward);
    ASSERT_TRUE(succeeded(table));
    EXPECT_EQ(cast<ac::TableType>(*table).getElementType(), expected[i]);
    auto tableWidth = analysis.getPackedWidth(forward.getFunctionType().getInput(1), *bound, forward);
    ASSERT_TRUE(succeeded(tableWidth)); EXPECT_EQ(*tableWidth, widths[i] * 2);
  }
  EXPECT_FALSE(ac::areEquivalentHardwareTypes(expected[0], expected[1]));
  EXPECT_TRUE(ac::areEquivalentHardwareTypes(expected[0], nominal("alpha.State")));
}

TEST_F(EnumHardwareAnalysisTest, ConversionsHaveSeparateScalarDependencies) {
  auto unit = parse(R"mlir(
  "ac.module"() ({
  ^bb0(%state: !a, %raw: !b65, %packet: !outer):
    %constant = "ac.enum.create"() {member = "ONE"} : () -> !a
    %bits = "ac.enum.to_bits"(%state) : (!a) -> !b65
    %converted, %member = "ac.enum.from_bits"(%raw) : (!b65) -> (!b, !b1)
    %inner = "ac.struct.get"(%packet) {field = "inner"} : (!outer) -> !inner
    %big = "ac.struct.get"(%inner) {field = "big"} : (!inner) -> !wide
    %big_bits = "ac.enum.to_bits"(%big) : (!wide) -> !b130
    %unused, %unused_member = "ac.enum.from_bits"(%bits) : (!b65) -> (!b, !b1)
    "ac.yield"(%constant, %bits, %converted, %member, %big_bits) : (!a, !b65, !b, !b1, !b130) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b65, !outer) -> (!a, !b65, !b, !b1, !b130), input_names = ["state", "raw", "packet"], output_names = ["constant", "bits", "converted", "member", "big_bits"]} : () -> ()
)mlir"); ASSERT_TRUE(unit);
  ac::HardwareAnalysis analysis(*unit);
  auto top = cast<ac::ModuleOp>(analysis.lookupDefinition("top"));
  auto facts = analysis.analyzeModule(top); ASSERT_TRUE(succeeded(facts));
  expectRows(facts->dependencies, {{{0, {}}, {}}, {{1, {}}, {{0, {}}}},
      {{2, {}}, {{1, {}}}}, {{3, {}}, {{1, {}}}},
      {{4, {}}, {{2, {"inner", "big"}}}}});
  unsigned conversions = 0;
  for (auto op : top.getBody().front().getOps<ac::EnumFromBitsOp>()) {
    ++conversions;
    for (auto result : op->getResults()) {
      auto edges = analysis.getDependencies(result, {});
      ASSERT_TRUE(succeeded(edges)); ASSERT_EQ(edges->size(), 1u);
      EXPECT_EQ(edges->front().value, op.getInput());
      EXPECT_TRUE(edges->front().path.empty());
    }
  }
  EXPECT_EQ(conversions, 2u);
}

TEST_F(EnumHardwareAnalysisTest, ImportedScalarSummariesCrossExplicitConversions) {
  auto unit = parse(R"mlir(
  "ac.module.import"() {sym_name = "provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!a, !b65) -> (!b65, !b, !b1, !b), input_names = ["state", "raw"], output_names = ["bits", "converted", "member", "reinterpreted"], dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = [{port = 0 : i64, path = []}]}, {output = {port = 1 : i64, path = []}, inputs = [{port = 1 : i64, path = []}]}, {output = {port = 2 : i64, path = []}, inputs = [{port = 1 : i64, path = []}]}, {output = {port = 3 : i64, path = []}, inputs = [{port = 0 : i64, path = []}]}]} : () -> ()
)mlir"); ASSERT_TRUE(unit);
  ac::HardwareAnalysis analysis(*unit);
  auto provider = cast<ac::ModuleImportOp>(analysis.lookupDefinition("provider"));
  auto rows = analysis.getImportDependencies(provider); ASSERT_TRUE(succeeded(rows));
  const std::vector<Row> expected{{{0, {}}, {{0, {}}}}, {{1, {}}, {{1, {}}}},
      {{2, {}}, {{1, {}}}}, {{3, {}}, {{0, {}}}}};
  expectRows(*rows, expected);
  // Mutate only a semantic endpoint after native parsing; no earlier malformed
  // ownership/type declaration can mask the exact summary coverage guard.
  ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
    llvm::raw_string_ostream out(diagnostics); d.print(out); return success();
  });
  auto incomplete = expected; incomplete.pop_back(); setSummary(provider, incomplete);
  EXPECT_TRUE(failed(analysis.getImportDependencies(provider)));
  EXPECT_NE(diagnostics.find("every output leaf exactly once"), std::string::npos);
  diagnostics.clear();
  auto projected = expected; projected[0].inputs[0].path = {"code"};
  setSummary(provider, projected);
  EXPECT_TRUE(failed(analysis.getImportDependencies(provider)));
  EXPECT_NE(diagnostics.find("non-struct payload"), std::string::npos);
}
} // namespace
