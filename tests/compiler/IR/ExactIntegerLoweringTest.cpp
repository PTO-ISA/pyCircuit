#include "Compiler/NumericLowering.h"
#include "Dialect/ACIR/SourceValueSemantics.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <optional>
#include <string>

namespace {
namespace ac = acir::ac;
using namespace acir::compiler;
using ac::detail::ValueKind;
using ac::detail::ValueOpcode;

llvm::APSInt integer(llvm::StringRef text) {
  bool negative = text.consume_front("-");
  llvm::APInt value(std::max(256u, unsigned(text.size() * 4 + 2)), text, 10);
  return llvm::APSInt(negative ? -value : value, false);
}
std::string decimal(const llvm::APSInt &value) {
  llvm::SmallString<96> text;
  value.toString(text);
  return text.str().str();
}

class ExactIntegerLoweringTest : public ::testing::Test {
protected:
  ExactIntegerLoweringTest() : builder(&context) {
    context.loadDialect<ac::ACIRDialect>();
    builder.setInsertionPointToEnd(&block);
    auto definition = mlir::FlatSymbolRefAttr::get(&context, "numeric.Test");
    auto site = builder.getDictionaryAttr(
        {builder.getNamedAttr("definition", definition),
         builder.getNamedAttr("ast_path", builder.getArrayAttr({}))});
    origin = builder.getDictionaryAttr(
        {builder.getNamedAttr("site", site),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
    span = builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("numeric.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
  }
  NumericLoweringSite site() { return {builder.getUnknownLoc(), origin, span}; }
  ac::StaticExprAttr literal(llvm::StringRef value) {
    auto integerValue = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
         builder.getNamedAttr("value",
                              ac::MathIntAttr::get(&context, integer(value)))});
    return ac::StaticExprAttr::get(
        &context,
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
             builder.getNamedAttr("value", integerValue),
             builder.getNamedAttr("origin", origin),
             builder.getNamedAttr("location", span)}));
  }
  ac::BitsType type(unsigned width) {
    return ac::BitsType::get(&context, literal(std::to_string(width)));
  }
  NumericValue input(unsigned width, llvm::StringRef lower,
                     llvm::StringRef upper) {
    return {block.addArgument(type(width), builder.getUnknownLoc()),
            ValueKind::Integer,
            IntegerInterval{integer(lower), integer(upper)}};
  }
  NumericValue constant(unsigned width, llvm::StringRef value) {
    mlir::OperationState state(builder.getUnknownLoc(),
                               ac::BitsConstantOp::getOperationName());
    state.addTypes(type(width));
    state.addAttribute("value", literal(value));
    auto result = builder.create(state)->getResult(0);
    auto bound = integer(value);
    auto upper = bound + integer("1");
    return {result, ValueKind::Integer, IntegerInterval{bound, upper}};
  }
  unsigned width(mlir::Value value) {
    auto expr = mlir::cast<ac::BitsType>(value.getType()).getWidth().getTree();
    auto spelling = expr.getAs<mlir::DictionaryAttr>("value")
                        .getAs<ac::MathIntAttr>("value")
                        .getCanonicalValue();
    return static_cast<unsigned>(std::stoul(spelling.str()));
  }
  std::string snapshot() {
    std::string result;
    llvm::raw_string_ostream stream(result);
    for (auto &operation : block)
      operation.print(stream);
    return result;
  }
  void interval(const NumericValue &value, llvm::StringRef lower,
                llvm::StringRef upper) {
    ASSERT_EQ(value.sourceKind, ValueKind::Integer);
    ASSERT_TRUE(value.interval);
    EXPECT_EQ(decimal(value.interval->lower), lower.str());
    EXPECT_EQ(decimal(value.interval->upper), upper.str());
  }
  // Evaluate only ordinary emitted bit-vector SSA, with explicitly supplied
  // finite inputs. Expected mathematical values below are independent literals.
  llvm::APInt evaluate(mlir::Value value,
                       const llvm::DenseMap<mlir::Value, llvm::APInt> &inputs) {
    if (auto found = inputs.find(value); found != inputs.end())
      return found->second.zextOrTrunc(width(value));
    auto *op = value.getDefiningOp();
    EXPECT_NE(op, nullptr);
    if (!op)
      return llvm::APInt(width(value), 0);
    if (auto c = mlir::dyn_cast<ac::BitsConstantOp>(op)) {
      auto text = c.getValue()
                      .getTree()
                      .getAs<mlir::DictionaryAttr>("value")
                      .getAs<ac::MathIntAttr>("value")
                      .getCanonicalValue();
      return integer(text).zextOrTrunc(width(value));
    }
    if (auto resize = mlir::dyn_cast<ac::BitsResizeOp>(op)) {
      auto input = evaluate(resize.getInput(), inputs);
      return resize.getMode() == "sext" ? input.sextOrTrunc(width(value))
                                        : input.zextOrTrunc(width(value));
    }
    if (auto extract = mlir::dyn_cast<ac::BitsExtractOp>(op)) {
      auto low = extract.getLow()
                     .getTree()
                     .getAs<mlir::DictionaryAttr>("value")
                     .getAs<ac::MathIntAttr>("value")
                     .getCanonicalValue();
      return evaluate(extract.getInput(), inputs)
          .extractBits(width(value),
                       static_cast<unsigned>(std::stoul(low.str())));
    }
    if (auto binary = mlir::dyn_cast<ac::BitsBinaryOp>(op)) {
      auto lhs = evaluate(binary.getLhs(), inputs);
      auto rhs = evaluate(binary.getRhs(), inputs);
      if (binary.getOpcode() == "add")
        return lhs + rhs;
      if (binary.getOpcode() == "sub")
        return lhs - rhs;
      if (binary.getOpcode() == "mul")
        return lhs * rhs;
      if (binary.getOpcode() == "and")
        return lhs & rhs;
    }
    if (auto selected = mlir::dyn_cast<ac::BitsSelectOp>(op)) {
      auto condition = evaluate(selected.getCondition(), inputs);
      return evaluate(condition.isZero() ? selected.getFalseValue()
                                         : selected.getTrueValue(),
                      inputs);
    }
    ADD_FAILURE() << "Unexpected operation in exact numeric lowering";
    return llvm::APInt(width(value), 0);
  }
  void expectValue(const NumericValue &result,
                   const llvm::DenseMap<mlir::Value, llvm::APInt> &inputs,
                   llvm::StringRef expected) {
    auto bits = evaluate(result.value, inputs);
    bool isSigned = result.interval && result.interval->lower.isNegative();
    EXPECT_EQ(decimal(llvm::APSInt(bits, !isSigned)), expected.str());
  }
  void verifyOperations() {
    for (auto &operation : block)
      EXPECT_TRUE(mlir::succeeded(mlir::verify(&operation)));
  }
  mlir::MLIRContext context;
  mlir::OpBuilder builder;
  mlir::Block block;
  mlir::DictionaryAttr origin, span;
};

TEST_F(ExactIntegerLoweringTest,
       TwoDynamicOperandsHaveExactIntervalsAndValues) {
  auto a = input(8, "0", "256"), b = input(8, "0", "256");
  auto sum = lowerExactIntegerBinary(builder, site(), ValueOpcode::Add, a, b);
  auto difference =
      lowerExactIntegerBinary(builder, site(), ValueOpcode::Sub, a, b);
  auto product =
      lowerExactIntegerBinary(builder, site(), ValueOpcode::Mul, a, b);
  ASSERT_TRUE(mlir::succeeded(sum) && mlir::succeeded(difference) &&
              mlir::succeeded(product));
  interval(*sum, "0", "511");
  interval(*difference, "-255", "256");
  interval(*product, "0", "65026");
  EXPECT_GE(width(sum->value), 9u);
  EXPECT_GE(width(difference->value), 9u);
  EXPECT_GE(width(product->value), 16u);
  llvm::DenseMap<mlir::Value, llvm::APInt> values{
      {a.value, llvm::APInt(8, 255)}, {b.value, llvm::APInt(8, 254)}};
  expectValue(*sum, values, "509");
  expectValue(*difference, values, "1");
  expectValue(*product, values, "64770");
  values[a.value] = llvm::APInt(8, 0);
  values[b.value] = llvm::APInt(8, 255);
  expectValue(*difference, values, "-255");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       SignedIntermediatesExtendBeforeNestedMultiply) {
  auto a = input(4, "-8", "8"), b = input(4, "-8", "8");
  auto sum = lowerExactIntegerBinary(builder, site(), ValueOpcode::Add, a, b);
  ASSERT_TRUE(mlir::succeeded(sum));
  interval(*sum, "-16", "15");
  auto product =
      lowerExactIntegerBinary(builder, site(), ValueOpcode::Mul, *sum, b);
  ASSERT_TRUE(mlir::succeeded(product));
  interval(*product, "-112", "129");
  expectValue(*product,
              {{a.value, llvm::APInt(4, 8)}, {b.value, llvm::APInt(4, 8)}},
              "128");
  expectValue(*product,
              {{a.value, llvm::APInt(4, 7)}, {b.value, llvm::APInt(4, 8)}},
              "8");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       WideArithmeticDoesNotStopAtHostOrOperandWidth) {
  auto a = input(65, "0", "36893488147419103232");
  auto b = input(65, "0", "36893488147419103232");
  auto sum = lowerExactIntegerBinary(builder, site(), ValueOpcode::Add, a, b);
  ASSERT_TRUE(mlir::succeeded(sum));
  interval(*sum, "0", "73786976294838206463");
  EXPECT_GE(width(sum->value), 66u);
  auto product =
      lowerExactIntegerBinary(builder, site(), ValueOpcode::Mul, a, b);
  ASSERT_TRUE(mlir::succeeded(product));
  EXPECT_GE(width(product->value), 130u);
  llvm::APInt maximum(65, "36893488147419103231", 10);
  expectValue(*sum, {{a.value, maximum}, {b.value, maximum}},
              "73786976294838206462");
  expectValue(*product, {{a.value, maximum}, {b.value, maximum}},
              "1361129467683753853779711453432234639361");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest, SwappedNonFullAndZeroMasksKeepSharedWideSSA) {
  auto a = input(8, "0", "256"), b = input(8, "0", "256");
  auto sum = lowerExactIntegerBinary(builder, site(), ValueOpcode::Add, a, b);
  ASSERT_TRUE(mlir::succeeded(sum));
  auto mask = constant(8, "171"), zero = constant(1, "0");
  auto masked = lowerExactIntegerBinary(builder, site(), ValueOpcode::AndBits,
                                        *sum, mask);
  auto swapped = lowerExactIntegerBinary(builder, site(), ValueOpcode::AndBits,
                                         mask, *sum);
  auto cleared = lowerExactIntegerBinary(builder, site(), ValueOpcode::AndBits,
                                         *sum, zero);
  ASSERT_TRUE(mlir::succeeded(masked) && mlir::succeeded(swapped) &&
              mlir::succeeded(cleared));
  interval(*masked, "0", "172");
  interval(*swapped, "0", "172");
  interval(*cleared, "0", "1");
  auto narrow = lowerExactIntegerBoundary(builder, site(), *masked, type(8));
  ASSERT_TRUE(mlir::succeeded(narrow));
  EXPECT_EQ(width(*narrow), 8u);
  auto original = sum->value;
  auto before = snapshot();
  auto same = lowerExactIntegerBoundary(
      builder, site(), *sum, mlir::cast<ac::BitsType>(original.getType()));
  ASSERT_TRUE(mlir::succeeded(same));
  EXPECT_EQ(*same, original);
  EXPECT_EQ(snapshot(), before);
  llvm::DenseMap<mlir::Value, llvm::APInt> values{
      {a.value, llvm::APInt(8, 255)}, {b.value, llvm::APInt(8, 255)}};
  expectValue(*sum, values, "510");
  expectValue(*masked, values, "170");
  expectValue(*swapped, values, "170");
  expectValue(*cleared, values, "0");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest, SingletonDynamicValueIsNotAKnownConstantMask) {
  auto value = input(9, "0", "511"), opaqueMask = input(8, "171", "172");
  auto before = snapshot();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(lowerExactIntegerBinary(
      builder, site(), ValueOpcode::AndBits, value, opaqueMask)));
  EXPECT_EQ(snapshot(), before);
}

TEST_F(ExactIntegerLoweringTest,
       EncodedSignedMinusOneIsNotAPositiveOneBitMask) {
  auto a = input(8, "0", "256");
  auto minusOne = constant(1, "1");
  minusOne.interval = IntegerInterval{integer("-1"), integer("0")};
  auto positiveMask = constant(8, "255");
  auto before = snapshot();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(lowerExactIntegerBinary(
      builder, site(), ValueOpcode::AndBits, a, minusOne)));
  EXPECT_EQ(snapshot(), before);
  EXPECT_TRUE(mlir::failed(lowerExactIntegerBinary(
      builder, site(), ValueOpcode::AndBits, minusOne, a)));
  EXPECT_EQ(snapshot(), before);
  auto masked = lowerExactIntegerBinary(builder, site(), ValueOpcode::AndBits,
                                        minusOne, positiveMask);
  ASSERT_TRUE(mlir::succeeded(masked));
  interval(*masked, "0", "256");
  expectValue(*masked, llvm::DenseMap<mlir::Value, llvm::APInt>{}, "255");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       ComputedSameWidthNeverProducesInvalidSuccessfulIR) {
  context.loadDialect<mlir::func::FuncDialect>();
  auto computed = ac::StaticExprAttr::get(
      &context,
      builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
           builder.getNamedAttr("operator", builder.getStringAttr("add")),
           builder.getNamedAttr("lhs", literal("4")),
           builder.getNamedAttr("rhs", literal("4")),
           builder.getNamedAttr("origin", origin),
           builder.getNamedAttr("location", span)}));
  auto computedType = ac::BitsType::get(&context, computed);
  auto module = mlir::ModuleOp::create(builder.getUnknownLoc());
  mlir::OwningOpRef<mlir::ModuleOp> owned(module);
  builder.setInsertionPointToStart(module.getBody());
  auto function = builder.create<mlir::func::FuncOp>(
      builder.getUnknownLoc(), "numeric.Test",
      builder.getFunctionType({computedType}, {}));
  auto *body = function.addEntryBlock();
  builder.setInsertionPointToEnd(body);
  builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
  builder.setInsertionPointToStart(body);
  NumericValue argument{body->getArgument(0), ValueKind::Integer,
                        IntegerInterval{integer("0"), integer("256")}};
  auto mask = constant(8, "15");
  auto print = [&] {
    std::string text;
    llvm::raw_string_ostream stream(text);
    module.print(stream);
    return text;
  };
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto before = print();
  auto converted =
      lowerExactIntegerBoundary(builder, site(), argument, type(8));
  if (mlir::failed(converted)) {
    EXPECT_EQ(print(), before);
  } else {
    EXPECT_TRUE(ac::areEquivalentHardwareTypes(converted->getType(), type(8)));
    EXPECT_TRUE(mlir::succeeded(mlir::verify(module)));
  }
  before = print();
  auto masked = lowerExactIntegerBinary(builder, site(), ValueOpcode::AndBits,
                                        argument, mask);
  if (mlir::failed(masked)) {
    EXPECT_EQ(print(), before);
  } else {
    interval(*masked, "0", "16");
    EXPECT_TRUE(mlir::succeeded(mlir::verify(module)));
  }
  EXPECT_EQ(body->getArgument(0).getType(), computedType);
}

TEST_F(ExactIntegerLoweringTest,
       FailedPlanningLeavesInputSSAAndInventoryUntouched) {
  auto a = input(8, "0", "256"), b = input(8, "0", "256");
  auto bad = a;
  bad.sourceKind = ValueKind::Boolean;
  auto unknown = a;
  unknown.sourceKind.reset();
  auto unproven = a;
  unproven.interval.reset();
  auto negative = constant(4, "-1");
  auto empty = input(8, "2", "2");
  auto overflow = input(9, "0", "511"), signedValue = input(9, "-255", "256");
  auto reference = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("parameter")),
       builder.getNamedAttr(
           "owner", mlir::FlatSymbolRefAttr::get(&context, "numeric.Test")),
       builder.getNamedAttr("name", builder.getStringAttr("WIDTH"))});
  auto symbolic = ac::BitsType::get(
      &context,
      ac::StaticExprAttr::get(
          &context,
          builder.getDictionaryAttr(
              {builder.getNamedAttr("kind", builder.getStringAttr("reference")),
               builder.getNamedAttr("ref", reference),
               builder.getNamedAttr("origin", origin),
               builder.getNamedAttr("location", span)})));
  auto before = snapshot();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  for (const auto &invalid : {bad, unknown, unproven, empty}) {
    EXPECT_TRUE(mlir::failed(lowerExactIntegerBinary(
        builder, site(), ValueOpcode::Add, invalid, b)));
    EXPECT_EQ(snapshot(), before);
  }
  EXPECT_TRUE(mlir::failed(lowerExactIntegerBinary(
      builder, site(), ValueOpcode::AndBits, a, negative)));
  EXPECT_TRUE(mlir::failed(
      lowerExactIntegerBinary(builder, site(), ValueOpcode::FloorDiv, a, b)));
  EXPECT_TRUE(mlir::failed(
      lowerExactIntegerBoundary(builder, site(), overflow, type(8))));
  EXPECT_TRUE(mlir::failed(
      lowerExactIntegerBoundary(builder, site(), signedValue, type(9))));
  EXPECT_TRUE(
      mlir::failed(lowerExactIntegerBoundary(builder, site(), a, symbolic)));
  EXPECT_TRUE(
      mlir::failed(lowerExactIntegerBoundary(builder, site(), bad, type(8))));
  EXPECT_TRUE(mlir::failed(
      lowerExactIntegerBoundary(builder, site(), unproven, type(8))));
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(a.value.getType(), type(8));
  EXPECT_EQ(b.value.getType(), type(8));
}

TEST_F(ExactIntegerLoweringTest,
       SelectUnionsFiniteIntervalsAcrossActualWidths) {
  NumericValue condition{block.addArgument(type(1), builder.getUnknownLoc()),
                         ValueKind::Boolean, std::nullopt};
  auto wide = input(12, "0", "4096"), narrow = input(5, "0", "32");
  auto selected =
      lowerExactIntegerSelect(builder, site(), condition, wide, narrow);
  ASSERT_TRUE(mlir::succeeded(selected));
  interval(*selected, "0", "4096");
  EXPECT_GE(width(selected->value), 12u);
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 1)},
               {wide.value, llvm::APInt(12, 4095)},
               {narrow.value, llvm::APInt(5, 31)}},
              "4095");
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 0)},
               {wide.value, llvm::APInt(12, 4095)},
               {narrow.value, llvm::APInt(5, 31)}},
              "31");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       SelectSignedUnsignedUnionProtectsUnsignedSignBit) {
  NumericValue condition{block.addArgument(type(1), builder.getUnknownLoc()),
                         ValueKind::Boolean, std::nullopt};
  auto signedValue = input(8, "-128", "128"),
       unsignedValue = input(8, "0", "256");
  auto selected = lowerExactIntegerSelect(builder, site(), condition,
                                          signedValue, unsignedValue);
  ASSERT_TRUE(mlir::succeeded(selected));
  interval(*selected, "-128", "256");
  EXPECT_GE(width(selected->value), 9u);
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 1)},
               {signedValue.value, llvm::APInt(8, 128)},
               {unsignedValue.value, llvm::APInt(8, 255)}},
              "-128");
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 0)},
               {signedValue.value, llvm::APInt(8, 128)},
               {unsignedValue.value, llvm::APInt(8, 255)}},
              "255");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       SelectRetainsWideSSAWithNarrowProvenIntervals) {
  NumericValue condition{block.addArgument(type(1), builder.getUnknownLoc()),
                         ValueKind::Boolean, std::nullopt};
  auto actualWide = input(65, "0", "2"), signedSmall = input(12, "-2", "2");
  auto selected = lowerExactIntegerSelect(builder, site(), condition,
                                          actualWide, signedSmall);
  ASSERT_TRUE(mlir::succeeded(selected));
  interval(*selected, "-2", "2");
  EXPECT_GE(width(selected->value), 66u);
  for (auto resized : block.getOps<ac::BitsResizeOp>())
    EXPECT_NE(resized.getMode(), "trunc");
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 1)},
               {actualWide.value, llvm::APInt(65, 1)},
               {signedSmall.value, llvm::APInt(12, 4095)}},
              "1");
  expectValue(*selected,
              {{condition.value, llvm::APInt(1, 0)},
               {actualWide.value, llvm::APInt(65, 1)},
               {signedSmall.value, llvm::APInt(12, 4095)}},
              "-1");
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       SelectRejectsUnknownAndMixedKindsBeforeMutation) {
  NumericValue condition{block.addArgument(type(1), builder.getUnknownLoc()),
                         ValueKind::Boolean, std::nullopt};
  auto trueValue = input(8, "0", "256"), falseValue = input(8, "0", "256");
  auto integerCondition = condition;
  integerCondition.sourceKind = ValueKind::Integer;
  integerCondition.interval = IntegerInterval{integer("0"), integer("2")};
  auto unknownCondition = condition;
  unknownCondition.sourceKind.reset();
  auto wideCondition = condition;
  wideCondition.value = block.addArgument(type(2), builder.getUnknownLoc());
  auto nonBitsCondition = condition;
  nonBitsCondition.value =
      block.addArgument(builder.getI1Type(), builder.getUnknownLoc());
  auto booleanBranch = trueValue;
  booleanBranch.sourceKind = ValueKind::Boolean;
  auto unknownBranch = trueValue;
  unknownBranch.sourceKind.reset();
  auto unprovenBranch = trueValue;
  unprovenBranch.interval.reset();
  auto oversizedInterval = trueValue;
  oversizedInterval.interval = IntegerInterval{integer("0"), integer("512")};
  auto before = snapshot();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  for (const auto &invalid :
       {integerCondition, unknownCondition, wideCondition, nonBitsCondition}) {
    EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(builder, site(), invalid,
                                                     trueValue, falseValue)));
    EXPECT_EQ(snapshot(), before);
  }
  for (const auto &invalid :
       {booleanBranch, unknownBranch, unprovenBranch, oversizedInterval}) {
    EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(builder, site(), condition,
                                                     invalid, falseValue)));
    EXPECT_EQ(snapshot(), before);
    EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(builder, site(), condition,
                                                     falseValue, invalid)));
    EXPECT_EQ(snapshot(), before);
  }
  EXPECT_EQ(trueValue.value.getType(), type(8));
  EXPECT_EQ(falseValue.value.getType(), type(8));
}

TEST_F(ExactIntegerLoweringTest,
       SelectRejectsNoncanonicalSameWidthTypesWithoutMutation) {
  context.loadDialect<mlir::func::FuncDialect>();
  auto expression = [&](llvm::StringRef left, llvm::StringRef right) {
    return ac::BitsType::get(
        &context,
        ac::StaticExprAttr::get(
            &context,
            builder.getDictionaryAttr(
                {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
                 builder.getNamedAttr("operator", builder.getStringAttr("add")),
                 builder.getNamedAttr("lhs", literal(left)),
                 builder.getNamedAttr("rhs", literal(right)),
                 builder.getNamedAttr("origin", origin),
                 builder.getNamedAttr("location", span)})));
  };
  auto computedEight = expression("4", "4"), computedOne = expression("0", "1");
  auto module = mlir::ModuleOp::create(builder.getUnknownLoc());
  mlir::OwningOpRef<mlir::ModuleOp> owned(module);
  builder.setInsertionPointToStart(module.getBody());
  auto function = builder.create<mlir::func::FuncOp>(
      builder.getUnknownLoc(), "numeric.Test",
      builder.getFunctionType({type(1), computedOne, computedEight, type(8)},
                              {}));
  auto *body = function.addEntryBlock();
  builder.setInsertionPointToEnd(body);
  builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
  builder.setInsertionPointToStart(body);
  NumericValue condition{body->getArgument(0), ValueKind::Boolean,
                         std::nullopt};
  NumericValue computedCondition{body->getArgument(1), ValueKind::Boolean,
                                 std::nullopt};
  NumericValue computedBranch{body->getArgument(2), ValueKind::Integer,
                              IntegerInterval{integer("0"), integer("256")}};
  NumericValue plainBranch{body->getArgument(3), ValueKind::Integer,
                           IntegerInterval{integer("0"), integer("256")}};
  auto print = [&] {
    std::string text;
    llvm::raw_string_ostream stream(text);
    module.print(stream);
    return text;
  };
  auto before = print();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(
      builder, site(), computedCondition, plainBranch, plainBranch)));
  EXPECT_EQ(print(), before);
  EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(
      builder, site(), condition, computedBranch, plainBranch)));
  EXPECT_EQ(print(), before);
  EXPECT_TRUE(mlir::failed(lowerExactIntegerSelect(
      builder, site(), condition, plainBranch, computedBranch)));
  EXPECT_EQ(print(), before);
  EXPECT_EQ(body->getArgument(2).getType(), computedEight);
}

TEST_F(ExactIntegerLoweringTest, RightShiftWidthCountAndEndpointMatrix) {
  struct Case {
    unsigned width;
    const char *upper, *maximum, *halfUpper, *halfMaximum;
  };
  const Case cases[] = {
      {1, "2", "1", "1", "0"},
      {13, "8192", "8191", "4096", "4095"},
      {40, "1099511627776", "1099511627775", "549755813888", "549755813887"},
      {65, "36893488147419103232", "36893488147419103231",
       "18446744073709551616", "18446744073709551615"}};
  // 2^300 exceeds both native count capacity and the old 256-bit test parser.
  const char *huge = "203703597633448608626844568840937816105146839366593625063"
                     "6140449354381299763336706183397376";
  EXPECT_GT(integer(huge).getActiveBits(), 256u);
  for (const auto &c : cases) {
    auto argument = input(c.width, "0", c.upper);
    const std::string counts[] = {"0",
                                  "1",
                                  std::to_string(c.width - 1),
                                  std::to_string(c.width),
                                  std::to_string(c.width + 1),
                                  huge};
    for (const auto &count : counts) {
      SCOPED_TRACE(std::to_string(c.width) + " >> " + count);
      auto shifted = lowerExactIntegerShiftRight(builder, site(), argument,
                                                 integer(count));
      ASSERT_TRUE(mlir::succeeded(shifted));
      bool overshift = count == huge || std::stoul(count) >= c.width;
      unsigned amount = overshift ? 0 : std::stoul(count);
      EXPECT_EQ(width(shifted->value), overshift ? 1u : c.width - amount);
      const char *expectedUpper = overshift     ? "1"
                                  : amount == 0 ? c.upper
                                  : amount == 1 ? c.halfUpper
                                                : "2";
      const char *expectedMaximum = overshift     ? "0"
                                    : amount == 0 ? c.maximum
                                    : amount == 1 ? c.halfMaximum
                                                  : "1";
      interval(*shifted, "0", expectedUpper);
      expectValue(*shifted,
                  {{argument.value, integer(c.maximum).zextOrTrunc(c.width)}},
                  expectedMaximum);
      expectValue(*shifted, {{argument.value, llvm::APInt(c.width, 0)}}, "0");
      if (overshift) {
        EXPECT_TRUE(
            mlir::isa<ac::BitsConstantOp>(shifted->value.getDefiningOp()));
      } else {
        auto extract = shifted->value.getDefiningOp<ac::BitsExtractOp>();
        ASSERT_TRUE(extract);
        EXPECT_EQ(extract.getInput(), argument.value);
        EXPECT_NE(shifted->value, argument.value);
        if (amount == 0)
          EXPECT_EQ(shifted->value.getType(), argument.value.getType());
      }
    }
  }
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       RightShiftNonzeroEndpointsUseInclusiveMaximum) {
  auto argument = input(13, "17", "1025");
  auto shifted =
      lowerExactIntegerShiftRight(builder, site(), argument, integer("4"));
  ASSERT_TRUE(mlir::succeeded(shifted));
  interval(*shifted, "1", "65");
  EXPECT_EQ(width(shifted->value), 9u);
  expectValue(*shifted, {{argument.value, llvm::APInt(13, 17)}}, "1");
  expectValue(*shifted, {{argument.value, llvm::APInt(13, 32)}}, "2");
  expectValue(*shifted, {{argument.value, llvm::APInt(13, 1024)}}, "64");
  auto once =
      lowerExactIntegerShiftRight(builder, site(), argument, integer("1"));
  ASSERT_TRUE(mlir::succeeded(once));
  interval(*once, "8", "513");
  EXPECT_EQ(width(once->value), 12u);
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       RightShiftRetainsActualWideSingletonRepresentation) {
  auto argument = input(65, "0", "1");
  for (const char *count : {"0", "1", "64", "65"}) {
    auto shifted =
        lowerExactIntegerShiftRight(builder, site(), argument, integer(count));
    ASSERT_TRUE(mlir::succeeded(shifted));
    interval(*shifted, "0", "1");
    unsigned amount = std::stoul(count);
    EXPECT_EQ(width(shifted->value), amount >= 65 ? 1u : 65u - amount);
    if (amount < 65) {
      EXPECT_TRUE(mlir::isa<ac::BitsExtractOp>(shifted->value.getDefiningOp()));
      EXPECT_NE(shifted->value, argument.value);
    } else {
      EXPECT_TRUE(
          mlir::isa<ac::BitsConstantOp>(shifted->value.getDefiningOp()));
    }
    expectValue(*shifted, {{argument.value, llvm::APInt(65, 0)}}, "0");
  }
  verifyOperations();
}

TEST_F(ExactIntegerLoweringTest,
       RightShiftZeroPreservesComputedTypeWithFreshSSA) {
  context.loadDialect<mlir::func::FuncDialect>();
  auto computed = ac::StaticExprAttr::get(
      &context,
      builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
           builder.getNamedAttr("operator", builder.getStringAttr("add")),
           builder.getNamedAttr("lhs", literal("6")),
           builder.getNamedAttr("rhs", literal("7")),
           builder.getNamedAttr("origin", origin),
           builder.getNamedAttr("location", span)}));
  auto computedType = ac::BitsType::get(&context, computed);
  auto module = mlir::ModuleOp::create(builder.getUnknownLoc());
  mlir::OwningOpRef<mlir::ModuleOp> owned(module);
  builder.setInsertionPointToStart(module.getBody());
  auto function = builder.create<mlir::func::FuncOp>(
      builder.getUnknownLoc(), "numeric.Test",
      builder.getFunctionType({computedType}, {}));
  auto *body = function.addEntryBlock();
  builder.setInsertionPointToEnd(body);
  builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
  builder.setInsertionPointToStart(body);
  NumericValue argument{body->getArgument(0), ValueKind::Integer,
                        IntegerInterval{integer("17"), integer("1025")}};
  auto shifted =
      lowerExactIntegerShiftRight(builder, site(), argument, integer("0"));
  ASSERT_TRUE(mlir::succeeded(shifted));
  EXPECT_NE(shifted->value, argument.value);
  EXPECT_EQ(shifted->value.getType(), computedType);
  EXPECT_EQ(argument.value.getType(), computedType);
  ASSERT_TRUE(shifted->value.getDefiningOp<ac::BitsExtractOp>());
  interval(*shifted, "17", "1025");
  EXPECT_TRUE(mlir::succeeded(mlir::verify(module)));
}

TEST_F(ExactIntegerLoweringTest, RightShiftInvalidProofsFailBeforeMutation) {
  auto argument = input(13, "0", "8192");
  auto boolean = argument;
  boolean.sourceKind = ValueKind::Boolean;
  auto unknown = argument;
  unknown.sourceKind.reset();
  auto unproven = argument;
  unproven.interval.reset();
  auto null = argument;
  null.value = {};
  auto nonBits = argument;
  nonBits.value =
      block.addArgument(builder.getI1Type(), builder.getUnknownLoc());
  auto empty = input(13, "2", "2");
  auto reversed = input(13, "3", "2");
  auto negative = input(13, "-1", "2");
  auto oversized = input(13, "0", "8193");
  auto zero = input(65, "0", "1");
  auto reference = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("parameter")),
       builder.getNamedAttr(
           "owner", mlir::FlatSymbolRefAttr::get(&context, "numeric.Test")),
       builder.getNamedAttr("name", builder.getStringAttr("WIDTH"))});
  auto unresolvedType = ac::BitsType::get(
      &context,
      ac::StaticExprAttr::get(
          &context,
          builder.getDictionaryAttr(
              {builder.getNamedAttr("kind", builder.getStringAttr("reference")),
               builder.getNamedAttr("ref", reference),
               builder.getNamedAttr("origin", origin),
               builder.getNamedAttr("location", span)})));
  auto unresolved = argument;
  unresolved.value = block.addArgument(unresolvedType, builder.getUnknownLoc());
  auto capacity = argument;
  capacity.value =
      block.addArgument(ac::BitsType::get(&context, literal("4294967296")),
                        builder.getUnknownLoc());
  auto computedZero = ac::StaticExprAttr::get(
      &context,
      builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
           builder.getNamedAttr("operator", builder.getStringAttr("add")),
           builder.getNamedAttr("lhs", literal("0")),
           builder.getNamedAttr("rhs", literal("0")),
           builder.getNamedAttr("origin", origin),
           builder.getNamedAttr("location", span)}));
  auto zeroWidth = argument;
  zeroWidth.value = block.addArgument(ac::BitsType::get(&context, computedZero),
                                      builder.getUnknownLoc());
  auto original = argument.value;
  auto originalType = original.getType();
  auto before = snapshot();
  mlir::ScopedDiagnosticHandler ignore(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  for (const auto &invalid :
       {boolean, unknown, unproven, null, nonBits, empty, reversed, negative,
        oversized, unresolved, capacity, zeroWidth}) {
    auto inputSSA = invalid.value;
    auto inputType = inputSSA ? inputSSA.getType() : mlir::Type{};
    EXPECT_TRUE(mlir::failed(
        lowerExactIntegerShiftRight(builder, site(), invalid, integer("1"))));
    EXPECT_EQ(invalid.value, inputSSA);
    EXPECT_EQ(invalid.value ? invalid.value.getType() : mlir::Type{},
              inputType);
    EXPECT_EQ(argument.value, original);
    EXPECT_EQ(argument.value.getType(), originalType);
    EXPECT_EQ(snapshot(), before);
  }
  for (const auto &value : {argument, zero}) {
    auto inputSSA = value.value;
    auto inputType = inputSSA ? inputSSA.getType() : mlir::Type{};
    EXPECT_TRUE(mlir::failed(
        lowerExactIntegerShiftRight(builder, site(), value, integer("-1"))));
    EXPECT_EQ(value.value, inputSSA);
    EXPECT_EQ(value.value ? value.value.getType() : mlir::Type{}, inputType);
    EXPECT_EQ(argument.value, original);
    EXPECT_EQ(argument.value.getType(), originalType);
    EXPECT_EQ(snapshot(), before);
  }
  auto invalidOrigin = site();
  invalidOrigin.origin = {};
  auto invalidSpan = site();
  invalidSpan.sourceSpan = {};
  for (const auto &invalidSite : {invalidOrigin, invalidSpan}) {
    auto inputSSA = argument.value;
    auto inputType = inputSSA ? inputSSA.getType() : mlir::Type{};
    EXPECT_TRUE(mlir::failed(lowerExactIntegerShiftRight(
        builder, invalidSite, argument, integer("0"))));
    EXPECT_EQ(argument.value, inputSSA);
    EXPECT_EQ(argument.value ? argument.value.getType() : mlir::Type{},
              inputType);
    EXPECT_EQ(argument.value, original);
    EXPECT_EQ(argument.value.getType(), originalType);
    EXPECT_EQ(snapshot(), before);
  }
  EXPECT_EQ(argument.value, original);
  EXPECT_EQ(argument.value.getType(), originalType);
}
} // namespace
