#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceHelperAnalysis.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <string>
#include <vector>

namespace acir::ac {
namespace {
const char *helpers = R"ir(
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @helpers.identity, ast_path = []}, expansion = []}, location = {path = "helpers.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @helpers.identity, ast_path = []}, expansion = []}, location = {path = "helpers.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
#owner = {package = "", path = "helpers.py"}
#span = {path = "helpers.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#identity = {site = {definition = @helpers.identity, ast_path = []}, expansion = []}
#chain = {site = {definition = @helpers.chain, ast_path = []}, expansion = []}
#none = {site = {definition = @helpers.none, ast_path = []}, expansion = []}
#offset = {site = {definition = @helpers.Owner.offset, ast_path = []}, expansion = []}
#constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  func.func @helpers.identity(%value: !b8) -> !b8 attributes {ac.helper_kind = "value", ac.source_owner = #owner, ac.origin = #identity, ac.declaration_role = "definition", ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #constraint, default = {present = false}, origin = #identity, location = #span}], ac.result_constraints = [#constraint], ac.return_form = "single", ac.capture_types = []} {
    func.return %value : !b8
  }
  func.func @helpers.chain(%value: !b8) -> !b8 attributes {ac.helper_kind = "value", ac.source_owner = #owner, ac.origin = #chain, ac.declaration_role = "definition", ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #constraint, default = {present = false}, origin = #chain, location = #span}], ac.result_constraints = [#constraint], ac.return_form = "single", ac.capture_types = []} {
    %called = func.call @helpers.identity(%value) : (!b8) -> !b8
    %sum = "ac.bits.binary"(%called, %value) {opcode = "add"} : (!b8, !b8) -> !b8
    func.return %sum : !b8
  }
  func.func @helpers.none() attributes {ac.helper_kind = "value", ac.source_owner = #owner, ac.origin = #none, ac.declaration_role = "definition", ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", ac.capture_types = []} {
    func.return
  }
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "helpers.Owner", source_owner = #owner, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
  func.func private @helpers.Owner.offset(%value: !b8, %snapshot: !b8) -> !b8 attributes {ac.helper_kind = "value", ac.source_owner = #owner, ac.origin = #offset, ac.declaration_role = "definition", ac.lexical_owner = @helpers.Owner, ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #constraint, default = {present = false}, origin = #offset, location = #span}], ac.result_constraints = [#constraint], ac.return_form = "single", ac.capture_types = [!b8]} {
    %sum = "ac.bits.binary"(%value, %snapshot) {opcode = "add"} : (!b8, !b8) -> !b8
    func.return %sum : !b8
  }
}
)ir";

class SourceHelperAuthorityTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OpBuilder builder{&context};
  std::string diagnostics;
  void SetUp() override {
    context.loadDialect<ACIRDialect, mlir::func::FuncDialect>();
  }
  mlir::OwningOpRef<mlir::ModuleOp> parse() {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(helpers, &context);
    EXPECT_TRUE(file);
    return file;
  }
  mlir::func::FuncOp function(mlir::ModuleOp file,
                              llvm::StringRef name = "helpers.identity") {
    return mlir::cast<mlir::func::FuncOp>(
        mlir::SymbolTable::lookupSymbolIn(file, name));
  }
  template <typename Function> auto capture(Function function) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &error) {
          llvm::raw_string_ostream stream(diagnostics);
          error.print(stream);
          return mlir::success();
        });
    return function();
  }
  mlir::LogicalResult verify(mlir::ModuleOp file) {
    llvm::SmallVector<mlir::func::FuncOp> declarations(
        file.getOps<mlir::func::FuncOp>());
    return capture([&] {
      return verifySourceValueHelperClosure(
          declarations, [&](mlir::FlatSymbolRefAttr symbol) {
            return mlir::dyn_cast_or_null<mlir::func::FuncOp>(
                mlir::SymbolTable::lookupSymbolIn(file, symbol.getValue()));
          });
    });
  }
  mlir::DictionaryAttr changed(mlir::DictionaryAttr value,
                               llvm::StringRef field,
                               mlir::Attribute replacement) {
    mlir::NamedAttrList fields(value);
    if (replacement)
      fields.set(field, replacement);
    else
      fields.erase(field);
    return fields.getDictionary(&context);
  }
  void rejects(mlir::ModuleOp file) {
    EXPECT_TRUE(mlir::failed(verify(file)));
    EXPECT_FALSE(diagnostics.empty());
  }
};

TEST_F(SourceHelperAuthorityTest,
       OrdinaryResultAndExplicitCaptureABIIsVerified) {
  auto file = parse();
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(verify(*file))) << diagnostics;
  auto identity = function(*file);
  EXPECT_EQ(identity.getFunctionType().getNumInputs(), 1u);
  EXPECT_EQ(identity.getFunctionType().getNumResults(), 1u);
  EXPECT_FALSE(identity->hasAttr("ac.check_templates"));
  auto none = function(*file, "helpers.none");
  EXPECT_EQ(none.getFunctionType().getNumResults(), 0u);
  auto captured = function(*file, "helpers.Owner.offset");
  EXPECT_EQ(captured.getFunctionType().getNumInputs(), 2u);
  EXPECT_EQ(captured->getAttrOfType<mlir::ArrayAttr>("ac.parameters").size(),
            1u);
  EXPECT_EQ(captured->getAttrOfType<mlir::ArrayAttr>("ac.capture_types").size(),
            1u);
}

TEST_F(SourceHelperAuthorityTest,
       MetadataHasMandatoryTypedFieldsAndNoRetiredABI) {
  for (auto field :
       {"ac.helper_kind", "ac.source_owner", "ac.origin", "ac.declaration_role",
        "ac.parameters", "ac.result_constraints", "ac.return_form",
        "ac.capture_types"}) {
    for (bool missing : {false, true}) {
      auto file = parse();
      ASSERT_TRUE(file);
      auto helper = function(*file);
      if (missing)
        helper->removeAttr(field);
      else
        helper->setAttr(field, builder.getUnitAttr());
      rejects(*file);
    }
  }
  for (auto field : {"ac.check_templates"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    function(*file)->setAttr(field, builder.getArrayAttr({}));
    rejects(*file);
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto helper = function(*file);
  auto payload = helper.getFunctionType().getInput(0);
  helper.setType(builder.getFunctionType({builder.getI1Type(), payload},
                                         {payload, builder.getI1Type()}));
  rejects(*file);
}

TEST_F(SourceHelperAuthorityTest,
       ParameterContractsAreClosedAndUseHardwareAuthority) {
  for (auto field :
       {"name", "binding", "constraint", "default", "origin", "location"}) {
    for (bool missing : {false, true}) {
      auto file = parse();
      ASSERT_TRUE(file);
      auto helper = function(*file);
      auto row = mlir::cast<mlir::DictionaryAttr>(
          helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
      helper->setAttr(
          "ac.parameters",
          builder.getArrayAttr(
              {changed(row, field,
                       missing ? mlir::Attribute{} : builder.getUnitAttr())}));
      rejects(*file);
    }
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto helper = function(*file);
  auto row = mlir::cast<mlir::DictionaryAttr>(
      helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
  auto constraint = row.getAs<mlir::DictionaryAttr>("constraint");
  helper->setAttr(
      "ac.parameters",
      builder.getArrayAttr({changed(
          row, "constraint",
          changed(constraint, "kind", builder.getStringAttr("logical")))}));
  rejects(*file);
}

TEST_F(SourceHelperAuthorityTest, HelperCallsRequireExactPhysicalSignature) {
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto chain = function(*file, "helpers.chain");
    auto call = *chain.getBody().front().getOps<mlir::func::CallOp>().begin();
    if (mutation == 0)
      call->setOperands({});
    if (mutation == 1)
      call->getResult(0).setType(builder.getI1Type());
    if (mutation == 2)
      call->setAttr("callee",
                    mlir::FlatSymbolRefAttr::get(&context, "helpers.missing"));
    rejects(*file);
  }
}

TEST_F(SourceHelperAuthorityTest,
       CapturesAreExplicitAndRestrictedToTheirLexicalOwner) {
  for (unsigned mutation = 0; mutation != 6; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto captured = function(*file, "helpers.Owner.offset");
    if (mutation == 0)
      captured->setAttr("ac.capture_types", builder.getArrayAttr({}));
    if (mutation == 1)
      captured->setAttr(
          "ac.capture_types",
          builder.getArrayAttr({mlir::TypeAttr::get(builder.getI1Type())}));
    if (mutation == 2)
      captured->removeAttr("ac.lexical_owner");
    if (mutation == 3)
      captured->setAttr("ac.lexical_owner", mlir::FlatSymbolRefAttr::get(
                                                &context, "helpers.Missing"));
    if (mutation == 4)
      captured->setAttr("sym_visibility", builder.getStringAttr("public"));
    if (mutation == 5)
      captured->setAttr(
          "ac.source_owner",
          builder.getDictionaryAttr(
              {builder.getNamedAttr("package", builder.getStringAttr("")),
               builder.getNamedAttr("path",
                                    builder.getStringAttr("foreign.py"))}));
    rejects(*file);
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto chain = function(*file, "helpers.chain");
  auto call = *chain.getBody().front().getOps<mlir::func::CallOp>().begin();
  call->setAttr("callee",
                mlir::FlatSymbolRefAttr::get(&context, "helpers.Owner.offset"));
  call->setOperands({chain.getBody().front().getArgument(0),
                     chain.getBody().front().getArgument(0)});
  rejects(*file);
}

TEST_F(SourceHelperAuthorityTest,
       UnsupportedEffectsAndAllocationsNeverBecomePureByDCE) {
  for (auto name : {"ac.instance", "ac.state", "ac.state.read",
                    "ac.legacy.method.call", "ac.transaction", "ac.comb", "ac.observe",
                    "ac.expect", "test.unknown"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    context.allowUnregisteredDialects();
    auto helper = function(*file);
    builder.setInsertionPoint(helper.getBody().front().getTerminator());
    mlir::OperationState effect(helper.getLoc(), name);
    builder.create(effect);
    rejects(*file);
  }
}

TEST_F(SourceHelperAuthorityTest, ReturnsAndRegionsUseTheOrdinaryPhysicalABI) {
  for (unsigned mutation = 0; mutation != 4; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto helper = function(*file);
    if (mutation == 0)
      helper.getBody().front().getTerminator()->setOperands({});
    if (mutation == 1)
      helper.getBody().push_back(new mlir::Block);
    if (mutation == 2)
      helper.getBody().getBlocks().clear();
    if (mutation == 3) {
      auto none = function(*file, "helpers.none");
      builder.setInsertionPoint(none.getBody().front().getTerminator());
      auto payload = helper.getFunctionType().getInput(0);
      mlir::OperationState constant(none.getLoc(),
                                    BitsConstantOp::getOperationName());
      auto bits = mlir::cast<BitsType>(payload);
      constant.addTypes(payload);
      constant.addAttribute("value", bits.getWidth());
      auto *value = builder.create(constant);
      none.getBody().front().getTerminator()->setOperands(value->getResults());
    }
    rejects(*file);
  }
}

TEST_F(SourceHelperAuthorityTest, UnusedRecursiveCallsStillInvalidateClosure) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto chain = function(*file, "helpers.chain");
  auto call = *chain.getBody().front().getOps<mlir::func::CallOp>().begin();
  call->setAttr("callee",
                mlir::FlatSymbolRefAttr::get(&context, "helpers.chain"));
  chain.getBody().front().getTerminator()->setOperands(
      chain.getBody().front().getArgument(0));
  EXPECT_TRUE(mlir::failed(verify(*file)));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recurs"),
            std::string::npos)
      << diagnostics;
}

TEST_F(SourceHelperAuthorityTest,
       StaticParameterReferencesFailSafelyWithoutAHelperInventory) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto helper = function(*file);
  auto old = mlir::cast<mlir::DictionaryAttr>(
      helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
  auto constraint = old.getAs<mlir::DictionaryAttr>("constraint");
  auto reference = mlir::parseAttribute(
      R"attr(#ac.static_expr<{kind = "reference", ref = {kind = "parameter", owner = @helpers.identity, name = "WIDTH"}, origin = {site = {definition = @helpers.identity, ast_path = []}, expansion = []}, location = {path = "helpers.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>)attr",
      &context);
  ASSERT_TRUE(reference);
  auto type = BitsType::get(&context, mlir::cast<StaticExprAttr>(reference));
  helper->setAttr(
      "ac.parameters",
      builder.getArrayAttr(
          {changed(old, "constraint",
                   changed(constraint, "type", mlir::TypeAttr::get(type)))}));
  rejects(*file);
}

TEST_F(SourceHelperAuthorityTest,
       MalformedRootsAndCalleesFailBeforeTypedGetters) {
  for (bool callee : {false, true}) {
    for (unsigned mutation = 0; mutation != 5; ++mutation) {
      auto file = parse();
      ASSERT_TRUE(file);
      auto helper = function(*file);
      auto chain = function(*file, "helpers.chain");
      if (mutation == 0)
        helper->removeAttr("sym_name");
      if (mutation == 1)
        helper->setAttr("sym_name", builder.getUnitAttr());
      if (mutation == 2)
        helper->removeAttr("function_type");
      if (mutation == 3)
        helper->setAttr("function_type",
                        mlir::TypeAttr::get(builder.getI1Type()));
      if (mutation == 4) {
        mlir::OperationState malformed(helper.getLoc(),
                                       mlir::func::FuncOp::getOperationName());
        malformed.addAttributes(helper->getAttrs());
        builder.setInsertionPoint(helper);
        auto replacement =
            mlir::cast<mlir::func::FuncOp>(builder.create(malformed));
        helper->erase();
        helper = replacement;
      }
      auto lookup = [&](mlir::FlatSymbolRefAttr symbol) {
        return symbol.getValue() == "helpers.identity" ? helper
                                                       : mlir::func::FuncOp{};
      };
      auto entry = callee ? chain : helper;
      EXPECT_TRUE(mlir::failed(
          capture([&] { return verifySourceValueHelper(entry, lookup); })));
      EXPECT_FALSE(diagnostics.empty());
      llvm::SmallVector<mlir::func::FuncOp> entries{entry};
      EXPECT_TRUE(mlir::failed(capture(
          [&] { return verifySourceValueHelperClosure(entries, lookup); })));
      EXPECT_FALSE(diagnostics.empty());
    }
  }
  auto lookup = [&](mlir::FlatSymbolRefAttr) { return mlir::func::FuncOp{}; };
  EXPECT_TRUE(mlir::failed(verifySourceValueHelper({}, lookup)));
  llvm::SmallVector<mlir::func::FuncOp> nulls{mlir::func::FuncOp{}};
  EXPECT_TRUE(mlir::failed(verifySourceValueHelperClosure(nulls, lookup)));
}

TEST_F(SourceHelperAuthorityTest,
       LocalSSAStillRequiresDominanceAndExactOperationTypes) {
  for (unsigned mutation = 0; mutation != 2; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto helper = function(*file, "helpers.chain");
    auto call = *helper.getBody().front().getOps<mlir::func::CallOp>().begin();
    auto binary = *helper.getBody().front().getOps<BitsBinaryOp>().begin();
    if (mutation == 0)
      call->setOperand(0, binary.getResult());
    if (mutation == 1)
      binary->getResult(0).setType(builder.getI1Type());
    auto lookup = [&](mlir::FlatSymbolRefAttr symbol) {
      return mlir::dyn_cast_or_null<mlir::func::FuncOp>(
          mlir::SymbolTable::lookupSymbolIn(*file, symbol.getValue()));
    };
    EXPECT_TRUE(mlir::failed(
        capture([&] { return verifySourceValueHelper(helper, lookup); })));
    EXPECT_FALSE(diagnostics.empty());
    rejects(*file);
  }
}

TEST_F(SourceHelperAuthorityTest,
       ClosureDepthIsBoundedInEitherDeclarationOrder) {
  for (unsigned edges : {128u, 129u}) {
    for (bool reversed : {false, true}) {
      auto file = parse();
      ASSERT_TRUE(file);
      auto identity = function(*file);
      llvm::SmallVector<mlir::func::FuncOp> levels;
      for (unsigned i = 0; i <= edges; ++i) {
        auto helper = mlir::cast<mlir::func::FuncOp>(identity->clone());
        auto name = "helpers.level" + std::to_string(i);
        helper->setAttr("sym_name", builder.getStringAttr(name));
        auto origin = helper->getAttrOfType<mlir::DictionaryAttr>("ac.origin");
        auto site = origin.getAs<mlir::DictionaryAttr>("site");
        site = changed(site, "definition",
                       mlir::FlatSymbolRefAttr::get(&context, name));
        origin = changed(origin, "site", site);
        helper->setAttr("ac.origin", origin);
        auto parameter = mlir::cast<mlir::DictionaryAttr>(
            helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
        helper->setAttr(
            "ac.parameters",
            builder.getArrayAttr({changed(parameter, "origin", origin)}));
        file->getBody()->getOperations().insert(file->getBody()->end(),
                                                helper.getOperation());
        levels.push_back(helper);
      }
      for (unsigned i = 0; i < edges; ++i) {
        auto helper = levels[i];
        builder.setInsertionPoint(helper.getBody().front().getTerminator());
        auto call = builder.create<mlir::func::CallOp>(
            helper.getLoc(), levels[i + 1].getSymName(),
            helper.getFunctionType().getResults(),
            helper.getBody().front().getArguments());
        helper.getBody().front().getTerminator()->setOperands(
            call->getResults());
      }
      if (reversed)
        std::reverse(levels.begin(), levels.end());
      auto lookup = [&](mlir::FlatSymbolRefAttr symbol) {
        return mlir::dyn_cast_or_null<mlir::func::FuncOp>(
            mlir::SymbolTable::lookupSymbolIn(*file, symbol.getValue()));
      };
      auto result = capture(
          [&] { return verifySourceValueHelperClosure(levels, lookup); });
      if (edges == 128)
        EXPECT_TRUE(mlir::succeeded(result)) << diagnostics;
      else {
        EXPECT_TRUE(mlir::failed(result));
        auto message = llvm::StringRef(diagnostics).lower();
        EXPECT_TRUE(message.find("budget") != std::string::npos ||
                    message.find("depth") != std::string::npos)
            << diagnostics;
      }
    }
  }
}

TEST_F(SourceHelperAuthorityTest, EarlyReturnsAndForeignSSAOperandsFailClosed) {
  for (bool foreign : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto helper = function(*file);
    if (foreign) {
      auto other = function(*file, "helpers.chain");
      helper.getBody().front().getTerminator()->setOperand(
          0, other.getBody().front().getArgument(0));
    } else {
      builder.setInsertionPointToStart(&helper.getBody().front());
      builder.create<mlir::func::ReturnOp>(
          helper.getLoc(), helper.getBody().front().getArgument(0));
    }
    rejects(*file);
  }
}

} // namespace
} // namespace acir::ac
