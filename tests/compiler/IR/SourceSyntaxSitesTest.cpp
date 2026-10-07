#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

std::string span(llvm::StringRef path = "caller.py", unsigned column = 1,
                 unsigned endColumn = 20) {
  return "{path = \"" + path.str() +
         "\", line = 3 : i64, column = " + std::to_string(column) +
         " : i64, end_line = 3 : i64, end_column = " +
         std::to_string(endColumn) + " : i64}";
}
std::string site(llvm::StringRef path = "caller.py") {
  return "{ast_path = [{kind = \"field\", name = \"body\"}, {kind = \"index\", "
         "value = 4 : i64}, "
         "{kind = \"field\", name = \"value\"}], location = " +
         span(path, 8, 13) + "}";
}
std::string origin() {
  return "{site = {definition = @Caller, ast_path = [{kind = \"field\", name = "
         "\"body\"}, "
         "{kind = \"index\", value = 4 : i64}]}, expansion = []}";
}
std::string annotation(llvm::StringRef syntaxSite) {
  return "#ac.source_type_expr<{kind = \"alias\", name = [\"pkg\", \"Word\"], "
         "site = " +
         syntaxSite.str() + "}>";
}
std::string
lexicalReference(llvm::StringRef syntaxSite,
                 llvm::StringRef names = "[\"facade\", \"LIMIT\"]") {
  return "#ac.static_expr<{kind = \"reference\", ref = {kind = \"lexical\", "
         "name = " +
         names.str() + ", site = " + syntaxSite.str() +
         "}, origin = " + origin() + ", location = " + span() + "}>";
}
std::string literal() {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
         "value = #ac.math_int<17>}, "
         "origin = " +
         origin() + ", location = " + span() + "}>";
}
std::string call(llvm::StringRef callee, llvm::StringRef syntaxSite = {}) {
  return "#ac.static_expr<{kind = \"call\", callee = " + callee.str() +
         (syntaxSite.empty() ? "" : ", callee_site = " + syntaxSite.str()) +
         ", arguments = [{kind = \"positional\", value = " + literal() +
         "}, {kind = \"keyword\", name = \"limit\", value = " + literal() +
         "}], origin = " + origin() + ", location = " + span() + "}>";
}

class SourceSyntaxSitesTest : public ::testing::Test {
protected:
  SourceSyntaxSitesTest() { context.loadDialect<ACIRDialect>(); }

  mlir::Attribute roundtrip(llvm::StringRef source) {
    auto parsed = mlir::parseAttribute(source, &context);
    EXPECT_TRUE(parsed) << source.str();
    if (!parsed)
      return {};
    std::string printed;
    llvm::raw_string_ostream stream(printed);
    parsed.print(stream);
    EXPECT_EQ(mlir::parseAttribute(printed, &context), parsed);
    return parsed;
  }
  void rejects(llvm::StringRef source, llvm::StringRef reason = {}) {
    SCOPED_TRACE(source.str());
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    EXPECT_FALSE(mlir::parseAttribute(source, &context));
    EXPECT_FALSE(diagnostics.empty());
    if (!reason.empty())
      EXPECT_NE(diagnostics.find(reason.str()), std::string::npos)
          << diagnostics;
  }
  std::string replace(std::string text, llvm::StringRef before,
                      llvm::StringRef after) {
    auto offset = text.find(before.str());
    EXPECT_NE(offset, std::string::npos);
    if (offset != std::string::npos)
      text.replace(offset, before.size(), after.str());
    return text;
  }
  mlir::MLIRContext context;
};

TEST_F(SourceSyntaxSitesTest, AliasRequiresExactlyKindNameAndCapturedSite) {
  auto alias = roundtrip(annotation(site()));
  ASSERT_TRUE(mlir::isa<SourceTypeExprAttr>(alias));
  auto record = mlir::cast<SourceTypeExprAttr>(alias).getValue();
  auto captured = record.getAs<mlir::DictionaryAttr>("site");
  ASSERT_TRUE(captured);
  auto path = captured.getAs<mlir::ArrayAttr>("ast_path");
  ASSERT_TRUE(path && !path.empty());
  EXPECT_EQ(captured.getAs<mlir::DictionaryAttr>("location")
                .getAs<mlir::IntegerAttr>("column")
                .getInt(),
            8);
  rejects(
      R"attr(#ac.source_type_expr<{kind = "alias", name = ["pkg", "Word"]}>)attr");
  rejects(replace(annotation(site()), "site = ", "extra = true, site = "));
  rejects(annotation("unit"));
  rejects(annotation("{}"));
  // Context-free alias syntax cannot prove a declaration's owning file.
  // D1b binds that operation owner; a structurally valid other path is legal
  // here.
  roundtrip(annotation(site("another.py")));
}

TEST_F(SourceSyntaxSitesTest, NamespaceSitesKeepPathAndSpanStructureClosed) {
  for (llvm::StringRef malformed :
       {"{ast_path = []}", "{location = {path = \"caller.py\"}}",
        "{ast_path = unit, location = unit}",
        "{ast_path = [1 : i64], location = unit}"})
    rejects(annotation(malformed));
  const std::string valid = site();
  rejects(annotation(replace(valid, "ast_path =", "extra = unit, ast_path =")));
  rejects(annotation(replace(valid, "kind = \"field\"", "kind = \"unknown\"")));
  rejects(annotation(replace(valid, "value = 4 : i64", "value = \"four\"")));
  rejects(annotation(replace(valid, "column = 8 : i64", "column = 0 : i64")));
  rejects(annotation(
      replace(valid, "end_column = 13 : i64", "end_column = 7 : i64")));
  rejects(annotation(replace(valid, "line = 3 : i64", "line = \"three\"")));
}

TEST_F(SourceSyntaxSitesTest, LexicalStaticReferenceRequiresRealSameFileSite) {
  auto expression = roundtrip(lexicalReference(site()));
  ASSERT_TRUE(mlir::isa<StaticExprAttr>(expression));
  // A captured name's span differs from the full expression's span; same file
  // is required, not artificial equality of columns or source paths.
  roundtrip(lexicalReference(site(), "[\"数据\", \"数值\"]"));
  rejects(lexicalReference(site("other.py")));
  rejects(replace(lexicalReference(site()), ", site = " + site(), ""));
  rejects(lexicalReference("unit"));
  rejects(
      replace(lexicalReference(site()), "name = ", "extra = unit, name = "));
  for (llvm::StringRef names :
       {"[]", "[1 : i64]", R"names(["bad-name"])names", R"names(["True"])names",
        R"names(["None"])names", R"names(["K"])names", R"names(["\FF"])names"})
    rejects(lexicalReference(site(), names));
}

TEST_F(SourceSyntaxSitesTest,
       LexicalCallsRequireSitesAndCanonicalCallsForbidThem) {
  roundtrip(call("[\"facade\", \"helper\"]", site()));
  roundtrip(call("@\"ultimate.helper\""));
  rejects(call("[\"facade\", \"helper\"]"));
  rejects(call("@\"ultimate.helper\"", site()));
  rejects(call("[\"facade\", \"helper\"]", site("other.py")));
  rejects(call("[\"facade\", \"helper\"]", "unit"));
  rejects(call("unit", site()));
  rejects(call("[]", site()));
  rejects(call("[1 : i8]", site()));
  rejects(call("[\"bad-name\"]", site()));
  rejects(replace(call("[\"facade\", \"helper\"]", site()), "kind = \"call\",",
                  "kind = \"call\", extra = unit,"));
  rejects(replace(call("[\"facade\", \"helper\"]", site()),
                  "kind = \"positional\",", "kind = \"unknown\","));
}

TEST_F(SourceSyntaxSitesTest,
       ContextFreeTreesAcceptConsistentForeignSourcePaths) {
  auto foreign = [&](std::string expression) {
    while (expression.find("caller.py") != std::string::npos)
      expression = replace(expression, "caller.py", "foreign.py");
    return expression;
  };
  // These parse successfully without a declaring operation. D1b owner checks
  // later distinguish this valid carrier from a valid carrier in the wrong
  // file.
  roundtrip(foreign(lexicalReference(site())));
  roundtrip(foreign(call("[\"facade\", \"helper\"]", site())));
  roundtrip(foreign(call("@\"ultimate.helper\"")));
  roundtrip(foreign(literal()));
}

TEST_F(SourceSyntaxSitesTest, SharedStaticDagsVerifyAndRebuiltBadLeavesReject) {
  auto leaf =
      mlir::dyn_cast<StaticExprAttr>(mlir::parseAttribute(literal(), &context));
  auto lexical = mlir::dyn_cast<StaticExprAttr>(
      mlir::parseAttribute(lexicalReference(site()), &context));
  ASSERT_TRUE(leaf && lexical);
  mlir::Builder builder(&context);
  auto buildDag = [&](StaticExprAttr initial, unsigned depth) {
    StaticExprAttr previous = initial;
    for (unsigned index = 0; index < depth; ++index) {
      auto tree = builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
           builder.getNamedAttr("operator", builder.getStringAttr("add")),
           builder.getNamedAttr("lhs", previous),
           builder.getNamedAttr("rhs", previous),
           builder.getNamedAttr("origin", leaf.getTree().get("origin")),
           builder.getNamedAttr("location", leaf.getTree().get("location"))});
      // Unchecked construction preserves sharing without recursively checking
      // every intermediate getter. The real grammar verifier runs below.
      previous = StaticExprAttr::get(&context, tree);
    }
    return previous;
  };
  auto verifies = [&](StaticExprAttr expression) {
    mlir::ScopedDiagnosticHandler suppress(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    auto emit = [&] {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return mlir::succeeded(StaticExprAttr::verify(emit, expression.getTree()));
  };
  auto withField = [&](mlir::DictionaryAttr dictionary, llvm::StringRef name,
                       mlir::Attribute value) {
    mlir::NamedAttrList fields(dictionary);
    fields.set(name, value);
    return fields.getDictionary(&context);
  };
  auto unknown = StaticExprAttr::get(
      &context,
      withField(leaf.getTree(), "kind", builder.getStringAttr("unknown")));
  auto extra =
      StaticExprAttr::get(&context, withField(leaf.getTree(), "unfounded_field",
                                              builder.getUnitAttr()));
  auto badSite = mlir::parseAttribute(site("foreign.py"), &context);
  ASSERT_TRUE(badSite);
  auto reference = lexical.getTree().getAs<mlir::DictionaryAttr>("ref");
  auto wrongLexicalSite = StaticExprAttr::get(
      &context, withField(lexical.getTree(), "ref",
                          withField(reference, "site", badSite)));
  // Size is stress input, not an admission limit or wall-clock assertion.
  for (unsigned depth : {8u, 64u}) {
    auto graph = buildDag(leaf, depth);
    EXPECT_TRUE(verifies(graph));
    EXPECT_TRUE(verifies(buildDag(lexical, depth)));
    // Pending bound syntax may share the same expression. This checks carrier
    // grammar, not evaluated lower/upper membership or a resolved domain.
    auto annotation = SourceTypeExprAttr::get(
        &context,
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
             builder.getNamedAttr("lower", graph),
             builder.getNamedAttr("upper", graph)}));
    auto emit = [&] {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    EXPECT_TRUE(mlir::succeeded(
        SourceTypeExprAttr::verify(emit, annotation.getValue())));
    // Attributes are immutable: each changed leaf/top is a new graph. A
    // previous valid traversal must never mask malformed rebuilt descendants.
    EXPECT_FALSE(verifies(buildDag(unknown, depth)));
    EXPECT_FALSE(verifies(buildDag(extra, depth)));
    EXPECT_FALSE(verifies(buildDag(wrongLexicalSite, depth)));
    EXPECT_TRUE(verifies(buildDag(leaf, depth)));
  }
  // Exercise ordinary textual parse/print with a bounded representation.
  // Printing the deeply shared graph would expand it exponentially as text.
  auto small = buildDag(leaf, 3);
  std::string printed;
  llvm::raw_string_ostream stream(printed);
  mlir::Attribute(small).print(stream);
  EXPECT_EQ(roundtrip(printed), small);
}

TEST_F(SourceSyntaxSitesTest,
       ExistingCanonicalStaticReferencesAndNestedGrammarRemain) {
  for (llvm::StringRef reference :
       {"{kind = \"export\", symbol = @LIMIT}",
        "{kind = \"parameter\", owner = @Caller, name = \"limit\"}",
        "{kind = \"binding\", id = {site = {definition = @Caller, ast_path = "
        "[]}, expansion = []}}",
        "{kind = \"induction\", loop = {site = {definition = @Caller, ast_path "
        "= []}, expansion = []}}"})
    roundtrip(
        "#ac.static_expr<{kind = \"reference\", ref = " + reference.str() +
        ", origin = " + origin() + ", location = " + span() + "}>");
  const std::string nested =
      "#ac.static_expr<{kind = \"binary\", operator = \"add\", lhs = " +
      lexicalReference(site()) +
      ", rhs = " + call("[\"helpers\", \"value\"]", site()) +
      ", origin = " + origin() + ", location = " + span() + "}>";
  roundtrip(nested);
  rejects(replace(nested, "callee_site = " + site(),
                  "callee_site = " + site("foreign.py")));
}

} // namespace
} // namespace acir::ac
