#include "Dialect/ACIR/ACIRStaticEvaluation.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

class SourceStaticEvaluationTest : public ::testing::Test {
protected:
  SourceStaticEvaluationTest() : builder(&context) {
    context.loadDialect<ACIRDialect, mlir::func::FuncDialect>();
    container = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
    auto scope = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
    container->getBody()->push_back(scope);
    evaluationSite = scope;
    sourceLocation = builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("expressions.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
    occurrence = builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "site",
             builder.getDictionaryAttr(
                 {builder.getNamedAttr(
                      "definition",
                      mlir::FlatSymbolRefAttr::get(&context, "Expressions")),
                  builder.getNamedAttr("ast_path", builder.getArrayAttr({}))})),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  }
  StaticExprAttr expression(llvm::StringRef kind,
                            llvm::ArrayRef<mlir::NamedAttribute> fields) {
    llvm::SmallVector<mlir::NamedAttribute> attributes{
        builder.getNamedAttr("kind", builder.getStringAttr(kind)),
        builder.getNamedAttr("origin", occurrence),
        builder.getNamedAttr("location", sourceLocation)};
    llvm::append_range(attributes, fields);
    return StaticExprAttr::get(&context, builder.getDictionaryAttr(attributes));
  }
  StaticExprAttr checkedExpression(
      llvm::StringRef kind, llvm::ArrayRef<mlir::NamedAttribute> fields) {
    llvm::SmallVector<mlir::NamedAttribute> attributes{
        builder.getNamedAttr("kind", builder.getStringAttr(kind)),
        builder.getNamedAttr("origin", occurrence),
        builder.getNamedAttr("location", sourceLocation)};
    llvm::append_range(attributes, fields);
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    return StaticExprAttr::getChecked(
        [&] { return mlir::emitError(mlir::UnknownLoc::get(&context)); },
        &context, builder.getDictionaryAttr(attributes));
  }
  StaticExprAttr integer(llvm::StringRef value) {
    return expression(
        "literal",
        {builder.getNamedAttr(
            "value",
            builder.getDictionaryAttr(
                {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                 builder.getNamedAttr(
                     "value",
                     MathIntAttr::get(&context, llvm::APSInt(value)))}))});
  }
  StaticExprAttr boolean(bool value) {
    return expression(
        "literal",
        {builder.getNamedAttr(
            "value",
            builder.getDictionaryAttr(
                {builder.getNamedAttr("kind", builder.getStringAttr("bool")),
                 builder.getNamedAttr("value", builder.getBoolAttr(value))}))});
  }
  StaticExprAttr unary(llvm::StringRef opcode, StaticExprAttr operand) {
    return expression("unary", {builder.getNamedAttr(
                                    "operator", builder.getStringAttr(opcode)),
                                builder.getNamedAttr("operand", operand)});
  }
  StaticExprAttr binary(llvm::StringRef opcode, StaticExprAttr lhs,
                        StaticExprAttr rhs) {
    return expression(
        "binary",
        {builder.getNamedAttr("operator", builder.getStringAttr(opcode)),
         builder.getNamedAttr("lhs", lhs), builder.getNamedAttr("rhs", rhs)});
  }
  StaticExprAttr select(StaticExprAttr condition, StaticExprAttr yes,
                        StaticExprAttr no) {
    return expression("select", {builder.getNamedAttr("condition", condition),
                                 builder.getNamedAttr("yes", yes),
                                 builder.getNamedAttr("no", no)});
  }
  mlir::FailureOr<mlir::DictionaryAttr> evaluate(StaticExprAttr expr) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    auto emit = [&] {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return detail::evaluateSourceStaticExpr(expr, evaluationSite, emit);
  }
  void expectInteger(StaticExprAttr expr, llvm::StringRef expected) {
    auto result = evaluate(expr);
    ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
    ASSERT_EQ(result->getAs<mlir::StringAttr>("kind").getValue(), "integer");
    auto value = result->getAs<MathIntAttr>("value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value.getCanonicalValue(), expected);
  }
  void expectBoolean(StaticExprAttr expr, bool expected) {
    auto result = evaluate(expr);
    ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
    ASSERT_EQ(result->getAs<mlir::StringAttr>("kind").getValue(), "bool");
    auto value = result->getAs<mlir::BoolAttr>("value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value.getValue(), expected);
  }
  std::string rejects(StaticExprAttr expr) {
    EXPECT_TRUE(mlir::failed(evaluate(expr)));
    EXPECT_FALSE(diagnostics.empty());
    return diagnostics;
  }
  StaticExprAttr zeroDivision() {
    return binary("floordiv", integer("7"), integer("0"));
  }
  StaticExprAttr negativeShift() {
    return binary("shl", integer("3"), integer("-1"));
  }
  StaticExprAttr faultAsBool(StaticExprAttr fault) {
    return binary("eq", fault, integer("0"));
  }

  mlir::MLIRContext context;
  mlir::Builder builder;
  mlir::OwningOpRef<mlir::ModuleOp> container;
  mlir::Operation *evaluationSite = nullptr;
  mlir::DictionaryAttr occurrence, sourceLocation;
  std::string diagnostics;
};

TEST_F(SourceStaticEvaluationTest,
       LiteralAndClosedMathematicalResultsRemainExact) {
  expectBoolean(boolean(true), true);
  expectInteger(integer("-340282366920938463463374607431768211456"),
                "-340282366920938463463374607431768211456");
  expectInteger(binary("mul", integer("18446744073709551616"),
                       integer("18446744073709551616")),
                "340282366920938463463374607431768211456");
  expectInteger(
      unary("neg",
            binary("add", integer("340282366920938463463374607431768211456"),
                   integer("1"))),
      "-340282366920938463463374607431768211457");
  expectInteger(binary("floordiv", integer("-7"), integer("3")), "-3");
  expectInteger(binary("mod", integer("7"), integer("-3")), "-2");
  expectInteger(binary("xor_bits", integer("-5"), integer("3")), "-8");
  expectInteger(unary("invert", integer("18446744073709551616")),
                "-18446744073709551617");
  expectInteger(binary("shr", integer("-5"), integer("1")), "-3");
  expectBoolean(binary("lt", integer("-18446744073709551617"), integer("-1")),
                true);
}

TEST_F(SourceStaticEvaluationTest,
       NotAndToIntKeepBooleanAndIntegerKindsDistinct) {
  expectBoolean(unary("not", boolean(false)), true);
  expectInteger(unary("to_int", boolean(false)), "0");
  expectInteger(unary("to_int", boolean(true)), "1");
  expectInteger(unary("to_int", integer("-18446744073709551617")),
                "-18446744073709551617");
  rejects(unary("not", integer("0")));
  rejects(binary("add", boolean(false), integer("1")));
  rejects(binary("eq", boolean(true), integer("1")));
  rejects(select(integer("1"), integer("3"), integer("4")));
}

TEST_F(SourceStaticEvaluationTest,
       StrictBinariesStopAtTheFirstDemandedFailure) {
  // Each fault is valid grammar and integer kind. Their independent diagnostics
  // identify which operand failed; reversing operands must reverse first
  // failure.
  const auto divide = rejects(zeroDivision());
  const auto shift = rejects(negativeShift());
  ASSERT_NE(divide, shift);
  EXPECT_EQ(rejects(binary("add", zeroDivision(), negativeShift())), divide);
  EXPECT_EQ(rejects(binary("add", negativeShift(), zeroDivision())), shift);
}

TEST_F(SourceStaticEvaluationTest,
       ShortCircuitAndSelectDoNotExecuteInactiveArithmeticFaults) {
  for (auto fault : {zeroDivision(), negativeShift()}) {
    auto condition = faultAsBool(fault);
    expectBoolean(binary("and_bool", boolean(false), condition), false);
    expectBoolean(binary("or_bool", boolean(true), condition), true);
    expectInteger(select(boolean(true), integer("23"), fault), "23");
    expectInteger(select(boolean(false), fault, integer("-19")), "-19");
    rejects(binary("and_bool", boolean(true), condition));
    rejects(binary("or_bool", boolean(false), condition));
    rejects(select(boolean(false), integer("23"), fault));
    rejects(select(boolean(true), fault, integer("-19")));
    EXPECT_EQ(rejects(select(faultAsBool(fault), integer("3"), integer("4"))),
              rejects(fault)); // Condition failure precedes either arm.
  }
}

TEST_F(SourceStaticEvaluationTest,
       CompletePreflightRejectsInvalidUnusedKindsAndGrammar) {
  const auto wrongKind = binary("add", boolean(false), integer("1"));
  rejects(select(boolean(true), integer("23"), wrongKind));
  const auto unknownOpcode = checkedExpression(
      "binary", {builder.getNamedAttr("operator", builder.getStringAttr("unknown")),
                 builder.getNamedAttr("lhs", integer("1")),
                 builder.getNamedAttr("rhs", integer("2"))});
  EXPECT_FALSE(unknownOpcode);
  EXPECT_NE(diagnostics.find("binary StaticExpr operator or fields are invalid"),
            std::string::npos);
  rejects(binary("and_bool", boolean(false), integer("0")));
  rejects(binary("or_bool", boolean(true), integer("1")));
  rejects(binary("and_bool", boolean(false), unary("not", integer("0"))));
  rejects(
      binary("or_bool", boolean(true), binary("eq", wrongKind, integer("0"))));
  auto unresolvedReference = expression(
      "reference",
      {builder.getNamedAttr(
          "ref",
          builder.getDictionaryAttr(
              {builder.getNamedAttr("kind", builder.getStringAttr("export")),
               builder.getNamedAttr("symbol", mlir::FlatSymbolRefAttr::get(
                                                  &context, "Unopened"))}))});
  auto unopenedCall = expression(
      "call", {builder.getNamedAttr("callee", mlir::FlatSymbolRefAttr::get(
                                                  &context, "Unopened")),
               builder.getNamedAttr("arguments", builder.getArrayAttr({}))});
  rejects(select(boolean(true), integer("3"), unresolvedReference));
  rejects(select(boolean(true), integer("3"), unopenedCall));
}

TEST_F(SourceStaticEvaluationTest,
       StaticHeterogeneousChoicesRespectActualSelectedKind) {
  for (bool chosen : {false, true}) {
    auto condition = binary("eq", integer(chosen ? "1" : "0"), integer("1"));
    auto choice = select(condition, boolean(true), integer("3"));
    if (chosen)
      expectBoolean(choice, true);
    else
      expectInteger(choice, "3");
    expectInteger(unary("to_int", choice), chosen ? "1" : "3");
    auto partial = select(condition, integer("3"), boolean(true));
    if (chosen)
      expectInteger(unary("neg", partial), "-3");
    else
      rejects(unary("neg", partial));
  }
  expectBoolean(select(boolean(true), boolean(false), integer("3")), false);
  expectInteger(select(boolean(false), boolean(false), integer("3")), "3");
  rejects(unary("neg", select(binary("eq", integer("1"), integer("1")),
                              boolean(false), boolean(true))));
}

TEST_F(SourceStaticEvaluationTest,
       AggregateSelectionPreservesCompleteValuesAndChecksUnusedGrammar) {
  auto boolValue = boolean(true).getTree().getAs<mlir::DictionaryAttr>("value");
  auto integerValue = integer("18446744073709551617")
                          .getTree()
                          .getAs<mlir::DictionaryAttr>("value");
  auto listValue = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("list")),
       builder.getNamedAttr("values",
                            builder.getArrayAttr({boolValue, integerValue}))});
  auto recordValue = [&](llvm::StringRef name) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("record")),
         builder.getNamedAttr("symbol",
                              mlir::FlatSymbolRefAttr::get(&context, name)),
         builder.getNamedAttr(
             "fields", builder.getArrayAttr({integerValue, listValue}))});
  };
  auto first = recordValue("Records.First");
  auto second = recordValue("Records.Second");
  auto literalValue = [&](mlir::DictionaryAttr value) {
    return expression("literal", {builder.getNamedAttr("value", value)});
  };
  for (bool chosen : {false, true}) {
    auto result = evaluate(
        select(boolean(chosen), literalValue(first), literalValue(listValue)));
    ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
    EXPECT_EQ(*result, chosen ? first : listValue);
    auto condition = binary("eq", integer(chosen ? "1" : "0"), integer("1"));
    result =
        evaluate(select(condition, literalValue(first), literalValue(second)));
    ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
    EXPECT_EQ(*result, chosen ? first : second);
  }
  mlir::NamedAttrList malformed(first);
  malformed.set("fields", builder.getArrayAttr(
                              {builder.getStringAttr("not a StaticValue")}));
  // Invalid grammar is rejected while constructing the checked attribute;
  // it cannot become an unselected branch of a verified expression.
  auto invalid = checkedExpression(
      "literal", {builder.getNamedAttr("value", malformed.getDictionary(&context))});
  EXPECT_FALSE(invalid);
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceStaticEvaluationTest,
       SharedExpressionDagsAndRebuiltLeavesHaveFreshPerCallResults) {
  auto dag = [&](StaticExprAttr leaf, unsigned depth) {
    for (unsigned index = 0; index < depth; ++index)
      leaf = binary("add", leaf, leaf);
    return leaf;
  };
  expectInteger(dag(integer("1"), 8), "256");
  expectInteger(dag(integer("1"), 64), "18446744073709551616");
  expectInteger(dag(integer("3"), 64), "55340232221128654848");
  rejects(dag(boolean(true), 64));
  expectInteger(dag(integer("1"), 64), "18446744073709551616");
}

} // namespace
} // namespace acir::ac
