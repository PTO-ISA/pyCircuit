#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/STLExtras.h"
#include "gtest/gtest.h"
#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;
const char *preamble = R"mlir(
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
)mlir";
const char *native = R"mlir(
  "ac.module"() ({
  ^bb0(%input: !b1):
    "ac.yield"(%input) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["input"], output_names = ["output"]} : () -> ()
  "ac.enum"() {sym_name = "shared.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "shared.Record" fields [{name = "value", type = !b1}]
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
)mlir";
class FinalSourceUnitsTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  OwningOpRef<mlir::ModuleOp> parse() {
    return parseSourceString<mlir::ModuleOp>(
        std::string(preamble) + "module {" + native + "}", &context);
  }
  DictionaryAttr owner(StringRef path, StringRef package = "") {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr(package)),
                                b.getNamedAttr("path", b.getStringAttr(path))});
  }
  auto collect(mlir::ModuleOp package) {
    return ac::collectFinalSourceUnits(package, [&] { return package.emitError(); });
  }
  template <typename Mutate> void rejects(Mutate mutate, StringRef guard) {
    auto unit = parse(); ASSERT_TRUE(unit);
    Builder b(&context); mutate(*unit, b);
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics); d.print(out); return success();
    });
    EXPECT_TRUE(failed(collect(*unit)));
    EXPECT_NE(diagnostics.find(guard.str()), std::string::npos) << diagnostics;
    diagnostics.clear();
  }
};
TEST_F(FinalSourceUnitsTest, NativeAbsenceRetainsOnlyActualOwners) {
  auto unit = parse(); ASSERT_TRUE(unit);
  auto rows = collect(*unit); ASSERT_TRUE(succeeded(rows)); ASSERT_EQ(rows->size(), 1u);
  EXPECT_EQ(rows->front().owner, owner("top.py"));
  ASSERT_EQ(rows->front().declarations.size(), 1u);
  auto top = cast<ac::ModuleOp>(rows->front().declarations.front());
  EXPECT_EQ(top.getSymName(), "top");
  EXPECT_FALSE(top->hasAttr("ac.origin")); EXPECT_FALSE(top->hasAttr("ac.declaration_role"));
}
TEST_F(FinalSourceUnitsTest, ExplicitRowsPreserveEmptyFacadesAndOwnedNominals) {
  auto unit = parse(); ASSERT_TRUE(unit); Builder b(&context);
  auto enumeration = *unit->getOps<ac::EnumOp>().begin();
  auto record = *unit->getOps<ac::StructOp>().begin();
  enumeration->setAttr("ac.source_owner", owner("types.py"));
  record->setAttr("ac.source_owner", owner("types.py"));
  unit->getOperation()->setAttr("ac.source_units", b.getArrayAttr({
      owner("facade.py"), owner("top.py"), owner("types.py")}));
  auto rows = collect(*unit); ASSERT_TRUE(succeeded(rows)); ASSERT_EQ(rows->size(), 3u);
  EXPECT_EQ((*rows)[0].owner, owner("facade.py")); EXPECT_TRUE((*rows)[0].declarations.empty());
  EXPECT_EQ((*rows)[1].owner, owner("top.py")); EXPECT_EQ((*rows)[1].declarations.size(), 1u);
  EXPECT_EQ((*rows)[2].owner, owner("types.py")); ASSERT_EQ((*rows)[2].declarations.size(), 2u);
  EXPECT_TRUE(llvm::is_contained((*rows)[2].declarations, enumeration.getOperation()));
  EXPECT_TRUE(llvm::is_contained((*rows)[2].declarations, record.getOperation()));
  std::string saved; llvm::raw_string_ostream stream(saved); unit->print(stream); stream.flush();
  auto reloaded = parseSourceString<mlir::ModuleOp>(saved, &context); ASSERT_TRUE(reloaded);
  auto restored = collect(*reloaded); ASSERT_TRUE(succeeded(restored)); ASSERT_EQ(restored->size(), 3u);
  EXPECT_EQ((*restored)[0].owner, owner("facade.py")); EXPECT_TRUE((*restored)[0].declarations.empty());
}
TEST_F(FinalSourceUnitsTest, OrdersUtf8PackageThenPathWithoutInferringNames) {
  auto unit = parse(); ASSERT_TRUE(unit); Builder b(&context);
  unit->getOperation()->setAttr("ac.source_units", b.getArrayAttr({
      owner("top.py"), owner("a.py", "z"), owner("a.py", "é")}));
  auto rows = collect(*unit); ASSERT_TRUE(succeeded(rows)); ASSERT_EQ(rows->size(), 3u);
  EXPECT_EQ((*rows)[0].owner, owner("top.py"));
  EXPECT_EQ((*rows)[1].owner, owner("a.py", "z")); EXPECT_TRUE((*rows)[1].declarations.empty());
  EXPECT_EQ((*rows)[2].owner, owner("a.py", "é")); EXPECT_TRUE((*rows)[2].declarations.empty());
}
TEST_F(FinalSourceUnitsTest, WrongTypedPresenceDoesNotFallBackToAbsence) {
  rejects([](mlir::ModuleOp unit, Builder &b) { unit->setAttr("ac.source_units", b.getStringAttr("absent")); }, "source_units");
  rejects([](mlir::ModuleOp unit, Builder &b) { unit->setAttr("ac.source_units", b.getArrayAttr({b.getStringAttr("not-owner")})); }, "SourceOwner");
  for (bool enumeration : {false, true})
    rejects([&](mlir::ModuleOp unit, Builder &b) {
      Operation *op = enumeration ? (*unit.getOps<ac::EnumOp>().begin()).getOperation()
                                 : (*unit.getOps<ac::StructOp>().begin()).getOperation();
      op->setAttr("ac.source_owner", b.getStringAttr("not-owner"));
    }, "source_owner");
}
TEST_F(FinalSourceUnitsTest, ExactOwnerShapeOrderingAndSubsetFailClosed) {
  rejects([&](mlir::ModuleOp unit, Builder &b) { unit->setAttr("ac.source_units", b.getArrayAttr({owner("top.py"), owner("top.py")})); }, "source_units");
  rejects([&](mlir::ModuleOp unit, Builder &b) { unit->setAttr("ac.source_units", b.getArrayAttr({owner("types.py"), owner("top.py")})); }, "source_units");
  rejects([&](mlir::ModuleOp unit, Builder &b) { unit->setAttr("ac.source_units", b.getArrayAttr({owner("facade.py")})); }, "source_units");
  rejects([&](mlir::ModuleOp unit, Builder &b) {
    unit->setAttr("ac.source_units", b.getArrayAttr({b.getDictionaryAttr({b.getNamedAttr("path", b.getStringAttr("top.py"))})}));
  }, "SourceOwner");
  rejects([&](mlir::ModuleOp unit, Builder &b) {
    (*unit.getOps<ac::EnumOp>().begin())->setAttr("ac.source_owner", b.getDictionaryAttr({b.getNamedAttr("path", b.getStringAttr("types.py"))}));
  }, "SourceOwner");
}
} // namespace
