#include "Compiler/FixedUnsignedDivRemLowering.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
using namespace acir::compiler;
using ac::detail::ValueKind;
using Result = FixedUnsignedDivRemResult;

llvm::APSInt integer(llvm::StringRef text) {
  bool negative = text.consume_front("-");
  llvm::APInt value(std::max(256u, unsigned(text.size() * 4 + 2)), text, 10);
  return llvm::APSInt(negative ? -value : value, false);
}

std::string print(mlir::Operation *operation) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  operation->print(stream);
  return text;
}

bool dependsOn(mlir::Value value, mlir::Value input) {
  if (value == input)
    return true;
  auto *operation = value.getDefiningOp();
  if (!operation)
    return false;
  return llvm::any_of(operation->getOperands(), [&](mlir::Value operand) {
    return dependsOn(operand, input);
  });
}

bool hasInputDependentMultiply(mlir::Value value, mlir::Value input) {
  auto *operation = value.getDefiningOp();
  if (!operation)
    return false;
  if (auto binary = mlir::dyn_cast<ac::BitsBinaryOp>(operation))
    if (binary.getOpcode() == "mul" && dependsOn(value, input))
      return true;
  return llvm::any_of(operation->getOperands(), [&](mlir::Value operand) {
    return hasInputDependentMultiply(operand, input);
  });
}

class FixedUnsignedDivRemLoweringTest : public ::testing::Test {
protected:
  FixedUnsignedDivRemLoweringTest() : builder(&context) {
    context.loadDialect<ac::ACIRDialect, mlir::func::FuncDialect>();
    auto occurrenceSite = builder.getDictionaryAttr(
        {builder.getNamedAttr("definition", mlir::FlatSymbolRefAttr::get(
                                                &context, "divrem.Test")),
         builder.getNamedAttr("ast_path", builder.getArrayAttr({}))});
    origin = builder.getDictionaryAttr(
        {builder.getNamedAttr("site", occurrenceSite),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
    span = builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("divrem.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
  }

  NumericLoweringSite site() { return {builder.getUnknownLoc(), origin, span}; }

  ac::StaticExprAttr literal(const llvm::APSInt &value) {
    return ac::StaticExprAttr::get(
        &context,
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
             builder.getNamedAttr(
                 "value",
                 builder.getDictionaryAttr(
                     {builder.getNamedAttr("kind",
                                           builder.getStringAttr("integer")),
                      builder.getNamedAttr(
                          "value", ac::MathIntAttr::get(&context, value))})),
             builder.getNamedAttr("origin", origin),
             builder.getNamedAttr("location", span)}));
  }
  ac::StaticExprAttr literal(llvm::StringRef text) {
    return literal(integer(text));
  }
  ac::BitsType type(uint64_t width) {
    return ac::BitsType::get(&context, literal(std::to_string(width)));
  }
  ac::StaticExprAttr computedWidth(llvm::StringRef lhs, llvm::StringRef rhs) {
    return ac::StaticExprAttr::get(
        &context,
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
             builder.getNamedAttr("operator", builder.getStringAttr("add")),
             builder.getNamedAttr("lhs", literal(lhs)),
             builder.getNamedAttr("rhs", literal(rhs)),
             builder.getNamedAttr("origin", origin),
             builder.getNamedAttr("location", span)}));
  }
  ac::BitsType unresolvedType() {
    auto reference = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("parameter")),
         builder.getNamedAttr(
             "owner", mlir::FlatSymbolRefAttr::get(&context, "divrem.Test")),
         builder.getNamedAttr("name", builder.getStringAttr("WIDTH"))});
    return ac::BitsType::get(
        &context,
        ac::StaticExprAttr::get(
            &context, builder.getDictionaryAttr(
                          {builder.getNamedAttr(
                               "kind", builder.getStringAttr("reference")),
                           builder.getNamedAttr("ref", reference),
                           builder.getNamedAttr("origin", origin),
                           builder.getNamedAttr("location", span)})));
  }

  mlir::OwningOpRef<mlir::ModuleOp> package(mlir::Type inputType) {
    auto module = mlir::ModuleOp::create(builder.getUnknownLoc());
    builder.setInsertionPointToStart(module.getBody());
    auto function = mlir::func::FuncOp::create(
        builder, builder.getUnknownLoc(), "divrem.Test",
        builder.getFunctionType({inputType}, {}));
    auto *block = function.addEntryBlock();
    builder.setInsertionPointToEnd(block);
    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc());
    builder.setInsertionPointToStart(block);
    return mlir::OwningOpRef<mlir::ModuleOp>(module);
  }
  NumericValue input(mlir::ModuleOp module) {
    auto function = *module.getOps<mlir::func::FuncOp>().begin();
    // The singleton mathematical interval must not narrow the physical input.
    return {function.getArgument(0), ValueKind::Integer,
            IntegerInterval{integer("0"), integer("1")}};
  }
  unsigned width(mlir::Value value, ac::HardwareAnalysis &analysis) {
    auto resolved = analysis.getPackedWidth(
        value.getType(), {}, builder.getInsertionBlock()->getParentOp());
    EXPECT_TRUE(mlir::succeeded(resolved));
    return mlir::succeeded(resolved) ? static_cast<unsigned>(*resolved) : 1;
  }
  llvm::APSInt constant(ac::StaticExprAttr expression) {
    return llvm::APSInt(expression.getTree()
                            .getAs<mlir::DictionaryAttr>("value")
                            .getAs<ac::MathIntAttr>("value")
                            .getCanonicalValue());
  }

  // Interpret existing bit operations only. Goldens use APInt udiv/urem,
  // independently of the helper's reciprocal calculation and geometry.
  llvm::APInt evaluate(mlir::Value value, mlir::Value inputValue,
                       const llvm::APInt &sample,
                       ac::HardwareAnalysis &analysis) {
    if (value == inputValue)
      return sample;
    unsigned resultWidth = width(value, analysis);
    auto *op = value.getDefiningOp();
    if (auto c = mlir::dyn_cast_or_null<ac::BitsConstantOp>(op))
      return constant(c.getValue()).zextOrTrunc(resultWidth);
    if (auto resize = mlir::dyn_cast_or_null<ac::BitsResizeOp>(op)) {
      EXPECT_EQ(resize.getMode(), "zext");
      return evaluate(resize.getInput(), inputValue, sample, analysis)
          .zext(resultWidth);
    }
    if (auto extract = mlir::dyn_cast_or_null<ac::BitsExtractOp>(op))
      return evaluate(extract.getInput(), inputValue, sample, analysis)
          .extractBits(resultWidth, constant(extract.getLow()).getZExtValue());
    if (auto binary = mlir::dyn_cast_or_null<ac::BitsBinaryOp>(op)) {
      auto lhs = evaluate(binary.getLhs(), inputValue, sample, analysis);
      auto rhs = evaluate(binary.getRhs(), inputValue, sample, analysis);
      if (binary.getOpcode() == "mul")
        return lhs * rhs;
      if (binary.getOpcode() == "sub")
        return lhs - rhs;
    }
    ADD_FAILURE() << "Unexpected operation in fixed unsigned div/rem SSA";
    return llvm::APInt(resultWidth, 0);
  }

  void verifyOperations(mlir::ModuleOp module, ac::HardwareAnalysis &analysis) {
    EXPECT_TRUE(mlir::succeeded(mlir::verify(module)));
    module.walk([&](mlir::Operation *operation) {
      if (operation->getName().getDialectNamespace() == "ac")
        EXPECT_TRUE(
            mlir::succeeded(analysis.verifyResolvedOperation(operation, {})));
    });
  }

  void checkArithmetic(unsigned inputWidth, const llvm::APInt &divisor,
                       const std::vector<llvm::APInt> &samples) {
    auto module = package(type(inputWidth));
    ac::HardwareAnalysis analysis(*module);
    auto numerator = input(*module);
    auto q =
        lowerFixedUnsignedDivRem(builder, analysis, site(), numerator, true,
                                 llvm::APSInt(divisor, true), Result::Quotient);
    auto r = lowerFixedUnsignedDivRem(builder, analysis, site(), numerator,
                                      true, llvm::APSInt(divisor, true),
                                      Result::Remainder);
    ASSERT_TRUE(mlir::succeeded(q));
    ASSERT_TRUE(mlir::succeeded(r));
    EXPECT_EQ(q->getType(), numerator.value.getType());
    EXPECT_EQ(r->getType(), numerator.value.getType());
    verifyOperations(*module, analysis);
    for (const auto &sample : samples) {
      auto quotient = evaluate(*q, numerator.value, sample, analysis);
      auto remainder = evaluate(*r, numerator.value, sample, analysis);
      EXPECT_EQ(quotient, sample.udiv(divisor));
      EXPECT_EQ(remainder, sample.urem(divisor));
      EXPECT_EQ(quotient * divisor + remainder, sample);
      EXPECT_TRUE(remainder.ult(divisor));
    }
    EXPECT_EQ(numerator.sourceKind, ValueKind::Integer);
    ASSERT_TRUE(numerator.interval);
    EXPECT_EQ(numerator.interval->lower, integer("0"));
    EXPECT_EQ(numerator.interval->upper, integer("1"));
  }

  void expectFailureUnchanged(mlir::ModuleOp module,
                              ac::HardwareAnalysis &analysis,
                              const NumericLoweringSite &loweringSite,
                              const NumericValue &numerator, bool authority,
                              const llvm::APSInt &divisor,
                              llvm::StringRef expectedDiagnostic,
                              Result result = Result::Quotient) {
    const auto before = print(module);
    size_t operationCount = 0;
    module.walk([&](mlir::Operation *) { ++operationCount; });
    auto originalValue = numerator.value;
    auto originalType = originalValue ? originalValue.getType() : mlir::Type{};
    auto originalKind = numerator.sourceKind;
    auto originalInterval = numerator.interval;
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler handler(
        loweringSite.location.getContext(), [&](mlir::Diagnostic &diagnostic) {
          llvm::raw_string_ostream stream(diagnostics);
          diagnostic.print(stream);
          return mlir::success();
        });
    EXPECT_TRUE(mlir::failed(
        lowerFixedUnsignedDivRem(builder, analysis, loweringSite, numerator,
                                 authority, divisor, result)));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_NE(diagnostics.find(expectedDiagnostic.str()), std::string::npos)
        << diagnostics;
    EXPECT_EQ(print(module), before);
    size_t afterCount = 0;
    module.walk([&](mlir::Operation *) { ++afterCount; });
    EXPECT_EQ(afterCount, operationCount);
    EXPECT_EQ(numerator.value, originalValue);
    EXPECT_EQ(numerator.value ? numerator.value.getType() : mlir::Type{},
              originalType);
    EXPECT_EQ(numerator.sourceKind, originalKind);
    ASSERT_EQ(numerator.interval.has_value(), originalInterval.has_value());
    if (originalInterval) {
      EXPECT_EQ(numerator.interval->lower, originalInterval->lower);
      EXPECT_EQ(numerator.interval->upper, originalInterval->upper);
    }
  }

  mlir::MLIRContext context;
  mlir::OpBuilder builder;
  mlir::DictionaryAttr origin, span;
};

TEST_F(FixedUnsignedDivRemLoweringTest, ExhaustiveSmallUnsignedArithmetic) {
  for (unsigned w = 1; w <= 6; ++w) {
    std::vector<llvm::APInt> samples;
    for (unsigned a = 0; a < (1u << w); ++a)
      samples.emplace_back(w, a);
    for (unsigned d = 1; d < (1u << w); ++d) {
      SCOPED_TRACE("width=" + std::to_string(w) +
                   " divisor=" + std::to_string(d));
      checkArithmetic(w, llvm::APInt(w, d), samples);
    }
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest, WideUnsignedArithmeticBeyondHostWidth) {
  for (unsigned w : {65u, 130u, 257u}) {
    auto maximum = llvm::APInt::getAllOnes(w);
    auto high = llvm::APInt::getOneBitSet(w, w - 1);
    std::vector<llvm::APInt> samples = {
        llvm::APInt(w, 0), llvm::APInt(w, 1), llvm::APInt(w, 2),
        llvm::APInt(w, 9), high - 1,          high,
        high + 1,          maximum - 1,       maximum};
    std::vector<llvm::APInt> divisors = {llvm::APInt(w, 1),
                                         llvm::APInt(w, 2),
                                         llvm::APInt(w, 3),
                                         llvm::APInt(w, 5),
                                         llvm::APInt(w, 7),
                                         llvm::APInt(w, 10),
                                         llvm::APInt::getOneBitSet(w, 64),
                                         high,
                                         high + 1,
                                         maximum};
    for (const auto &d : divisors) {
      llvm::SmallString<96> text;
      d.toString(text, 10, false);
      SCOPED_TRACE("width=" + std::to_string(w) +
                   " divisor=" + text.str().str());
      checkArithmetic(w, d, samples);
    }
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest,
       PreservesComputedInputTypeAndMissingInterval) {
  auto computed = ac::BitsType::get(&context, computedWidth("6", "7"));
  auto module = package(computed);
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  numerator.interval.reset();
  numerator.sourceKind.reset();
  for (Result result : {Result::Quotient, Result::Remainder}) {
    auto lowered = lowerFixedUnsignedDivRem(
        builder, analysis, site(), numerator, true, integer("10"), result);
    ASSERT_TRUE(mlir::succeeded(lowered));
    EXPECT_EQ(lowered->getType(), computed);
    EXPECT_EQ(numerator.value.getType(), computed);
    auto sample = llvm::APInt(13, 8191);
    EXPECT_EQ(evaluate(*lowered, numerator.value, sample, analysis),
              result == Result::Quotient ? sample.udiv(llvm::APInt(13, 10))
                                         : sample.urem(llvm::APInt(13, 10)));
  }
  verifyOperations(*module, analysis);
  EXPECT_FALSE(numerator.interval);
  EXPECT_FALSE(numerator.sourceKind);
}

TEST_F(FixedUnsignedDivRemLoweringTest,
       UnitAndPowerOfTwoDivisorsRetainInputDependentMultiply) {
  for (const char *divisor : {"1", "2", "16"}) {
    auto module = package(type(13));
    ac::HardwareAnalysis analysis(*module);
    auto numerator = input(*module);
    for (Result result : {Result::Quotient, Result::Remainder}) {
      auto lowered = lowerFixedUnsignedDivRem(
          builder, analysis, site(), numerator, true, integer(divisor), result);
      ASSERT_TRUE(mlir::succeeded(lowered));
      EXPECT_TRUE(dependsOn(*lowered, numerator.value));
      // Existing MUL makes X/Z arithmetic observable even when %1 is always
      // zero for known inputs. Identity/mask/constant rewrites lose this fact.
      EXPECT_TRUE(hasInputDependentMultiply(*lowered, numerator.value));
    }
    verifyOperations(*module, analysis);
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest,
       RejectsAbsentBooleanWrongTypeAndFalseAuthority) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  const char *guard = "requires authoritative unsigned bits";
  expectFailureUnchanged(*module, analysis, site(), numerator, false,
                         integer("3"), guard);
  auto boolean = numerator;
  boolean.sourceKind = ValueKind::Boolean;
  expectFailureUnchanged(*module, analysis, site(), boolean, true, integer("3"),
                         guard);
  auto absent = numerator;
  absent.value = {};
  expectFailureUnchanged(*module, analysis, site(), absent, true, integer("3"),
                         guard);
  auto wrongModule = package(builder.getI8Type());
  ac::HardwareAnalysis wrongAnalysis(*wrongModule);
  expectFailureUnchanged(*wrongModule, wrongAnalysis, site(),
                         input(*wrongModule), true, integer("3"), guard);
}

TEST_F(FixedUnsignedDivRemLoweringTest, RejectsDivisorsEvenForKnownZeroInput) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  mlir::OperationState state(builder.getUnknownLoc(),
                             ac::BitsConstantOp::getOperationName());
  state.addTypes(type(8));
  state.addAttribute("value", literal("0"));
  numerator.value = builder.create(state)->getResult(0);
  for (Result result : {Result::Quotient, Result::Remainder}) {
    for (const char *d : {"0", "-1"})
      expectFailureUnchanged(*module, analysis, site(), numerator, true,
                             integer(d), "divisor must be positive", result);
    for (const auto &d :
         {integer("256"),
          llvm::APSInt(llvm::APInt::getOneBitSet(513, 512), true)})
      expectFailureUnchanged(*module, analysis, site(), numerator, true, d,
                             "divisor must fit input width", result);
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest, RejectsMalformedOccurrenceAndSpan) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  auto malformed = site();
  malformed.origin = builder.getDictionaryAttr({});
  expectFailureUnchanged(*module, analysis, malformed, numerator, true,
                         integer("3"), "Occurrence");
  malformed.origin = builder.getDictionaryAttr(
      {builder.getNamedAttr("site", builder.getStringAttr("not-a-site")),
       builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  expectFailureUnchanged(*module, analysis, malformed, numerator, true,
                         integer("3"), "Occurrence");
  malformed = site();
  malformed.sourceSpan = builder.getDictionaryAttr({});
  expectFailureUnchanged(*module, analysis, malformed, numerator, true,
                         integer("3"), "SourceSpan");
  llvm::SmallVector<mlir::NamedAttribute> fields(span.begin(), span.end());
  for (auto &field : fields)
    if (field.getName() == "column")
      field = builder.getNamedAttr("column", builder.getI64IntegerAttr(0));
  malformed.sourceSpan = builder.getDictionaryAttr(fields);
  expectFailureUnchanged(*module, analysis, malformed, numerator, true,
                         integer("3"),
                         "SourceSpan coordinates must be one-based");
  for (bool missingOrigin : {false, true}) {
    malformed = site();
    if (missingOrigin)
      malformed.origin = {};
    else
      malformed.sourceSpan = {};
    expectFailureUnchanged(*module, analysis, malformed, numerator, true,
                           integer("3"),
                           missingOrigin ? "Occurrence" : "SourceSpan");
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest,
       RejectsInvalidInsertionPointAndPackage) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  auto *block = builder.getInsertionBlock();
  builder.clearInsertionPoint();
  expectFailureUnchanged(*module, analysis, site(), numerator, true,
                         integer("3"), "requires a valid insertion point");
  mlir::Block detached;
  builder.setInsertionPointToEnd(&detached);
  expectFailureUnchanged(*module, analysis, site(), numerator, true,
                         integer("3"), "requires a valid insertion point");
  EXPECT_TRUE(detached.empty());
  auto other = package(type(8));
  auto otherBefore = print(other->getOperation());
  expectFailureUnchanged(*module, analysis, site(), numerator, true,
                         integer("3"), "requires a valid insertion point");
  EXPECT_EQ(print(other->getOperation()), otherBefore);
  builder.setInsertionPointToStart(block);
  expectFailureUnchanged(*module, analysis, site(), input(*other), true,
                         integer("3"), "context mismatch");
  EXPECT_EQ(print(other->getOperation()), otherBefore);
  ac::HardwareAnalysis absentAnalysis(mlir::ModuleOp{});
  expectFailureUnchanged(*module, absentAnalysis, site(), numerator, true,
                         integer("3"), "context mismatch");
}

TEST_F(FixedUnsignedDivRemLoweringTest, RejectsForeignContextBeforeMutation) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  auto numerator = input(*module);
  mlir::MLIRContext foreignContext;
  auto foreignSite = site();
  foreignSite.location = mlir::UnknownLoc::get(&foreignContext);
  expectFailureUnchanged(*module, analysis, foreignSite, numerator, true,
                         integer("3"), "context mismatch");
  foreignSite = site();
  foreignSite.origin = mlir::DictionaryAttr::get(&foreignContext);
  expectFailureUnchanged(*module, analysis, foreignSite, numerator, true,
                         integer("3"), "context mismatch");
  foreignSite = site();
  foreignSite.sourceSpan = mlir::DictionaryAttr::get(&foreignContext);
  expectFailureUnchanged(*module, analysis, foreignSite, numerator, true,
                         integer("3"), "context mismatch");
  mlir::OwningOpRef<mlir::ModuleOp> foreignPackage(
      mlir::ModuleOp::create(mlir::UnknownLoc::get(&foreignContext)));
  ac::HardwareAnalysis foreignAnalysis(*foreignPackage);
  expectFailureUnchanged(*module, foreignAnalysis, site(), numerator, true,
                         integer("3"), "context mismatch");
}

TEST_F(FixedUnsignedDivRemLoweringTest, RejectsUnresolvedAndNonpositiveWidths) {
  // Literal zero/negative BitsType construction is rejected by the type
  // invariant itself. Computed widths reach the helper's shared resolution.
  for (auto inputType :
       {unresolvedType(), ac::BitsType::get(&context, computedWidth("0", "0")),
        ac::BitsType::get(&context, computedWidth("-1", "0"))}) {
    auto module = package(inputType);
    ac::HardwareAnalysis analysis(*module);
    expectFailureUnchanged(*module, analysis, site(), input(*module), true,
                           integer("1"), "");
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest,
       RejectsGeometryOverflowBeforeAPIntAllocation) {
  // All values are declared type widths; no wide APInt numerator is allocated.
  // The divisor-one case must check P=2W, rather than allocate 2^K first.
  constexpr uint64_t capacity = std::numeric_limits<unsigned>::max();
  for (uint64_t w : {capacity / 2 + 1, capacity, capacity + 1,
                     std::numeric_limits<uint64_t>::max()}) {
    auto module = package(type(w));
    ac::HardwareAnalysis analysis(*module);
    expectFailureUnchanged(*module, analysis, site(), input(*module), true,
                           integer("1"),
                           "geometry exceeds compiler representation capacity");
  }
}

TEST_F(FixedUnsignedDivRemLoweringTest, RejectsInvalidRequestedResult) {
  auto module = package(type(8));
  ac::HardwareAnalysis analysis(*module);
  expectFailureUnchanged(*module, analysis, site(), input(*module), true,
                         integer("3"), "invalid result mode",
                         static_cast<Result>(2));
}
} // namespace
