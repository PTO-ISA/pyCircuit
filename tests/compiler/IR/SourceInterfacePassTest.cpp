#include "Compiler/SourceUnit.h"
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
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
using namespace mlir;

const char *types = R"mlir(
#w1 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Child, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Child, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Child, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Child, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#n = #ac.static_expr<{kind = "reference", location = {path = "unit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @unit.Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @unit.Generic, name = "N"}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w3], !b8>
!controls = !ac.table<[#w3], !b1>
)mlir";

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
std::string child() {
  return module("Child", R"mlir(^bb0(%x: !b8):
    "ac.yield"(%x) : (!b8) -> ()
  )mlir",
                "(!b8) -> !b8", "[\"x\"]", "[\"y\"]");
}
std::string parent() {
  return module("Parent", R"mlir(^bb0(%unused: !b8, %x: !b8):
    %out = "ac.instance"(%x) {instance_name = "leaf", callee = @unit.Child, parameters = [], type_arguments = [], occurrence = {site = {definition = @unit.Parent, ast_path = []}, expansion = []}} : (!b8) -> !b8
    "ac.yield"(%out) : (!b8) -> ()
  )mlir",
                "(!b8, !b8) -> !b8", "[\"unused\", \"x\"]", "[\"y\"]");
}

class SourceInterfacePassTest : public ::testing::Test {
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
        std::string(types) + "module {\n" + content.str() + "\n}", &context);
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
                         (isa<ac::StructOp>(op) &&
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
  LogicalResult extract(mlir::ModuleOp unit) {
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return success();
    });
    PassManager manager(&context);
    manager.addPass(acir::createExtractSourceInterfacePass());
    return manager.run(unit);
  }
  ac::ModuleImportOp lookup(mlir::ModuleOp unit, StringRef name) {
    return dyn_cast_or_null<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(unit, name));
  }
  void dependencies(ac::ModuleImportOp declaration,
                    const std::vector<std::vector<unsigned>> &expected) {
    ASSERT_TRUE(declaration);
    auto rows = declaration->getAttrOfType<ArrayAttr>("dependency_summary");
    ASSERT_TRUE(rows);
    ASSERT_EQ(rows.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
      auto row = cast<DictionaryAttr>(rows[i]);
      auto output = row.getAs<DictionaryAttr>("output");
      EXPECT_EQ(output.getAs<IntegerAttr>("port").getInt(),
                static_cast<int64_t>(i));
      EXPECT_TRUE(output.getAs<ArrayAttr>("path").empty());
      auto inputs = row.getAs<ArrayAttr>("inputs");
      ASSERT_EQ(inputs.size(), expected[i].size());
      for (size_t j = 0; j < inputs.size(); ++j) {
        auto endpoint = cast<DictionaryAttr>(inputs[j]);
        EXPECT_EQ(endpoint.getAs<IntegerAttr>("port").getInt(), expected[i][j]);
        EXPECT_TRUE(endpoint.getAs<ArrayAttr>("path").empty());
      }
    }
  }
};

TEST_F(SourceInterfacePassTest, AllOriginalCalleesRemainVisibleInBothOrders) {
  for (bool reversed : {false, true}) {
    auto unit = parse(reversed ? parent() + child() : child() + parent());
    ASSERT_TRUE(unit);
    ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
    dependencies(lookup(*unit, "unit.Child"), {{0}});
    dependencies(lookup(*unit, "unit.Parent"), {{1}});
    EXPECT_TRUE(unit->getOps<ac::ModuleOp>().empty());
    EXPECT_TRUE(unit->getOps<ac::SystemOp>().empty());
    EXPECT_EQ((*unit)->getAttrOfType<StringAttr>("ac.unit_kind").getValue(),
              "interface");
  }
}

TEST_F(SourceInterfacePassTest,
       RuleCaptureFanoutRetainsDistinctRealDependencies) {
  auto unit = parse(
      module("Fanout", R"mlir(^bb0(%a: !b8, %b: !b8, %unused: !b8):
    %both = "ac.rule"(%a, %b) ({
    ^bb0(%x: !b8, %y: !b8):
      %value = "ac.bits.binary"(%x, %y) {opcode = "xor"} : (!b8, !b8) -> !b8
      "ac.yield"(%value) : (!b8) -> ()
    }) {name = "combine", occurrence = {site = {definition = @unit.Fanout, ast_path = []}, expansion = []}} : (!b8, !b8) -> !b8
    "ac.yield"(%a, %both, %both) : (!b8, !b8, !b8) -> ()
  )mlir",
             "(!b8, !b8, !b8) -> (!b8, !b8, !b8)", "[\"a\", \"b\", \"unused\"]",
             "[\"direct\", \"mixed\", \"fanout\"]"));
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  dependencies(lookup(*unit, "unit.Fanout"), {{0}, {0, 1}, {0, 1}});
}

TEST_F(SourceInterfacePassTest,
       ActualCollectionMapFoldAndSignatureArePreserved) {
  auto unit = parse(
      child() + module("Tables",
                       R"mlir(^bb0(%lanes: !lanes, %bias: !b8, %unused: !b8):
    %mapped = "ac.table.map"(%lanes, %bias) ({
    ^bb0(%ordinal: !b2, %element: !b8, %capture: !b8):
      %value = "ac.bits.binary"(%element, %capture) {opcode = "xor"} : (!b8, !b8) -> !b8
      "ac.yield"(%value) : (!b8) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 1>} : (!lanes, !b8) -> !lanes
    %children = "ac.collection"(%mapped) {instance_name = "lanes", callee = @unit.Child, parameters = [], type_arguments = [], shape = [#w3], occurrence = {site = {definition = @unit.Tables, ast_path = []}, expansion = []}} : (!lanes) -> !lanes
    %reduced = "ac.table.fold"(%children) {kind = "xor"} : (!lanes) -> !b8
    "ac.yield"(%children, %reduced) : (!lanes, !b8) -> ()
  )mlir",
                       "(!lanes, !b8, !b8) -> (!lanes, !b8)",
                       "[\"lanes\", \"bias\", \"unused\"]",
                       "[\"lanes_out\", \"folded\"]"));
  ASSERT_TRUE(unit);
  auto original =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*unit, "unit.Tables"));
  auto signature = original.getFunctionTypeAttr();
  auto originBefore = original->getAttr("ac.origin");
  auto sourceBefore = print(*unit);
  auto interface = cast<mlir::ModuleOp>((*unit)->clone());
  OwningOpRef<mlir::ModuleOp> extracted(interface);
  ASSERT_TRUE(succeeded(extract(interface))) << diagnostics;
  dependencies(lookup(interface, "unit.Tables"), {{0, 1}, {0, 1}});
  EXPECT_EQ(lookup(interface, "unit.Tables").getFunctionTypeAttr(), signature);
  EXPECT_EQ(lookup(interface, "unit.Tables")->getAttr("ac.origin"),
            originBefore);
  EXPECT_EQ(print(*unit), sourceBefore);
  auto collections = original.getBody().front().getOps<ac::CollectionOp>();
  ASSERT_TRUE(llvm::hasSingleElement(collections));
  EXPECT_EQ((*collections.begin()).getShape().size(), 1u);
}

TEST_F(SourceInterfacePassTest,
       SymbolicTableShapeDefaultAndTypeFormalCopyExactly) {
  auto unit = parse(module(
      "Generic",
      R"mlir(^bb0(%x: !ac.table<[#n], !ac.type_param<@unit.Generic, "T">>):
    "ac.yield"(%x) : (!ac.table<[#n], !ac.type_param<@unit.Generic, "T">>) -> ()
  )mlir",
      "(!ac.table<[#n], !ac.type_param<@unit.Generic, \"T\">>) -> "
      "!ac.table<[#n], !ac.type_param<@unit.Generic, \"T\">>",
      "[\"x\"]", "[\"y\"]",
      "[{name = \"N\", type = !ac.math_int, default = #w3}]", "[\"T\"]"));
  ASSERT_TRUE(unit);
  auto definition =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*unit, "unit.Generic"));
  auto parameters = definition.getParameters();
  auto typeParameters = definition.getTypeParameters();
  auto signature = definition.getFunctionTypeAttr();
  auto metadata = (*unit)->getAttrDictionary();
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  auto declaration = lookup(*unit, "unit.Generic");
  ASSERT_TRUE(declaration);
  EXPECT_EQ(declaration.getParameters(), parameters);
  EXPECT_EQ(declaration.getTypeParameters(), typeParameters);
  EXPECT_EQ(declaration.getFunctionTypeAttr(), signature);
  for (NamedAttribute item : metadata)
    if (item.getName() != "ac.unit_kind")
      EXPECT_EQ((*unit)->getAttr(item.getName()), item.getValue());
  dependencies(declaration, {{0}});
}

TEST_F(SourceInterfacePassTest, CollectionOldQFeedbackRemainsATemporalCut) {
  const char *storage = R"mlir(
  "ac.module.import"() {sym_name = "gfsim.dff.Dff", source_owner = {package = "gfsim", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@gfsim.dff.Dff, "T">, !ac.type_param<@gfsim.dff.Dff, "T">) -> !ac.type_param<@gfsim.dff.Dff, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  )mlir";
  auto unit =
      parse(std::string(storage) +
            module("Registers",
                   R"mlir(^bb0(%clk: !controls, %rst: !controls, %init: !lanes):
    %q = "ac.collection"(%clk, %rst, %q, %init) {instance_name = "registers", callee = @gfsim.dff.Dff, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @unit.Registers, ast_path = []}, expansion = []}} : (!controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"(%q) : (!lanes) -> ()
  )mlir",
                   "(!controls, !controls, !lanes) -> !lanes",
                   "[\"clk\", \"rst\", \"init\"]", "[\"q\"]"));
  ASSERT_TRUE(unit);
  auto snapshot =
      SymbolTable::lookupSymbolIn(*unit, "gfsim.dff.Dff")->getAttrDictionary();
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  dependencies(lookup(*unit, "unit.Registers"), {{}});
  EXPECT_EQ(lookup(*unit, "gfsim.dff.Dff")->getAttrDictionary(), snapshot);
}

TEST_F(SourceInterfacePassTest,
       SourceStructureRejectsMalformedIdentityNamespaceAndOwnerClosure) {
  for (unsigned mutation = 0; mutation != 8; ++mutation) {
    SCOPED_TRACE(mutation);
    auto unit = parse(child());
    ASSERT_TRUE(unit);
    Builder b(&context);
    auto definition =
        cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*unit, "unit.Child"));
    if (mutation == 0)
      (*unit)->setAttr("ac.stage", b.getStringAttr("final"));
    if (mutation == 1)
      definition->setAttr("source_owner", owner("foreign.py"));
    if (mutation == 2)
      definition->setAttr("ac.origin", origin("unit.Other"));
    if (mutation == 3)
      definition->removeAttr("ac.origin");
    if (mutation == 4)
      definition->setAttr("ac.declaration_role",
                          b.getStringAttr("import_snapshot"));
    if (mutation == 5)
      (*unit)->setAttr("ac.interfaces",
                       b.getArrayAttr({owner(), owner("z.py"), owner("a.py")}));
    if (mutation == 6 || mutation == 7) {
      auto row = cast<DictionaryAttr>(
          (*unit)->getAttrOfType<ArrayAttr>("ac.exports")[0]);
      NamedAttrList fields(row);
      if (mutation == 6)
        fields.set("target", FlatSymbolRefAttr::get(&context, "unit.Missing"));
      else
        fields.set("site", b.getDictionaryAttr({}));
      (*unit)->setAttr("ac.exports",
                       b.getArrayAttr({fields.getDictionary(&context)}));
    }
    auto before = print(*unit);
    auto result = ac::verifySourceBodyStructure(
        *unit, [&] { return unit->emitError(); });
    EXPECT_TRUE(failed(result));
    EXPECT_TRUE(failed(extract(*unit)));
    EXPECT_EQ(print(*unit), before);
  }
}

TEST_F(SourceInterfacePassTest,
       FailureAfterAnotherDefinitionAnalysisLeavesWholeInputUntouched) {
  auto cycle = module("Cycle", R"mlir(^bb0(%input: !b8):
    %self = "ac.instance"(%self) {instance_name = "self", callee = @unit.Child, parameters = [], type_arguments = [], occurrence = {site = {definition = @unit.Cycle, ast_path = []}, expansion = []}} : (!b8) -> !b8
    "ac.yield"(%self) : (!b8) -> ()
  )mlir",
                      "(!b8) -> !b8", "[\"input\"]", "[\"out\"]");
  auto unit = parse(child() + parent() + cycle);
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(mlir::verify(*unit)));
  auto before = print(*unit);
  auto *childBefore = SymbolTable::lookupSymbolIn(*unit, "unit.Child");
  auto *parentBefore = SymbolTable::lookupSymbolIn(*unit, "unit.Parent");
  EXPECT_TRUE(failed(extract(*unit)));
  EXPECT_EQ(print(*unit), before);
  EXPECT_EQ(SymbolTable::lookupSymbolIn(*unit, "unit.Child"), childBefore);
  EXPECT_EQ(SymbolTable::lookupSymbolIn(*unit, "unit.Parent"), parentBefore);
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceInterfacePassTest, MetadataFailureAndReExtractionAreAtomic) {
  auto unit = parse(child());
  ASSERT_TRUE(unit);
  Builder b(&context);
  (*unit)->setAttr("ac.interfaces", b.getArrayAttr({owner(), owner()}));
  auto before = print(*unit);
  EXPECT_TRUE(failed(extract(*unit)));
  EXPECT_EQ(print(*unit), before);
  (*unit)->setAttr("ac.interfaces", b.getArrayAttr({owner()}));
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  before = print(*unit);
  EXPECT_TRUE(failed(extract(*unit)));
  EXPECT_EQ(print(*unit), before);
}

TEST_F(SourceInterfacePassTest,
       StaleOrdinarySnapshotExtractsButProviderAuthorityStillRejects) {
  auto provider = parse(child());
  ASSERT_TRUE(provider);
  Builder b(&context);
  auto providerOwner = owner("provider.py");
  auto definition =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*provider, "unit.Child"));
  definition.setSymName("provider.Child");
  definition->setAttr("source_owner", providerOwner);
  definition->setAttr("ac.origin", origin("provider.Child"));
  (*provider)->setAttr("ac.source_owner", providerOwner);
  (*provider)->setAttr("ac.interfaces", b.getArrayAttr({providerOwner}));
  (*provider)->setAttr(
      "ac.exports",
      b.getArrayAttr({b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr("Child")),
           b.getNamedAttr("target",
                          FlatSymbolRefAttr::get(&context, "provider.Child")),
           b.getNamedAttr("site", site("provider.py"))})}));
  ASSERT_TRUE(succeeded(extract(*provider))) << diagnostics;
  auto registry = acir::compiler::SourceHeaderRegistry::create(
      &context, {*provider}, [&] { return provider->emitError(); });
  ASSERT_TRUE(succeeded(registry));
  const char *snapshot = R"mlir(
  "ac.module.import"() {sym_name = "provider.Child", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> !b8, input_names = ["x"], output_names = ["y"], dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  )mlir";
  auto consumerBody = module("Consumer", R"mlir(^bb0(%x: !b8):
    %y = "ac.instance"(%x) {instance_name = "child", callee = @provider.Child, parameters = [], type_arguments = [], occurrence = {site = {definition = @unit.Consumer, ast_path = []}, expansion = []}} : (!b8) -> !b8
    "ac.yield"(%y) : (!b8) -> ()
  )mlir",
                             "(!b8) -> !b8", "[\"x\"]", "[\"y\"]");
  auto consumer = parse(std::string(snapshot) + consumerBody);
  ASSERT_TRUE(consumer);
  auto attrs = lookup(*consumer, "provider.Child")->getAttrDictionary();
  ASSERT_TRUE(succeeded(extract(*consumer))) << diagnostics;
  EXPECT_EQ(lookup(*consumer, "provider.Child")->getAttrDictionary(), attrs);
  dependencies(lookup(*consumer, "unit.Consumer"), {{}});
  EXPECT_TRUE(failed(
      registry->verifyModuleImport(lookup(*consumer, "provider.Child"),
                                   [&] { return consumer->emitError(); })));
}

TEST_F(SourceInterfacePassTest,
       PrimitiveLookingSnapshotNeverEstablishesCatalogTrust) {
  const char *snapshot = R"mlir(
  "ac.module.import"() {sym_name = "spoof.dff.Dff", source_owner = {package = "spoof", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@spoof.dff.Dff, "T">, !ac.type_param<@spoof.dff.Dff, "T">) -> !ac.type_param<@spoof.dff.Dff, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  )mlir";
  auto unit = parse(std::string(snapshot) + child());
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  auto registry = acir::compiler::SourceHeaderRegistry::create(
      &context, {}, [&] { return unit->emitError(); });
  ASSERT_TRUE(succeeded(registry));
  auto fake = lookup(*unit, "spoof.dff.Dff");
  ASSERT_TRUE(fake);
  EXPECT_FALSE(registry->isTrustedBuiltin(fake));
  EXPECT_TRUE(failed(
      registry->verifyModuleImport(fake, [&] { return unit->emitError(); })));
  auto supplied = acir::compiler::SourceHeaderRegistry::create(
      &context, {*unit}, [&] { return unit->emitError(); });
  EXPECT_TRUE(failed(supplied));
}

TEST_F(SourceInterfacePassTest,
       ForgedOwningSummaryIsRejectedByBothBodyInterfaceVerifiers) {
  auto body = parse(child());
  ASSERT_TRUE(body);
  OwningOpRef<mlir::ModuleOp> header(cast<mlir::ModuleOp>((*body)->clone()));
  ASSERT_TRUE(succeeded(extract(*header))) << diagnostics;
  auto error = [&] { return body->emitError(); };
  ASSERT_TRUE(succeeded(
      acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header, error)));
  auto registry =
      acir::compiler::SourceHeaderRegistry::create(&context, {}, error);
  ASSERT_TRUE(succeeded(registry));
  ASSERT_TRUE(succeeded(registry->verifyBodySnapshots(*body, *header, error)));

  // A well-formed empty dependency row falsely claims that the owning
  // identity module's output is input-independent. Neither pair verifier
  // may trust an interface's structurally valid summary over actual SSA.
  Builder b(&context);
  auto declaration = lookup(*header, "unit.Child");
  auto rows = declaration.getDependencySummary();
  ASSERT_TRUE(llvm::hasSingleElement(rows));
  NamedAttrList row(cast<DictionaryAttr>(rows[0]));
  row.set("inputs", b.getArrayAttr({}));
  declaration->setAttr("dependency_summary",
                       b.getArrayAttr({row.getDictionary(&context)}));
  ASSERT_TRUE(succeeded(mlir::verify(*header)));
  ASSERT_TRUE(succeeded(ac::verifySourceInterfaceStructure(*header, error)));
  EXPECT_TRUE(failed(
      acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header, error)));
  EXPECT_TRUE(failed(registry->verifyBodySnapshots(*body, *header, error)));
}

TEST_F(SourceInterfacePassTest,
       StagedInterfaceVerificationFailureRetainsExactOriginalIRAndIdentity) {
  auto unit = parse(child() + parent());
  ASSERT_TRUE(unit);
  auto *definition = SymbolTable::lookupSymbolIn(*unit, "unit.Child");
  Builder b(&context);
  // Current native ModuleOp admits an arbitrary binding annotation, while
  // ModuleImportOp forbids target bindings. All source preflight/analysis is
  // valid, so failure occurs only when the detached interface is verified.
  definition->setAttr("cpp_binding", b.getStringAttr("foreign_binding"));
  ASSERT_TRUE(succeeded(mlir::verify(*unit)));
  ASSERT_TRUE(succeeded(ac::verifySourceBodyStructure(
      *unit, [&] { return unit->emitError(); })));
  auto *rootOperation = unit->getOperation();
  auto *originalBlock = unit->getBody();
  auto attributes = (*unit)->getAttrDictionary();
  auto before = print(*unit);
  EXPECT_TRUE(failed(extract(*unit)));
  EXPECT_EQ(unit->getOperation(), rootOperation);
  EXPECT_EQ(unit->getBody(), originalBlock);
  EXPECT_EQ((*unit)->getAttrDictionary(), attributes);
  EXPECT_EQ(SymbolTable::lookupSymbolIn(*unit, "unit.Child"), definition);
  EXPECT_EQ(print(*unit), before);
  EXPECT_NE(diagnostics.find("binding"), std::string::npos) << diagnostics;
}

TEST_F(SourceInterfacePassTest,
       DeclaredStructOutputsRetainDistinctInputFieldDependencies) {
  const char *pair = R"mlir(
  ac.struct "types.Pair" fields [{name = "first", type = !b8}, {name = "second", type = !b8}] {ac.source_owner = {package = "", path = "types.py"}}
  )mlir";
  auto unit = parse(
      std::string(pair) +
      module(
          "Fields",
          R"mlir(^bb0(%a: !ac.struct<"types.Pair">, %b: !ac.struct<"types.Pair">):
    %a_second = ac.struct.get %a["second"] : (!ac.struct<"types.Pair">) -> !b8
    %b_first = ac.struct.get %b["first"] : (!ac.struct<"types.Pair">) -> !b8
    %a_first = ac.struct.get %a["first"] : (!ac.struct<"types.Pair">) -> !b8
    %joined = ac.struct.create(%a_second, %b_first) : (!b8, !b8) -> !ac.struct<"types.Pair">
    "ac.yield"(%joined, %a_first) : (!ac.struct<"types.Pair">, !b8) -> ()
  )mlir",
          "(!ac.struct<\"types.Pair\">, !ac.struct<\"types.Pair\">) -> "
          "(!ac.struct<\"types.Pair\">, !b8)",
          "[\"a\", \"b\"]", "[\"joined\", \"extra\"]"));
  ASSERT_TRUE(unit);
  ASSERT_TRUE(succeeded(extract(*unit))) << diagnostics;
  auto rows = lookup(*unit, "unit.Fields").getDependencySummary();
  // joined.first <- a.second; joined.second <- b.first; extra <- a.first.
  struct Expected {
    unsigned output, input;
    StringRef outputField, inputField;
  };
  const Expected expected[] = {{0, 0, "first", "second"},
                               {0, 1, "second", "first"},
                               {1, 0, "", "first"}};
  ASSERT_EQ(rows.size(), std::size(expected));
  Builder b(&context);
  for (auto [index, value] : llvm::enumerate(expected)) {
    auto row = cast<DictionaryAttr>(rows[index]);
    auto output = row.getAs<DictionaryAttr>("output");
    EXPECT_EQ(output.getAs<IntegerAttr>("port").getInt(), value.output);
    EXPECT_EQ(output.getAs<ArrayAttr>("path"),
              value.outputField.empty()
                  ? b.getArrayAttr({})
                  : b.getArrayAttr({b.getStringAttr(value.outputField)}));
    auto inputs = row.getAs<ArrayAttr>("inputs");
    ASSERT_EQ(inputs.size(), 1u);
    auto input = cast<DictionaryAttr>(inputs[0]);
    EXPECT_EQ(input.getAs<IntegerAttr>("port").getInt(), value.input);
    EXPECT_EQ(input.getAs<ArrayAttr>("path"),
              b.getArrayAttr({b.getStringAttr(value.inputField)}));
  }
}

std::string localPackedStruct() {
  return R"mlir(
  ac.struct "unit.Parcel" fields [{name = "tag", type = !b2}, {name = "payload", type = !b8}] {ac.source_owner = {package = "", path = "unit.py"}}
  )mlir";
}
std::string localPackedModule() {
  return module("Packed", R"mlir(^bb0(%value: !ac.struct<"unit.Parcel">):
    %tag = ac.struct.get %value["tag"] : (!ac.struct<"unit.Parcel">) -> !b2
    %payload = ac.struct.get %value["payload"] : (!ac.struct<"unit.Parcel">) -> !b8
    %copy = ac.struct.create(%tag, %payload) : (!b2, !b8) -> !ac.struct<"unit.Parcel">
    "ac.yield"(%copy) : (!ac.struct<"unit.Parcel">) -> ()
  )mlir", "(!ac.struct<\"unit.Parcel\">) -> !ac.struct<\"unit.Parcel\">",
                "[\"value\"]", "[\"copy\"]");
}

TEST_F(SourceInterfacePassTest,
       CurrentPackedStructSchemaExtractsAndPreservesRegistryAuthority) {
  auto body = parse(localPackedStruct() + localPackedModule());
  ASSERT_TRUE(body);
  auto original = cast<ac::StructOp>(SymbolTable::lookupSymbolIn(*body, "unit.Parcel"));
  auto originalAttrs = original->getAttrDictionary();
  OwningOpRef<mlir::ModuleOp> header(cast<mlir::ModuleOp>((*body)->clone()));
  ASSERT_TRUE(succeeded(extract(*header))) << diagnostics;
  auto declaration = cast<ac::StructOp>(SymbolTable::lookupSymbolIn(*header, "unit.Parcel"));
  EXPECT_EQ(declaration->getAttrDictionary(), originalAttrs);
  for (Attribute field : declaration.getFields()) {
    auto row = cast<DictionaryAttr>(field);
    EXPECT_EQ(row.size(), 2u);
    EXPECT_TRUE(row.getAs<StringAttr>("name"));
    EXPECT_TRUE(row.getAs<TypeAttr>("type"));
  }
  auto error = [&] { return body->emitError(); };
  auto registry = acir::compiler::SourceHeaderRegistry::create(&context, {*header}, error);
  ASSERT_TRUE(succeeded(registry));
  EXPECT_EQ(registry->lookupRecord(FlatSymbolRefAttr::get(&context, "unit.Parcel")), declaration);
  EXPECT_TRUE(succeeded(acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header, error)));
  EXPECT_TRUE(succeeded(registry->verifyBodySnapshots(*body, *header, error)));
}

TEST_F(SourceInterfacePassTest,
       PackedStructFieldTypeTamperCannotPassOwningBodySnapshots) {
  auto body = parse(localPackedStruct() + localPackedModule());
  ASSERT_TRUE(body);
  OwningOpRef<mlir::ModuleOp> header(cast<mlir::ModuleOp>((*body)->clone()));
  ASSERT_TRUE(succeeded(extract(*header))) << diagnostics;
  auto record = cast<ac::StructOp>(SymbolTable::lookupSymbolIn(*header, "unit.Parcel"));
  Builder b(&context);
  SmallVector<Attribute> fields(record.getFields().begin(), record.getFields().end());
  NamedAttrList changed(cast<DictionaryAttr>(fields[0]));
  changed.set("type", cast<DictionaryAttr>(fields[1]).getAs<TypeAttr>("type"));
  fields[0] = changed.getDictionary(&context);
  record->setAttr("fields", b.getArrayAttr(fields));
  ASSERT_TRUE(succeeded(mlir::verify(*header)));
  auto error = [&] { return body->emitError(); };
  auto registry = acir::compiler::SourceHeaderRegistry::create(&context, {*header}, error);
  ASSERT_TRUE(succeeded(registry));
  EXPECT_TRUE(failed(acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header, error)));
  EXPECT_TRUE(failed(registry->verifyBodySnapshots(*body, *header, error)));
}

TEST_F(SourceInterfacePassTest,
       PackedStructForeignOwnerCannotReplaceAnOwningDefinition) {
  auto body = parse(localPackedStruct() + localPackedModule());
  ASSERT_TRUE(body);
  OwningOpRef<mlir::ModuleOp> header(cast<mlir::ModuleOp>((*body)->clone()));
  ASSERT_TRUE(succeeded(extract(*header))) << diagnostics;
  auto error = [&] { return body->emitError(); };
  auto registry = acir::compiler::SourceHeaderRegistry::create(&context, {*header}, error);
  ASSERT_TRUE(succeeded(registry));
  auto record = cast<ac::StructOp>(SymbolTable::lookupSymbolIn(*body, "unit.Parcel"));
  record->setAttr("ac.source_owner", owner("foreign.py"));
  EXPECT_TRUE(failed(ac::verifySourceBodyStructure(*body, error)));
  EXPECT_TRUE(failed(registry->verifyBodySnapshots(*body, *header, error)));
  EXPECT_TRUE(failed(extract(*body)));
}

TEST_F(SourceInterfacePassTest,
       PackedStructNominalIdentityCannotSubstituteAnExtractedDeclaration) {
  auto body = parse(localPackedStruct() + localPackedModule());
  ASSERT_TRUE(body);
  OwningOpRef<mlir::ModuleOp> header(cast<mlir::ModuleOp>((*body)->clone()));
  ASSERT_TRUE(succeeded(extract(*header))) << diagnostics;
  auto error = [&] { return body->emitError(); };
  auto registry = acir::compiler::SourceHeaderRegistry::create(&context, {*header}, error);
  ASSERT_TRUE(succeeded(registry));
  auto record = cast<ac::StructOp>(SymbolTable::lookupSymbolIn(*body, "unit.Parcel"));
  record.setSymName("unit.DifferentParcel");
  record->setAttr("ac.origin", origin("unit.DifferentParcel"));
  EXPECT_TRUE(failed(registry->verifyBodySnapshots(*body, *header, error)));
  EXPECT_TRUE(failed(acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header, error)));
}
} // namespace
