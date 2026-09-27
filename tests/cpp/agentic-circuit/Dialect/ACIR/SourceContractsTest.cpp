#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;

mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

mlir::IntegerAttr integer(mlir::OpBuilder &builder, int64_t value) {
  return builder.getI64IntegerAttr(value);
}

mlir::IntegerAttr negativeSignedInteger(mlir::OpBuilder &builder) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), 64,
                             mlir::IntegerType::Signed),
      -1);
}

mlir::IntegerAttr maximumUnsignedInteger(mlir::OpBuilder &builder) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), 64,
                             mlir::IntegerType::Unsigned),
      llvm::APInt::getAllOnes(64));
}

mlir::IntegerAttr wideInteger(mlir::OpBuilder &builder,
                              mlir::IntegerType::SignednessSemantics signedness,
                              const llvm::APInt &value) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), 128, signedness), value);
}

mlir::DictionaryAttr sourceSpan(mlir::OpBuilder &builder, int64_t line = 1,
                                int64_t column = 1, int64_t endLine = 1,
                                int64_t endColumn = 2) {
  return dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                              {"line", integer(builder, line)},
                              {"column", integer(builder, column)},
                              {"end_line", integer(builder, endLine)},
                              {"end_column", integer(builder, endColumn)}});
}

mlir::DictionaryAttr fieldComponent(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("field")},
                              {"name", builder.getStringAttr("payload")}});
}

mlir::DictionaryAttr indexComponent(mlir::OpBuilder &builder,
                                    int64_t value = 0) {
  return dictionary(builder, {{"kind", builder.getStringAttr("index")},
                              {"value", integer(builder, value)}});
}

mlir::DictionaryAttr site(mlir::OpBuilder &builder, mlir::ArrayAttr path) {
  return dictionary(builder,
                    {{"definition", mlir::FlatSymbolRefAttr::get(
                                        builder.getContext(), "demo.Model")},
                     {"ast_path", path}});
}

template <typename Verifier>
bool verifies(mlir::MLIRContext &context, mlir::DictionaryAttr value,
              Verifier verifier) {
  mlir::ScopedDiagnosticHandler suppressExpectedDiagnostics(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };
  return mlir::succeeded(verifier(value, emitError));
}

llvm::APSInt signedInteger(unsigned width, llvm::StringRef value) {
  bool negative = value.consume_front("-");
  llvm::APInt bits(width, value, 10);
  if (negative)
    bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/false);
}

TEST(MathIntAttrTest, PreservesArbitraryPrecisionSignedDecimalValues) {
  mlir::MLIRContext context;
  context.loadDialect<ACIRDialect>();

  const auto zero = MathIntAttr::get(&context, signedInteger(1, "0"));
  const auto positive = MathIntAttr::get(
      &context,
      signedInteger(512, "12345678901234567890123456789012345678901234567890"));
  const auto negative = MathIntAttr::get(
      &context,
      signedInteger(512,
                    "-98765432109876543210987654321098765432109876543210"));

  EXPECT_EQ(zero.getCanonicalValue(), "0");
  EXPECT_EQ(positive.getCanonicalValue(),
            "12345678901234567890123456789012345678901234567890");
  EXPECT_EQ(negative.getCanonicalValue(),
            "-98765432109876543210987654321098765432109876543210");
}

TEST(MathIntAttrTest, MathematicalIdentityDoesNotDependOnHostBitWidth) {
  mlir::MLIRContext context;
  context.loadDialect<ACIRDialect>();

  EXPECT_EQ(MathIntAttr::get(&context, signedInteger(8, "-1")),
            MathIntAttr::get(&context, signedInteger(257, "-1")));
  EXPECT_EQ(MathIntAttr::get(&context, signedInteger(9, "255")),
            MathIntAttr::get(&context, signedInteger(512, "255")));
}

TEST(MathIntAttrTest, InternalParserRejectsNonCanonicalDecimalSpellings) {
  mlir::MLIRContext context;
  context.loadDialect<ACIRDialect>();
  mlir::ScopedDiagnosticHandler suppressExpectedDiagnostics(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };

  for (llvm::StringRef spelling :
       {"", "+1", "00", "01", "-0", "-01", "--1", "1x"})
    EXPECT_TRUE(
        mlir::failed(detail::parseMathIntAttr(&context, spelling, emitError)))
        << spelling.str();
}

TEST(MathIntAttrTest, AttributeAndTypeParsePrintRoundTrip) {
  mlir::MLIRContext context;
  context.loadDialect<ACIRDialect>();

  auto attribute = mlir::parseAttribute(
      "#ac.math_int<-123456789012345678901234567890>", &context);
  ASSERT_TRUE(attribute);
  ASSERT_TRUE(mlir::isa<MathIntAttr>(attribute));
  std::string attributeText;
  llvm::raw_string_ostream(attributeText) << attribute;
  EXPECT_EQ(attributeText, "#ac.math_int<-123456789012345678901234567890>");
  EXPECT_EQ(attribute, mlir::parseAttribute(attributeText, &context));

  auto type = mlir::parseType("!ac.math_int", &context);
  ASSERT_TRUE(type);
  ASSERT_TRUE(mlir::isa<MathIntType>(type));
  std::string typeText;
  llvm::raw_string_ostream(typeText) << type;
  EXPECT_EQ(typeText, "!ac.math_int");
  EXPECT_EQ(type, mlir::parseType(typeText, &context));
}

TEST(SourceSpanContractTest, AcceptsPositiveOneBasedOrderedCoordinates) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);

  EXPECT_TRUE(verifies(context, sourceSpan(builder, 1, 1, 1, 1),
                       detail::verifySourceSpan));
  EXPECT_TRUE(verifies(context, sourceSpan(builder, 7, 3, 9, 2),
                       detail::verifySourceSpan));
  auto maximumU64 = maximumUnsignedInteger(builder);
  ASSERT_TRUE(mlir::cast<mlir::IntegerType>(maximumU64.getType()).isUnsigned());
  EXPECT_TRUE(verifies(
      context,
      dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                           {"line", maximumU64},
                           {"column", integer(builder, 1)},
                           {"end_line", maximumU64},
                           {"end_column", integer(builder, 1)}}),
      detail::verifySourceSpan));
  auto wideSigned =
      wideInteger(builder, mlir::IntegerType::Signed, llvm::APInt(128, 7));
  auto wideUnsigned =
      wideInteger(builder, mlir::IntegerType::Unsigned, llvm::APInt(128, 7));
  ASSERT_TRUE(mlir::cast<mlir::IntegerType>(wideSigned.getType()).isSigned());
  ASSERT_TRUE(
      mlir::cast<mlir::IntegerType>(wideUnsigned.getType()).isUnsigned());
  for (mlir::Attribute wideSmall : {
           mlir::Attribute(wideSigned),
           mlir::Attribute(wideUnsigned),
       })
    EXPECT_TRUE(verifies(
        context,
        dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                             {"line", wideSmall},
                             {"column", integer(builder, 1)},
                             {"end_line", wideSmall},
                             {"end_column", integer(builder, 1)}}),
        detail::verifySourceSpan));
}

TEST(SourceSpanContractTest, RejectsMissingUnknownAndWrongKindFields) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);
  auto valid = sourceSpan(builder);

  EXPECT_FALSE(verifies(
      context,
      dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                           {"line", integer(builder, 1)},
                           {"column", integer(builder, 1)},
                           {"end_line", integer(builder, 1)}}),
      detail::verifySourceSpan));
  EXPECT_FALSE(verifies(
      context,
      dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                           {"line", integer(builder, 1)},
                           {"column", integer(builder, 1)},
                           {"end_line", integer(builder, 1)},
                           {"end_column", integer(builder, 2)},
                           {"unknown", builder.getUnitAttr()}}),
      detail::verifySourceSpan));
  EXPECT_FALSE(
      verifies(context,
               dictionary(builder, {{"path", integer(builder, 1)},
                                    {"line", integer(builder, 1)},
                                    {"column", integer(builder, 1)},
                                    {"end_line", integer(builder, 1)},
                                    {"end_column", integer(builder, 2)}}),
               detail::verifySourceSpan));
  EXPECT_TRUE(verifies(context, valid, detail::verifySourceSpan));
}

TEST(SourceSpanContractTest, RejectsBoolNonPositiveAndReversedCoordinates) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);

  EXPECT_FALSE(verifies(
      context,
      dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                           {"line", builder.getBoolAttr(true)},
                           {"column", integer(builder, 1)},
                           {"end_line", integer(builder, 1)},
                           {"end_column", integer(builder, 2)}}),
      detail::verifySourceSpan));
  for (mlir::Attribute negative :
       {mlir::Attribute(integer(builder, -1)),
        mlir::Attribute(negativeSignedInteger(builder))})
    EXPECT_FALSE(verifies(
        context,
        dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                             {"line", negative},
                             {"column", integer(builder, 1)},
                             {"end_line", integer(builder, 1)},
                             {"end_column", integer(builder, 2)}}),
        detail::verifySourceSpan));
  const llvm::APInt tooLarge = llvm::APInt::getOneBitSet(128, 64);
  for (mlir::Attribute wideTooLarge : {
           mlir::Attribute(
               wideInteger(builder, mlir::IntegerType::Signed, tooLarge)),
           mlir::Attribute(
               wideInteger(builder, mlir::IntegerType::Unsigned, tooLarge)),
       })
    EXPECT_FALSE(verifies(
        context,
        dictionary(builder, {{"path", builder.getStringAttr("pkg/model.py")},
                             {"line", wideTooLarge},
                             {"column", integer(builder, 1)},
                             {"end_line", wideTooLarge},
                             {"end_column", integer(builder, 1)}}),
        detail::verifySourceSpan));
  for (auto span :
       {sourceSpan(builder, 0, 1, 1, 2), sourceSpan(builder, 1, 0, 1, 2),
        sourceSpan(builder, 1, 1, 0, 2), sourceSpan(builder, 1, 1, 1, 0),
        sourceSpan(builder, 2, 1, 1, 2), sourceSpan(builder, 2, 4, 2, 3)})
    EXPECT_FALSE(verifies(context, span, detail::verifySourceSpan));
}

TEST(PathComponentContractTest, AcceptsOnlyClosedFieldAndIndexVariants) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);

  EXPECT_TRUE(
      verifies(context, fieldComponent(builder), detail::verifyPathComponent));
  EXPECT_TRUE(
      verifies(context,
               dictionary(builder, {{"kind", builder.getStringAttr("field")},
                                    {"name", builder.getStringAttr("")}}),
               detail::verifyPathComponent));
  EXPECT_TRUE(verifies(context, indexComponent(builder, 0),
                       detail::verifyPathComponent));
  EXPECT_TRUE(verifies(context, indexComponent(builder, 42),
                       detail::verifyPathComponent));
  EXPECT_TRUE(verifies(
      context,
      dictionary(builder, {{"kind", builder.getStringAttr("index")},
                           {"value", maximumUnsignedInteger(builder)}}),
      detail::verifyPathComponent));

  for (auto invalid : {
           dictionary(builder, {{"kind", builder.getStringAttr("field")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("index")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("other")},
                                {"name", builder.getStringAttr("payload")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("field")},
                                {"name", integer(builder, 1)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("index")},
                                {"value", builder.getBoolAttr(true)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("index")},
                                {"value", integer(builder, -1)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("index")},
                                {"value", negativeSignedInteger(builder)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("field")},
                                {"name", builder.getStringAttr("payload")},
                                {"unknown", builder.getUnitAttr()}}),
       })
    EXPECT_FALSE(verifies(context, invalid, detail::verifyPathComponent));
}

TEST(SiteContractTest, AcceptsFlatDefinitionsAndBothPathVariants) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);

  EXPECT_TRUE(verifies(context, site(builder, builder.getArrayAttr({})),
                       detail::verifySite));
  EXPECT_TRUE(verifies(
      context,
      site(builder, builder.getArrayAttr(
                        {fieldComponent(builder), indexComponent(builder, 3)})),
      detail::verifySite));
}

TEST(SiteContractTest, RejectsMissingUnknownAndWrongKindFields) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);
  auto flat = mlir::FlatSymbolRefAttr::get(&context, "demo.Model");
  auto path = builder.getArrayAttr({fieldComponent(builder)});
  auto nested = mlir::SymbolRefAttr::get(
      &context, "demo", {mlir::FlatSymbolRefAttr::get(&context, "Model")});

  for (auto invalid : {
           dictionary(builder, {{"definition", flat}}),
           dictionary(builder, {{"ast_path", path}}),
           dictionary(builder, {{"definition", flat},
                                {"ast_path", path},
                                {"unknown", builder.getUnitAttr()}}),
           dictionary(builder,
                      {{"definition", builder.getStringAttr("demo.Model")},
                       {"ast_path", path}}),
           dictionary(builder, {{"definition", nested}, {"ast_path", path}}),
           dictionary(builder, {{"definition", flat},
                                {"ast_path", builder.getStringAttr("field")}}),
           dictionary(builder, {{"definition", flat},
                                {"ast_path",
                                 builder.getArrayAttr({integer(builder, 0)})}}),
           dictionary(
               builder,
               {{"definition", flat},
                {"ast_path",
                 builder.getArrayAttr({dictionary(
                     builder, {{"kind", builder.getStringAttr("index")},
                               {"value", builder.getBoolAttr(true)}})})}}),
       })
    EXPECT_FALSE(verifies(context, invalid, detail::verifySite));
}

TEST(SiteContractTest, NonDictionaryPathElementEmitsADiagnostic) {
  mlir::MLIRContext context;
  mlir::OpBuilder builder(&context);
  auto invalid = site(builder, builder.getArrayAttr({integer(builder, 0)}));
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler captureDiagnostic(
      &context, [&](mlir::Diagnostic &value) {
        llvm::raw_string_ostream stream(diagnostic);
        value.print(stream);
        return mlir::success();
      });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };

  EXPECT_TRUE(mlir::failed(detail::verifySite(invalid, emitError)));
  EXPECT_NE(diagnostic.find("Site"), std::string::npos) << diagnostic;
  EXPECT_NE(diagnostic.find("DictionaryAttr"), std::string::npos) << diagnostic;
}

} // namespace
} // namespace acir::ac
