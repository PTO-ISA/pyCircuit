#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

// Budget admission must precede definition-local association validation, even
// for an unreachable definition. The fixture has 128 small metadata records;
// it never constructs a large lane family or data payload.
class HardwareSourceCheckValidationBudgetTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::RuleOp checkedRule;
  ac::SourceExpectOp finalExpect;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "budget.py", 1, 1); }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr occurrence(StringRef owner, unsigned index) {
    auto step = d({f("kind", b.getStringAttr("index")),
                   f("value", b.getI64IntegerAttr(index))});
    return d(
        {f("site", d({f("definition", FlatSymbolRefAttr::get(&context, owner)),
                      f("ast_path", b.getArrayAttr({step}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("budget.py")),
              f("line", b.getI64IntegerAttr(1)),
              f("column", b.getI64IntegerAttr(1)),
              f("end_line", b.getI64IntegerAttr(1)),
              f("end_column", b.getI64IntegerAttr(2))});
  }
  Type bit() {
    return ac::BitsType::get(
        &context,
        cast<ac::StaticExprAttr>(parseAttribute(
            "#ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<1>}, origin = {site = "
            "{definition = @Root, ast_path = []}, expansion = []}, "
            "location = {path = \"budget.py\", line = 1 : i64, column "
            "= 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>",
            &context)));
  }
  Operation *op(StringRef name, ValueRange inputs = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  void definition(StringRef name, unsigned checks) {
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr(name)),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("budget.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names",
           b.getArrayAttr({b.getStringAttr("a"), b.getStringAttr("b")})),
         f("output_names", b.getArrayAttr({})),
         f("function_type",
           TypeAttr::get(b.getFunctionType({bit(), bit()}, {})))});
    state.addRegion();
    auto owner = cast<ac::ModuleOp>(b.create(state));
    auto *body = new Block;
    owner.getBody().push_back(body);
    body->addArgument(bit(), loc());
    body->addArgument(bit(), loc());
    b.setInsertionPointToEnd(body);
    if (checks) {
      SmallVector<Attribute> ids, required;
      for (unsigned index = 0; index < checks; ++index) {
        auto id = d({f("registration", occurrence(name, 2)),
                     f("check", occurrence(name, 4)),
                     f("obligation", b.getI64IntegerAttr(index))});
        ids.push_back(id);
        required.push_back(d({f("id", id), f("kind", b.getStringAttr("assert")),
                              f("location", span())}));
      }
      OperationState rule(loc(), ac::RuleOp::getOperationName());
      rule.addOperands(body->getArguments());
      rule.addTypes(SmallVector<Type>(1 + 2 * checks, bit()));
      rule.addAttributes({f("name", b.getStringAttr("inspect")),
                          f("occurrence", occurrence(name, 2)),
                          f("ac.required_checks", b.getArrayAttr(required))});
      rule.addRegion();
      checkedRule = cast<ac::RuleOp>(b.create(rule));
      auto *calculation = new Block;
      checkedRule.getBody().push_back(calculation);
      calculation->addArgument(bit(), loc());
      calculation->addArgument(bit(), loc());
      b.setInsertionPointToEnd(calculation);
      SmallVector<Value> yields{calculation->getArgument(0)};
      for (unsigned index = 0; index < checks; ++index)
        yields.append(
            {calculation->getArgument(0), calculation->getArgument(1)});
      op(ac::YieldOp::getOperationName(), yields);
      b.setInsertionPointToEnd(body);
      for (unsigned index = 0; index < checks; ++index)
        finalExpect = cast<ac::SourceExpectOp>(
            op(ac::SourceExpectOp::getOperationName(),
               {checkedRule.getResult(1 + 2 * index),
                checkedRule.getResult(2 + 2 * index)},
               {},
               {f("kind", b.getStringAttr("assert")), f("location", span()),
                f("ac.check_id", ids[index])}));
    }
    op(ac::YieldOp::getOperationName());
  }
  template <typename Function> auto capture(Function function) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream stream(diagnostics);
      error.print(stream);
      return success();
    });
    return function();
  }
  void budgetPrecedesAssociation(bool reachable) {
    package = mlir::ModuleOp::create(loc());
    definition("Root", reachable ? 128 : 0);
    if (!reachable)
      definition("Unreachable", 128);
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry", d({f("callee", FlatSymbolRefAttr::get(&context, "Root")),
                      f("parameters", b.getArrayAttr({})),
                      f("type_arguments", b.getArrayAttr({}))}))});
    ac::HardwareAnalysis analysis(*package);
    auto accepted = capture([&] { return analysis.getSourceCheckPlan(); });
    ASSERT_TRUE(succeeded(accepted)) << diagnostics;
    ASSERT_EQ(accepted->bindings.size(), 128u);
    EXPECT_EQ(accepted->checks.size(), reachable ? 128u : 0u);
    // Only the last association is malformed. Equal physical values do not
    // authorize the earlier prefix result as this obligation's carrier.
    finalExpect->setOperand(0, checkedRule.getResult(0));
    auto tiny = capture([&] { return analysis.getSourceCheckPlan({1}); });
    EXPECT_TRUE(failed(tiny));
    expectBudgetOnly();
    auto revalidated =
        capture([&] { return analysis.verifySourceCheckPlan(*accepted, {1}); });
    EXPECT_TRUE(failed(revalidated));
    expectBudgetOnly();
    auto ample = capture([&] { return analysis.getSourceCheckPlan(); });
    EXPECT_TRUE(failed(ample));
    EXPECT_NE(
        diagnostics.find("expect operands must match owning rule check suffix"),
        std::string::npos)
        << diagnostics;
  }
  void expectBudgetOnly() {
    EXPECT_NE(diagnostics.find("source-check plan exceeds analysis work budget "
                               "before occurrence expansion"),
              std::string::npos)
        << diagnostics;
    EXPECT_EQ(
        diagnostics.find("expect operands must match owning rule check suffix"),
        std::string::npos)
        << diagnostics;
  }
};

TEST_F(HardwareSourceCheckValidationBudgetTest,
       SelectedDefinitionBudgetAdmissionPrecedesLateAssociation) {
  budgetPrecedesAssociation(true);
}

TEST_F(HardwareSourceCheckValidationBudgetTest,
       UnreachableDefinitionBudgetAdmissionPrecedesLateAssociation) {
  budgetPrecedesAssociation(false);
}
} // namespace
