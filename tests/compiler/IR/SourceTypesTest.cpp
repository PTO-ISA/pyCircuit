#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>

namespace acir::ac {
namespace {

std::string integerDomain(llvm::StringRef lower, llvm::StringRef upper) {
  return "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<" +
         lower.str() + ">, upper = #ac.math_int<" + upper.str() + ">}>";
}

// Reuse the existing LiteralExpr grammar; type-expression carriers describe
// pending annotations and do not evaluate the static expression themselves.
std::string literalExpr(llvm::StringRef value) {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
         "value = #ac.math_int<" +
         value.str() +
         ">}, origin = "
         "{site = {definition = @Annotations, ast_path = []}, expansion = []}, "
         "location = {path = \"annotations.py\", line = 1 : i64, "
         "column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>";
}

std::string integerExpr(llvm::StringRef lower, llvm::StringRef upper) {
  return "#ac.source_type_expr<{kind = \"integer\", lower = " +
         literalExpr(lower) + ", upper = " + literalExpr(upper) + "}>";
}

const char *aliasSite =
    R"site({ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], location = {path = "annotations.py", line = 4 : i64, column = 3 : i64, end_line = 4 : i64, end_column = 17 : i64}})site";

std::string alias(llvm::StringRef name) {
  return "#ac.source_type_expr<{kind = \"alias\", name = " + name.str() +
         ", site = " + aliasSite + "}>";
}

class SourceTypesTest : public ::testing::Test {
protected:
  SourceTypesTest() { context.loadDialect<ACIRDialect>(); }

  mlir::Attribute attributeRoundtrip(llvm::StringRef source) {
    auto value = mlir::parseAttribute(source, &context);
    EXPECT_TRUE(value) << source.str();
    if (!value)
      return {};
    std::string printed;
    llvm::raw_string_ostream stream(printed);
    value.print(stream);
    EXPECT_EQ(mlir::parseAttribute(printed, &context), value);
    return value;
  }

  mlir::Type typeRoundtrip(llvm::StringRef source) {
    auto value = mlir::parseType(source, &context);
    EXPECT_TRUE(value) << source.str();
    if (!value)
      return {};
    std::string printed;
    llvm::raw_string_ostream stream(printed);
    value.print(stream);
    EXPECT_EQ(mlir::parseType(printed, &context), value);
    return value;
  }

  void rejects(llvm::StringRef source, bool type = false) {
    SCOPED_TRACE(source.str());
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    if (type)
      EXPECT_FALSE(mlir::parseType(source, &context));
    else
      EXPECT_FALSE(mlir::parseAttribute(source, &context));
    EXPECT_FALSE(diagnostics.empty());
  }

  std::string replace(std::string source, llvm::StringRef before,
                      llvm::StringRef after) {
    auto offset = source.find(before.str());
    EXPECT_NE(offset, std::string::npos);
    if (offset != std::string::npos)
      source.replace(offset, before.size(), after.str());
    return source;
  }

  mlir::MLIRContext context;
};

TEST_F(SourceTypesTest, RegisteredTypesAndReferenceDomainsRoundtrip) {
  EXPECT_TRUE(mlir::isa<UnresolvedType>(typeRoundtrip("!ac.unresolved")));
  EXPECT_TRUE(mlir::isa<BoolType>(typeRoundtrip("!ac.bool")));
  auto boolean = attributeRoundtrip("#ac.source_domain<{kind = \"bool\"}>");
  auto integer = attributeRoundtrip(integerDomain("0", "2"));
  ASSERT_TRUE(mlir::isa<SourceDomainAttr>(boolean));
  ASSERT_TRUE(mlir::isa<SourceDomainAttr>(integer));
  EXPECT_NE(boolean, integer);
  auto boolRef = typeRoundtrip("!ac.ref<#ac.source_domain<{kind = \"bool\"}>>");
  auto intRef = typeRoundtrip("!ac.ref<" + integerDomain("0", "2") + ">");
  ASSERT_TRUE(mlir::isa<RefType>(boolRef));
  ASSERT_TRUE(mlir::isa<RefType>(intRef));
  EXPECT_NE(boolRef, intRef);
  EXPECT_EQ(mlir::cast<RefType>(boolRef).getDomain(), boolean);
  EXPECT_EQ(mlir::cast<RefType>(intRef).getDomain(), integer);
  // Both may eventually select i1 storage, but their source kinds stay
  // distinct.
  EXPECT_NE(typeRoundtrip("!ac.bool"), mlir::IntegerType::get(&context, 1));
}

TEST_F(SourceTypesTest, MathematicalDomainsHaveNoPhysicalWidthLimit) {
  for (auto [lower, upper] :
       {std::pair{"-1", "1"},
        std::pair{"18446744073709551616", "18446744073709551617"},
        std::pair{"-340282366920938463463374607431768211456",
                  "340282366920938463463374607431768211456"}}) {
    auto domain = attributeRoundtrip(integerDomain(lower, upper));
    ASSERT_TRUE(mlir::isa<SourceDomainAttr>(domain));
    const auto fields = mlir::cast<SourceDomainAttr>(domain).getValue();
    EXPECT_EQ(fields.getAs<MathIntAttr>("lower").getCanonicalValue(), lower);
    EXPECT_EQ(fields.getAs<MathIntAttr>("upper").getCanonicalValue(), upper);
    EXPECT_FALSE(fields.get("storage"));
    EXPECT_FALSE(fields.get("interpretation"));
    typeRoundtrip("!ac.ref<" + integerDomain(lower, upper) + ">");
  }
}

TEST_F(SourceTypesTest, DomainsRejectMalformedKindsBoundsAndPhysicalFields) {
  for (llvm::StringRef source :
       {"#ac.source_domain<{}>", "#ac.source_domain<{kind = 1 : i8}>",
        "#ac.source_domain<{kind = \"record\"}>",
        "#ac.source_domain<{kind = \"bool\", storage = i1}>",
        "#ac.source_domain<{kind = \"bool\", lower = #ac.math_int<0>}>",
        "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<0>}>",
        "#ac.source_domain<{kind = \"integer\", lower = 0 : i64, upper = "
        "#ac.math_int<2>}>",
        "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<0>, upper "
        "= 2 : i64}>"})
    rejects(source);
  for (auto [lower, upper] :
       {std::pair{"0", "0"}, std::pair{"7", "-2"},
        std::pair{"18446744073709551617", "18446744073709551616"}})
    rejects(integerDomain(lower, upper));
  for (llvm::StringRef field :
       {"storage = i64", "signedness = \"signed\"",
        "interpretation = \"unsigned\"", "extra = unit"})
    rejects(replace(integerDomain("0", "256"), "kind = \"integer\",",
                    "kind = \"integer\", " + field.str() + ","));
}

TEST_F(SourceTypesTest, ReferenceParameterMustBeAResolvedSourceDomain) {
  for (llvm::StringRef source :
       {"!ac.ref<i1>", "!ac.ref<!ac.bool>",
        "!ac.ref<#ac.source_type_expr<{kind = \"bool\"}>>",
        "!ac.ref<#ac.source_domain<{kind = \"unresolved\"}>>",
        "!ac.ref<#ac.source_domain<{kind = \"integer\", lower = "
        "#ac.math_int<1>, upper = #ac.math_int<1>}>>"})
    rejects(source, true);
  // A checked ref must reverify even an unchecked programmatic attribute.
  mlir::Builder builder(&context);
  auto unchecked = SourceDomainAttr::get(
      &context,
      builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("bool")),
           builder.getNamedAttr("storage",
                                mlir::TypeAttr::get(builder.getI1Type()))}));
  std::string diagnostics;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
    llvm::raw_string_ostream stream(diagnostics);
    d.print(stream);
    return mlir::success();
  });
  auto emit = [&] { return mlir::emitError(mlir::UnknownLoc::get(&context)); };
  EXPECT_FALSE(RefType::getChecked(emit, &context, unchecked));
  EXPECT_FALSE(RefType::getChecked(emit, &context, SourceDomainAttr{}));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceTypesTest, PendingAnnotationsUseExistingStaticExpressionGrammar) {
  EXPECT_TRUE(mlir::isa<SourceTypeExprAttr>(
      attributeRoundtrip("#ac.source_type_expr<{kind = \"bool\"}>")));
  auto integer = attributeRoundtrip(integerExpr("0", "18446744073709551617"));
  ASSERT_TRUE(mlir::isa<SourceTypeExprAttr>(integer));
  auto fields = mlir::cast<SourceTypeExprAttr>(integer).getValue();
  EXPECT_TRUE(mlir::isa<StaticExprAttr>(fields.get("lower")));
  EXPECT_TRUE(mlir::isa<StaticExprAttr>(fields.get("upper")));
  // Resolution owns evaluation/membership; this carrier owns StaticExpr
  // grammar.
  attributeRoundtrip(integerExpr("-71", "93"));
  for (llvm::StringRef source :
       {"#ac.source_type_expr<{}>",
        "#ac.source_type_expr<{kind = \"integer\", lower = #ac.math_int<0>, "
        "upper = #ac.math_int<2>}>",
        "#ac.source_type_expr<{kind = \"integer\", lower = 0 : i64, upper = 2 "
        ": i64}>",
        "#ac.source_type_expr<{kind = \"bool\", storage = i1}>",
        "#ac.source_type_expr<{kind = \"bool\", extra = true}>",
        "#ac.source_type_expr<{kind = \"unknown\"}>"})
    rejects(source);
  const std::string expression = integerExpr("0", "5");
  rejects(replace(expression, "kind = \"literal\"", "kind = \"unknown\""));
  rejects(replace(expression, "end_column = 2", "end_column = 0"));
  rejects(replace(expression, "lower = " + literalExpr("0"), "lower = unit"));
  rejects(replace(expression, "kind = \"integer\", lower",
                  "kind = \"integer\", signedness = \"unsigned\", lower"));
}

TEST_F(SourceTypesTest,
       AliasComponentsFollowTheCommonLexicalIdentifierContract) {
  for (llvm::StringRef names :
       {R"names(["Word"])names", R"names(["pkg", "_Types", "Value2"])names",
        R"names(["数据", "数值"])names"})
    EXPECT_TRUE(
        mlir::isa<SourceTypeExprAttr>(attributeRoundtrip(alias(names))));
  for (llvm::StringRef names :
       {"[]", R"names("pkg.Word")names", "[1 : i8]", R"names([""])names",
        R"names(["pkg.Word"])names", R"names(["3Word"])names",
        R"names(["bad-name"])names", R"names(["bad\00name"])names"})
    rejects(alias(names));
  rejects(replace(alias(R"names(["Word"])names"), "kind = \"alias\",",
                  "kind = \"alias\", extra = true,"));
  // The retired two-field carrier is a shape negative, independent of lexical
  // failures below, all of which carry otherwise-valid captured syntax sites.
  rejects(R"attr(#ac.source_type_expr<{kind = "alias", name = ["Word"]}>)attr");
  for (llvm::StringRef names :
       {R"names(["K"])names", R"names(["é"])names", R"names(["True"])names",
        R"names(["None"])names", R"names(["\FF"])names"})
    rejects(alias(names));

  mlir::Builder builder(&context);
  auto site = mlir::parseAttribute(aliasSite, &context);
  ASSERT_TRUE(mlir::isa<mlir::DictionaryAttr>(site));
  const std::string malformedUtf8(1, static_cast<char>(0xff));
  auto payload = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("alias")),
       builder.getNamedAttr("name", builder.getArrayAttr({builder.getStringAttr(
                                        malformedUtf8)})),
       builder.getNamedAttr("site", site)});
  std::string diagnostics;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
    llvm::raw_string_ostream stream(diagnostics);
    d.print(stream);
    return mlir::success();
  });
  auto emit = [&] { return mlir::emitError(mlir::UnknownLoc::get(&context)); };
  EXPECT_FALSE(SourceTypeExprAttr::getChecked(emit, &context, payload));
  EXPECT_NE(diagnostics.find("valid UTF-8"), std::string::npos) << diagnostics;
}

} // namespace
} // namespace acir::ac
