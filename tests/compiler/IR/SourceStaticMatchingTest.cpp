#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <string>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;
using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

llvm::APSInt signedInteger(unsigned width, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  llvm::APInt bits(width, spelling, 10);
  if (negative)
    bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/false);
}

MathIntAttr math(mlir::MLIRContext &context, llvm::StringRef spelling) {
  unsigned width = static_cast<unsigned>(spelling.size() * 4 + 2);
  return MathIntAttr::get(&context, signedInteger(width, spelling));
}

mlir::IntegerAttr length(mlir::OpBuilder &builder, uint64_t value) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), 64), llvm::APInt(64, value));
}

mlir::DictionaryAttr staticBool(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")}});
}

mlir::DictionaryAttr staticInteger(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")}});
}

mlir::DictionaryAttr staticInteger(mlir::OpBuilder &builder,
                                   llvm::StringRef lower,
                                   llvm::StringRef upper) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"lower", math(*builder.getContext(), lower)},
                              {"upper", math(*builder.getContext(), upper)}});
}

mlir::DictionaryAttr staticList(mlir::OpBuilder &builder,
                                mlir::DictionaryAttr element, uint64_t size) {
  return dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"element", element},
                              {"length", length(builder, size)}});
}

mlir::DictionaryAttr boolValue(mlir::OpBuilder &builder, bool value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"value", builder.getBoolAttr(value)}});
}

mlir::DictionaryAttr integerValue(mlir::OpBuilder &builder,
                                  llvm::StringRef value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"value", math(*builder.getContext(), value)}});
}

mlir::DictionaryAttr listValue(mlir::OpBuilder &builder,
                               std::initializer_list<mlir::Attribute> values) {
  return dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"values", builder.getArrayAttr(values)}});
}

mlir::DictionaryAttr presentDefault(mlir::OpBuilder &builder,
                                    mlir::DictionaryAttr value) {
  return dictionary(builder,
                    {{"present", builder.getBoolAttr(true)}, {"value", value}});
}

struct CheckResult {
  bool passed;
  std::string diagnostic;
};

template <typename Callback>
CheckResult check(mlir::MLIRContext &context, Callback callback) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };
  return {mlir::succeeded(callback(emitError)), diagnostic};
}

void expectRejected(const CheckResult &result) {
  EXPECT_FALSE(result.passed);
  EXPECT_FALSE(result.diagnostic.empty());
}

class SourceStaticMatchingTest : public ::testing::Test {
protected:
  SourceStaticMatchingTest() : builder(&context) {
    context.loadDialect<ACIRDialect>();
  }

  CheckResult match(mlir::DictionaryAttr value, mlir::DictionaryAttr expected) {
    auto noRecords = [](mlir::FlatSymbolRefAttr)
        -> mlir::FailureOr<detail::ResolvedRecordView> {
      return mlir::failure();
    };
    return check(context, [&](EmitError emit) {
      return detail::verifyStaticValueMatchesType(
          value, expected, detail::ExpectedTypeKind::Static, noRecords, emit);
    });
  }

  CheckResult matchDefault(mlir::DictionaryAttr value,
                           mlir::DictionaryAttr expected) {
    auto noRecords = [](mlir::FlatSymbolRefAttr)
        -> mlir::FailureOr<detail::ResolvedRecordView> {
      return mlir::failure();
    };
    return check(context, [&](EmitError emit) {
      return detail::verifyDefaultMatchesType(
          value, expected, detail::ExpectedTypeKind::Static, noRecords, emit);
    });
  }

  mlir::MLIRContext context;
  mlir::OpBuilder builder;
};

TEST_F(SourceStaticMatchingTest,
       UnboundedIntegerAcceptsArbitraryPrecisionPositiveAndNegativeValues) {
  auto expected = staticInteger(builder);

  for (llvm::StringRef value : {
           "-123456789012345678901234567890123456789012345678901234567890",
           "0",
           "987654321098765432109876543210987654321098765432109876543210",
       }) {
    auto result = match(integerValue(builder, value), expected);
    EXPECT_TRUE(result.passed) << value.str() << ": " << result.diagnostic;
  }
}

TEST_F(SourceStaticMatchingTest,
       BoundedIntegerUsesInclusiveLowerAndExclusiveUpperAtAnyPrecision) {
  constexpr llvm::StringLiteral lower =
      "-12345678901234567890123456789012345678901234567890";
  constexpr llvm::StringLiteral upper =
      "12345678901234567890123456789012345678901234567891";
  auto expected = staticInteger(builder, lower, upper);

  EXPECT_TRUE(match(integerValue(builder, lower), expected).passed);
  EXPECT_TRUE(
      match(integerValue(builder,
                         "12345678901234567890123456789012345678901234567890"),
            expected)
          .passed);
  expectRejected(
      match(integerValue(builder,
                         "-12345678901234567890123456789012345678901234567891"),
            expected));
  expectRejected(match(integerValue(builder, upper), expected));

  auto positive =
      staticInteger(builder, "184467440737095516160000000000000000000",
                    "184467440737095516160000000000000000002");
  EXPECT_TRUE(
      match(integerValue(builder, "184467440737095516160000000000000000001"),
            positive)
          .passed);
  expectRejected(
      match(integerValue(builder, "184467440737095516160000000000000000002"),
            positive));
}

TEST_F(SourceStaticMatchingTest, BoolAndIntegerKindsNeverMatchImplicitly) {
  EXPECT_TRUE(match(boolValue(builder, true), staticBool(builder)).passed);
  EXPECT_TRUE(match(integerValue(builder, "1"), staticInteger(builder)).passed);
  expectRejected(match(boolValue(builder, true), staticInteger(builder)));
  expectRejected(match(integerValue(builder, "1"), staticBool(builder)));
}

TEST_F(SourceStaticMatchingTest,
       NestedStaticListsRequireExactLengthsAndMatchingElements) {
  auto expected = staticList(
      builder, staticList(builder, staticInteger(builder, "-2", "3"), 2), 2);
  auto valid =
      listValue(builder, {listValue(builder, {integerValue(builder, "-2"),
                                              integerValue(builder, "2")}),
                          listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "1")})});
  EXPECT_TRUE(match(valid, expected).passed);

  expectRejected(match(
      listValue(builder, {listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "1")})}),
      expected));
  expectRejected(match(
      listValue(builder, {listValue(builder, {integerValue(builder, "0")}),
                          listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "1")})}),
      expected));
  auto wrongElement = match(
      listValue(builder, {listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "3")}),
                          listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "1")})}),
      expected);
  expectRejected(wrongElement);
  EXPECT_NE(wrongElement.diagnostic.find("element[0]"), std::string::npos)
      << wrongElement.diagnostic;
  EXPECT_NE(wrongElement.diagnostic.find("element[1]"), std::string::npos)
      << wrongElement.diagnostic;
  expectRejected(match(
      listValue(builder, {listValue(builder, {boolValue(builder, true),
                                              integerValue(builder, "1")}),
                          listValue(builder, {integerValue(builder, "0"),
                                              integerValue(builder, "1")})}),
      expected));
}

TEST_F(SourceStaticMatchingTest,
       PresentValueAndListDefaultsUseStaticExpectedTypes) {
  auto bounded = staticInteger(builder, "10", "12");
  EXPECT_TRUE(matchDefault(presentDefault(builder, integerValue(builder, "10")),
                           bounded)
                  .passed);
  expectRejected(matchDefault(
      presentDefault(builder, integerValue(builder, "12")), bounded));
  expectRejected(
      matchDefault(presentDefault(builder, boolValue(builder, true)), bounded));

  auto listType = staticList(builder, staticInteger(builder), 2);
  EXPECT_TRUE(
      matchDefault(
          presentDefault(builder,
                         listValue(builder, {integerValue(builder, "-1"),
                                             integerValue(builder, "1")})),
          listType)
          .passed);
  expectRejected(matchDefault(
      presentDefault(builder, listValue(builder, {integerValue(builder, "1")})),
      listType));
}

} // namespace
} // namespace acir::ac
