// Independent AST/scope and private-budget invariants; public query execution
// belongs to the existing source-interface and Enum/collection owners.
#include "Compiler/PythonImportAST.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/TableQueryLowering.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseSet.h"
#include "gtest/gtest.h"
#include <array>
#include <limits>
#include <string>

namespace {
using namespace mlir;
namespace detail = acir::compiler::detail;
using Resource = detail::TableQueryResource;
using Budget = detail::TableQueryBudget;
using Charge = detail::TableQueryCharge;

class TableQuerySourceBoundary : public ::testing::Test {
protected:
  MLIRContext context;
  Builder b{&context};
  std::string diagnostics;
  void SetUp() override { context.loadDialect<acir::ac::ACIRDialect>(); }
  NamedAttribute field(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr node(StringRef kind, ArrayRef<NamedAttribute> fields = {}) {
    SmallVector<NamedAttribute> span;
    for (StringRef name : {"start_line", "start_byte_column",
                           "start_codepoint_column", "end_line",
                           "end_byte_column", "end_codepoint_column"})
      span.push_back(field(name, b.getI64IntegerAttr(1)));
    return b.getDictionaryAttr({field("kind", b.getStringAttr(kind)),
                               field("fields", b.getDictionaryAttr(fields)),
                               field("span", b.getDictionaryAttr(span))});
  }
  DictionaryAttr name(StringRef spelling) {
    return node("Name", {field("id", b.getStringAttr(spelling))});
  }
  DictionaryAttr argument(StringRef spelling = "row") {
    return node("arg", {field("arg", b.getStringAttr(spelling)),
                        field("annotation", b.getUnitAttr())});
  }
  DictionaryAttr arguments(ArrayRef<Attribute> positional = {}) {
    return node("arguments", {
        field("args", b.getArrayAttr(positional)),
        field("posonlyargs", b.getArrayAttr({})),
        field("kwonlyargs", b.getArrayAttr({})),
        field("defaults", b.getArrayAttr({})),
        field("kw_defaults", b.getArrayAttr({})),
        field("vararg", b.getUnitAttr()), field("kwarg", b.getUnitAttr())});
  }
  DictionaryAttr lambda(DictionaryAttr args, DictionaryAttr body) {
    return node("Lambda", {field("args", args), field("body", body)});
  }
  DictionaryAttr change(DictionaryAttr ast, StringRef key, Attribute value) {
    NamedAttrList fields(ast.getAs<DictionaryAttr>("fields"));
    if (value) fields.set(key, value); else fields.erase(key);
    return node(ast.getAs<StringAttr>("kind").getValue(),
                fields.getDictionary(&context).getValue());
  }
  OwningOpRef<ModuleOp> transport(ArrayRef<Attribute> statements) {
    auto module = ModuleOp::create(b.getUnknownLoc());
    auto root = node("Module", {field("body", b.getArrayAttr(statements))});
    module->setAttr("ac.python_capture", b.getArrayAttr({b.getDictionaryAttr({
        field("path", b.getStringAttr("query.py")),
        field("body", b.getArrayAttr({root}))})}));
    return module;
  }
  FailureOr<detail::CapturedSource> capture(ModuleOp module) {
    return detail::readSingleCapture(module,
             [&] { return mlir::emitError(b.getUnknownLoc()); });
  }
  void rejects(DictionaryAttr callback) {
    auto input = transport({node("Expr", {field("value", callback)})});
    std::string before, after;
    { llvm::raw_string_ostream out(before); input->print(out); }
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream out(diagnostics); error.print(out);
      return success();
    });
    EXPECT_TRUE(failed(capture(*input)));
    EXPECT_FALSE(diagnostics.empty());
    { llvm::raw_string_ostream out(after); input->print(out); }
    EXPECT_EQ(before, after);
  }
  DictionaryAttr attribute(StringRef base, StringRef member) {
    return node("Attribute", {field("value", name(base)),
                               field("attr", b.getStringAttr(member))});
  }
  DictionaryAttr call(DictionaryAttr callee, ArrayRef<Attribute> args = {},
                      ArrayRef<Attribute> keywords = {}) {
    return node("Call", {field("func", callee), field("args", b.getArrayAttr(args)),
                          field("keywords", b.getArrayAttr(keywords))});
  }
  DictionaryAttr integer(StringRef value) {
    return node("Constant", {field("value", b.getDictionaryAttr({
        field("integer", b.getStringAttr(value))}))});
  }
  DictionaryAttr function(StringRef spelling, DictionaryAttr args,
                          ArrayRef<Attribute> body, StringRef decorator) {
    return node("FunctionDef", {field("name", b.getStringAttr(spelling)),
        field("args", args), field("body", b.getArrayAttr(body)),
        field("decorator_list", b.getArrayAttr({attribute("ac", decorator)})),
        field("returns", b.getUnitAttr())});
  }
};

TEST_F(TableQuerySourceBoundary, CapturedOneArgumentLambdaPreservesSourceSite) {
  auto callback = lambda(arguments({argument()}), name("row"));
  auto input = transport({node("Expr", {field("value", callback)})});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto body = source->module.item("body", 0).child("value").child("body");
  EXPECT_EQ(body.string("id"), "row");
  EXPECT_EQ(body.path.size(), 4u);
  EXPECT_EQ(source->path, "query.py");
  auto args = change(arguments(), "posonlyargs", b.getArrayAttr({argument()}));
  auto positionalOnly = transport({node("Expr", {field("value", lambda(args, name("row")))})});
  EXPECT_TRUE(succeeded(capture(*positionalOnly)));
}

TEST_F(TableQuerySourceBoundary, OrdinaryLambdaArityIsTransportedForTypedMethods) {
  // Capture validates ordinary lambda structure. Receiver shape and callback
  // arity belong to typed first/argmin/map admission, exercised publicly.
  auto zero = lambda(arguments(), integer("1"));
  auto two = lambda(arguments({argument("lane"), argument("tag")}), name("tag"));
  auto mapped = call(attribute("values", "map"), {two, name("other")});
  auto input = transport({node("Expr", {field("value", zero)}),
                          node("Expr", {field("value", mapped)})});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto zeroArgs = source->module.item("body", 0).child("value").child("args");
  EXPECT_EQ(zeroArgs.array("args").size(), 0u);
  auto callback = source->module.item("body", 1).child("value").item("args", 0);
  EXPECT_EQ(callback.child("args").array("args").size(), 2u);
  auto body = callback.child("body");
  EXPECT_TRUE(detail::sourceBindingShadowed(*source, body, "lane"));
  EXPECT_TRUE(detail::sourceBindingShadowed(*source, body, "tag"));
  auto otherInput = source->module.item("body", 1).child("value").item("args", 1);
  EXPECT_FALSE(detail::sourceBindingShadowed(*source, otherInput, "tag"));
}

TEST_F(TableQuerySourceBoundary, RejectsInvalidDefaultsAndAnnotations) {
  auto valid = arguments({argument()});
  rejects(lambda(change(valid, "defaults", b.getArrayAttr({integer("1")})), name("row")));
  rejects(lambda(change(valid, "kwonlyargs", b.getArrayAttr({argument("extra")})), name("row")));
  rejects(lambda(change(valid, "kw_defaults", b.getArrayAttr({integer("1")})), name("row")));
  for (StringRef field : {"vararg", "kwarg"})
    rejects(lambda(change(valid, field, argument("extra")), name("row")));
  rejects(lambda(arguments({change(argument(), "annotation", name("int"))}), name("row")));
  rejects(lambda(arguments({argument("")}), name("row")));
}

TEST_F(TableQuerySourceBoundary, RejectsMalformedArgumentsBeforeNullableArrayAccess) {
  auto valid = arguments({argument()});
  for (StringRef field : {"args", "posonlyargs", "kwonlyargs", "defaults", "kw_defaults"}) {
    rejects(lambda(change(valid, field, {}), name("row")));
    rejects(lambda(change(valid, field, b.getUnitAttr()), name("row")));
    rejects(lambda(change(valid, field, b.getStringAttr("not an array")), name("row")));
  }
  rejects(lambda(name("not_arguments"), name("row")));
  rejects(lambda(valid, node("Pass")));
  auto callback = lambda(valid, name("row"));
  rejects(change(callback, "body", b.getUnitAttr()));
  rejects(change(callback, "args", b.getUnitAttr()));
  rejects(change(callback, "body", {}));
  rejects(change(callback, "args", {}));
  rejects(change(callback, "extra", b.getUnitAttr()));
}

TEST_F(TableQuerySourceBoundary, LambdaArgumentShadowsOnlyInsideItsBody) {
  auto callback = lambda(arguments({argument("ac")}), attribute("ac", "concat"));
  auto positionalOnly = lambda(
      change(arguments(), "posonlyargs", b.getArrayAttr({argument("ac")})),
      attribute("ac", "concat"));
  auto imported = node("Import", {field("names", b.getArrayAttr({node("alias", {
      field("name", b.getStringAttr("pycircuit")), field("asname", b.getStringAttr("ac"))})}))});
  auto input = transport({imported, node("Expr", {field("value", callback)}),
                          node("Expr", {field("value", attribute("ac", "concat"))}),
                          node("Expr", {field("value", positionalOnly)})});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto inside = source->module.item("body", 1).child("value").child("body").child("value");
  auto outside = source->module.item("body", 2).child("value").child("value");
  auto onlyInside = source->module.item("body", 3).child("value").child("body").child("value");
  auto declaration = source->module.item("body", 1).child("value").child("args").item("args", 0);
  for (unsigned repeat = 0; repeat < 4; ++repeat) {
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, inside, "ac"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, outside, "ac"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, onlyInside, "ac"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, declaration, "ac"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inside, "other"));
  }
}

TEST_F(TableQuerySourceBoundary, LateLexicalBindingSurvivesRepeatedTemporaryNameQueries) {
  const StringRef bound = "late_binding_with_owned_query_name";
  auto scope = function("Top", arguments(), {
      node("Expr", {field("value", name(bound))}),
      node("Assign", {field("targets", b.getArrayAttr({name(bound)})),
                       field("value", integer("1"))})}, "module");
  auto input = transport({scope});
  std::string before, after;
  { llvm::raw_string_ostream out(before); input->print(out); }
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto reference = source->module.item("body", 0).item("body", 0).child("value");
  auto originalSpan = reference.value.getAs<DictionaryAttr>("span");
  for (unsigned repeat = 0; repeat < 8; ++repeat) {
    // Each query's allocated name storage dies before the following query.
    EXPECT_TRUE(detail::sourceBindingShadowed(
        *source, reference, std::string("late_binding_") + "with_owned_query_name"));
    EXPECT_FALSE(detail::sourceBindingShadowed(
        *source, reference, std::string("missing_binding_") + "with_owned_query_name"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, reference, "ac"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, reference, bound));
  }
  EXPECT_EQ(source->path, "query.py");
  EXPECT_EQ(source->module.item("body", 0).value, scope);
  EXPECT_EQ(reference.value.getAs<DictionaryAttr>("span"), originalSpan);
  ASSERT_EQ(reference.path.size(), 5u);
  EXPECT_EQ(reference.path[0].field, "body");
  EXPECT_EQ(reference.path[1].index, 0u);
  EXPECT_EQ(reference.path[2].field, "body");
  EXPECT_EQ(reference.path[3].index, 0u);
  EXPECT_EQ(reference.path[4].field, "value");
  { llvm::raw_string_ostream out(after); input->print(out); }
  EXPECT_EQ(before, after);
}

TEST_F(TableQuerySourceBoundary, NestedDefinitionBodiesKeepTheirBindingsLocal) {
  auto nested = function("Nested", arguments(), {
      node("Expr", {field("value", name("function_local"))}),
      node("Assign", {field("targets", b.getArrayAttr({name("function_local")})),
                       field("value", integer("1"))})}, "rule");
  auto nestedClass = node("ClassDef", {
      field("name", b.getStringAttr("NestedClass")),
      field("bases", b.getArrayAttr({})), field("keywords", b.getArrayAttr({})),
      field("decorator_list", b.getArrayAttr({})),
      field("body", b.getArrayAttr({
          node("Expr", {field("value", name("class_local"))}),
          node("Assign", {field("targets", b.getArrayAttr({name("class_local")})),
                           field("value", integer("1"))})}))});
  auto input = transport({function("Top", arguments(), {
      node("Expr", {field("value", name("function_local"))}), nested, nestedClass}, "module")});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto outer = source->module.item("body", 0);
  auto outside = outer.item("body", 0).child("value");
  auto inFunction = outer.item("body", 1).item("body", 0).child("value");
  auto inClass = outer.item("body", 2).item("body", 0).child("value");
  for (unsigned repeat = 0; repeat < 4; ++repeat) {
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, inFunction, "function_local"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, outside, "function_local"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, inClass, "class_local"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, outside, "class_local"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inFunction, "class_local"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inClass, "function_local"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, outside, "Nested"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, outside, "NestedClass"));
  }
}

TEST_F(TableQuerySourceBoundary, SharedInnerScopeRetainsOccurrenceSpecificOuterBindings) {
  auto shared = function("Inner", arguments(), {
      node("Expr", {field("value", name("ac"))})}, "rule");
  auto shadowed = function("Shadowed", arguments(), {
      shared, node("Assign", {field("targets", b.getArrayAttr({name("ac")})),
                               field("value", integer("1"))})}, "module");
  auto clear = function("Clear", arguments(), {shared}, "module");
  auto input = transport({shadowed, clear});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto shadowedInner = source->module.item("body", 0).item("body", 0);
  auto clearInner = source->module.item("body", 1).item("body", 0);
  ASSERT_EQ(shadowedInner.value, clearInner.value);
  auto inShadowed = shadowedInner.item("body", 0).child("value");
  auto inClear = clearInner.item("body", 0).child("value");
  for (unsigned repeat = 0; repeat < 4; ++repeat) {
    EXPECT_TRUE(detail::sourceBindingShadowed(*source, inShadowed, "ac"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inClear, "ac"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inClear, "absent"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inShadowed, "absent"));
  }
}

TEST_F(TableQuerySourceBoundary, FunctionArgumentsBindOnlyInsideBody) {
  auto args = arguments({change(argument("positional"), "annotation", name("positional"))});
  args = change(args, "posonlyargs", b.getArrayAttr({argument("posonly")}));
  args = change(args, "kwonlyargs", b.getArrayAttr({argument("keyword")}));
  args = change(args, "vararg", argument("variadic"));
  args = change(args, "kwarg", argument("keywords"));
  args = change(args, "defaults", b.getArrayAttr({name("positional")}));
  args = change(args, "kw_defaults", b.getArrayAttr({name("keyword")}));
  auto scope = function("Top", args, {
      node("Expr", {field("value", name("positional"))})}, "module");
  scope = change(scope, "decorator_list", b.getArrayAttr({attribute("positional", "module")}));
  scope = change(scope, "returns", name("positional"));
  auto input = transport({scope});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto capturedScope = source->module.item("body", 0);
  auto inside = capturedScope.item("body", 0).child("value");
  std::array<detail::AstNode, 5> headers{
      capturedScope.child("args").item("args", 0).child("annotation"),
      capturedScope.child("args").item("defaults", 0),
      capturedScope.child("args").item("kw_defaults", 0),
      capturedScope.item("decorator_list", 0).child("value"),
      capturedScope.child("returns")};
  for (unsigned repeat = 0; repeat < 4; ++repeat) {
    for (StringRef binding : {"positional", "posonly", "keyword", "variadic", "keywords"}) {
      EXPECT_TRUE(detail::sourceBindingShadowed(*source, inside, binding));
      for (const auto &header : headers)
        EXPECT_FALSE(detail::sourceBindingShadowed(*source, header, binding));
    }
    EXPECT_FALSE(detail::sourceBindingShadowed(*source, inside, "absent"));
  }
}

TEST_F(TableQuerySourceBoundary, CaptureReplacementKeepsBindingAnswersIndependent) {
  auto makeScope = [&](StringRef binding) {
    return function("Top", arguments(), {
        node("Expr", {field("value", name("ac"))}),
        node("Assign", {field("targets", b.getArrayAttr({name(binding)})),
                         field("value", integer("1"))})}, "module");
  };
  auto input = transport({makeScope("ac")});
  auto original = capture(*input);
  ASSERT_TRUE(succeeded(original));
  auto originalReference = original->module.item("body", 0).item("body", 0).child("value");
  EXPECT_TRUE(detail::sourceBindingShadowed(*original, originalReference, "ac"));
  EXPECT_FALSE(detail::sourceBindingShadowed(*original, originalReference, "other"));

  auto replacementInput = transport({makeScope("other")});
  (*input)->setAttr("ac.python_capture", (*replacementInput)->getAttr("ac.python_capture"));
  auto replacement = capture(*input);
  ASSERT_TRUE(succeeded(replacement));
  auto replacementReference = replacement->module.item("body", 0).item("body", 0).child("value");
  ASSERT_EQ(originalReference.value, replacementReference.value);
  for (unsigned repeat = 0; repeat < 4; ++repeat) {
    EXPECT_FALSE(detail::sourceBindingShadowed(*replacement, replacementReference, "ac"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*original, originalReference, "ac"));
    EXPECT_TRUE(detail::sourceBindingShadowed(*replacement, replacementReference, "other"));
    EXPECT_FALSE(detail::sourceBindingShadowed(*original, originalReference, "other"));
  }
}

TEST_F(TableQuerySourceBoundary, CallbackRuleCallIsNotASurroundingRegistration) {
  auto import = node("Import", {field("names", b.getArrayAttr({node("alias", {
      field("name", b.getStringAttr("pycircuit")), field("asname", b.getStringAttr("ac"))})}))});
  auto write = function("Write", arguments({argument("state")}), {
      node("Assign", {field("targets", b.getArrayAttr({name("state")})),
                       field("value", integer("1"))})}, "rule");
  auto nestedCall = call(name("Write"), {name("state")});
  auto callback = lambda(arguments({argument("row")}), nestedCall);
  auto query = call(attribute("entries", "first"), {}, {node("keyword", {
      field("arg", b.getStringAttr("where")), field("value", callback)})});
  auto registration = call(name("Write"), {name("state")});
  auto top = function("Top", arguments(), {
      node("AnnAssign", {field("target", name("state")),
          field("annotation", attribute("ac", "u8")), field("value", integer("0")),
          field("simple", b.getDictionaryAttr({field("integer", b.getStringAttr("1"))}))}),
      node("Expr", {field("value", query)}),
      node("Expr", {field("value", registration)})}, "module");
  auto input = transport({import, write, top});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  detail::SourceRuleWritesAnalysis analysis(input->getOperation());
  ASSERT_TRUE(analysis.isPlanValid());
  auto module = source->module.item("body", 2);
  auto nested = module.item("body", 1).child("value").item("keywords", 0).child("value").child("body");
  auto actual = module.item("body", 2).child("value");
  EXPECT_EQ(analysis.lookup(module, nested), nullptr);
  ASSERT_NE(analysis.lookup(module, actual), nullptr);
  EXPECT_TRUE(analysis.pendingPairs().empty());
  // This is registration-boundary evidence only; callback purity is a later
  // public query admission obligation and must reject the nested Write call.
}

std::array<uint64_t, 7> counters(const Budget &budget) {
  std::array<uint64_t, 7> result;
  for (unsigned i = 0; i < result.size(); ++i)
    result[i] = budget.used(static_cast<Resource>(i));
  return result;
}

TEST(TableQueryBudget, ExactLimitsAndAtomicExhaustionAtEveryCounter) {
  const std::array<uint64_t, 7> limits{128, 4096, 1048576, 65536, 262144, 1048576, 1048576};
  for (unsigned i = 0; i < limits.size(); ++i) {
    Resource resource = static_cast<Resource>(i);
    EXPECT_EQ(Budget::limit(resource), limits[i]);
    Budget budget;
    Charge prefix;
    prefix.amounts[i] = limits[i] - 1;
    std::string reason;
    ASSERT_TRUE(budget.reserve(prefix, reason));
    ASSERT_TRUE(budget.reserveProduct(resource, 1, 1, reason));
    const auto before = counters(budget);
    EXPECT_FALSE(budget.reserveProduct(resource, 1, 1, reason));
    EXPECT_EQ(counters(budget), before);
    EXPECT_FALSE(reason.empty());
  }
}

TEST(TableQueryBudget, FailedMultiCounterReservationPublishesNoPartialCharge) {
  Budget budget;
  Charge initial;
  initial.amounts[static_cast<unsigned>(Resource::Work)] = 19;
  std::string reason;
  ASSERT_TRUE(budget.reserve(initial, reason));
  const auto before = counters(budget);
  Charge mixed;
  mixed.amounts.fill(1);
  mixed.amounts[static_cast<unsigned>(Resource::ConstantBytes)] = Budget::limit(Resource::ConstantBytes) + 1;
  EXPECT_FALSE(budget.reserve(mixed, reason));
  EXPECT_EQ(counters(budget), before);
}

TEST(TableQueryBudget, ProductsCheckRemainingCapacityBeforeMultiplication) {
  Budget budget;
  std::string reason;
  const auto before = counters(budget);
  EXPECT_FALSE(budget.reserveProduct(Resource::PayloadWords,
      std::numeric_limits<uint64_t>::max(), 2, reason));
  EXPECT_EQ(counters(budget), before);
  EXPECT_TRUE(budget.reserveProduct(Resource::PayloadWords,
      std::numeric_limits<uint64_t>::max(), 0, reason));
  EXPECT_EQ(counters(budget), before);
  EXPECT_TRUE(budget.reserveProduct(Resource::Work, 512, 1024, reason));
  auto shared = counters(budget);
  EXPECT_FALSE(budget.reserveProduct(Resource::Work, 512, 1025, reason));
  EXPECT_EQ(counters(budget), shared);
}

TEST(TableQueryBudget, ChargeBuildersRejectOverflowWithoutChangingOtherAmounts) {
  Charge charge;
  charge.amounts[static_cast<unsigned>(Resource::Operations)] = 17;
  charge.amounts[static_cast<unsigned>(Resource::Work)] = std::numeric_limits<uint64_t>::max();
  const auto before = charge.amounts;
  EXPECT_FALSE(Budget::add(charge, Resource::Work, 1));
  EXPECT_EQ(charge.amounts, before);
  EXPECT_FALSE(Budget::addProduct(charge, Resource::PayloadWords,
      std::numeric_limits<uint64_t>::max(), 2));
  EXPECT_EQ(charge.amounts, before);
  EXPECT_TRUE(Budget::addProduct(charge, Resource::PayloadWords,
      std::numeric_limits<uint64_t>::max(), 0));
  EXPECT_EQ(charge.amounts, before);
}

TEST(TableQueryBudget, NestingIsAnActiveDepthAndWorkRemainsCumulative) {
  Budget budget;
  std::string reason;
  for (unsigned i = 0; i < 128; ++i) ASSERT_TRUE(budget.enter(reason));
  EXPECT_FALSE(budget.enter(reason));
  EXPECT_EQ(budget.used(Resource::Nesting), 128u);
  for (unsigned i = 0; i < 128; ++i) budget.leave();
  EXPECT_EQ(budget.used(Resource::Nesting), 0u);
  ASSERT_TRUE(budget.reserveProduct(Resource::Occurrences, 2048, 1, reason));
  ASSERT_TRUE(budget.reserveProduct(Resource::Occurrences, 2048, 1, reason));
  EXPECT_FALSE(budget.reserveProduct(Resource::Occurrences, 1, 1, reason));
  EXPECT_EQ(budget.used(Resource::Occurrences), 4096u);
}
// The descriptor observes already reserved storage. Its result may be used
// for an audit without a second ledger debit, even at exhausted Work capacity.
class TableQueryDescriptor : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder builder{&context};
  OwningOpRef<ModuleOp> module;
  Budget budget;
  DictionaryAttr origin, span;
  void SetUp() override {
    context.loadDialect<acir::ac::ACIRDialect>();
    module = ModuleOp::create(builder.getUnknownLoc());
    builder.setInsertionPointToEnd(module->getBody());
    auto site = builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "definition", FlatSymbolRefAttr::get(&context, "descriptor.Test")),
         builder.getNamedAttr("ast_path", builder.getArrayAttr({}))});
    origin = builder.getDictionaryAttr(
        {builder.getNamedAttr("site", site),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
    span = builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("descriptor.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
  }
  acir::ac::StaticExprAttr literal(uint64_t value) {
    auto number = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
         builder.getNamedAttr(
             "value",
             acir::ac::MathIntAttr::get(
                 &context, llvm::APSInt(llvm::APInt(65, value), false)))});
    return acir::ac::StaticExprAttr::get(
        &context,
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
             builder.getNamedAttr("value", number),
             builder.getNamedAttr("origin", origin),
             builder.getNamedAttr("location", span)}));
  }
  Type bits(uint64_t width) {
    return acir::ac::BitsType::get(&context, literal(width));
  }
  Operation *constant(uint64_t width) {
    OperationState state(builder.getUnknownLoc(),
                         acir::ac::BitsConstantOp::getOperationName());
    state.addTypes(bits(width));
    state.addAttribute("value", literal(0));
    return builder.create(state);
  }
  detail::TableQueryLowering lowering() {
    return detail::TableQueryLowering(builder, builder.getUnknownLoc(), *module,
                                      budget, {});
  }
};

TEST_F(TableQueryDescriptor, ReservedWideConstantAuditDoesNotDebitAnyCounter) {
  // Cross a physical word boundary and retain all three value/known/Z planes.
  auto *operation = constant(65);
  auto observer = lowering();
  Charge description;
  const auto empty = counters(budget);
  ASSERT_TRUE(observer.describeWithoutReservation(operation, 3, description));
  EXPECT_EQ(counters(budget), empty);
  EXPECT_EQ(description.amounts[static_cast<unsigned>(Resource::PayloadWords)],
            3u * 3u * 2u);
  EXPECT_EQ(description.amounts[static_cast<unsigned>(Resource::ConstantBytes)],
            2u * sizeof(uint64_t));
  std::string reason;
  ASSERT_TRUE(budget.reserve(description, reason));
  ASSERT_TRUE(budget.reserveProduct(
      Resource::Work,
      Budget::limit(Resource::Work) - budget.used(Resource::Work), 1, reason));
  const auto reserved = counters(budget);
  for (unsigned repeat = 0; repeat < 3; ++repeat) {
    Charge observed;
    ASSERT_TRUE(observer.describeWithoutReservation(operation, 3, observed));
    EXPECT_EQ(observed.amounts, description.amounts);
    EXPECT_EQ(counters(budget), reserved);
  }
}

TEST_F(TableQueryDescriptor, FailedDescriptionAndReservationKeepLedgerAtomic) {
  auto observer = lowering();
  std::string reason;
  ASSERT_TRUE(budget.reserveProduct(Resource::Occurrences, 7, 1, reason));
  const auto before = counters(budget);
  Charge zeroRows;
  EXPECT_FALSE(observer.describeWithoutReservation(constant(65), 0, zeroRows));
  EXPECT_EQ(counters(budget), before);
  // A type larger than the payload contract fails width observation without
  // materializing a payload or resetting prior source work.
  Charge oversized;
  EXPECT_FALSE(observer.describeWithoutReservation(
      constant(64 * (Budget::limit(Resource::PayloadWords) + 1)), 1,
      oversized));
  EXPECT_EQ(counters(budget), before);
  Charge valid;
  ASSERT_TRUE(observer.describeWithoutReservation(constant(129), 1, valid));
  ASSERT_TRUE(budget.reserveProduct(Resource::ConstantBytes,
                                    Budget::limit(Resource::ConstantBytes), 1,
                                    reason));
  const auto full = counters(budget);
  EXPECT_FALSE(budget.reserve(valid, reason));
  EXPECT_EQ(counters(budget), full);
  EXPECT_FALSE(reason.empty());
}
// Exercise the existing typed-stage owner. These hooks model fact-key lifetime;
// public source tests separately exercise importer authority/publication.
class NamedFixedStage : public TableQueryDescriptor {
protected:
  llvm::DenseSet<Value> facts, temporaryFacts;
  SmallVector<Value> forgotten;
  Operation *holder = nullptr;
  Block *scalar = nullptr;
  Value receiver, row, result;
  void makeTemplate(bool identity = false, bool splat = false) {
    auto table = acir::ac::TableType::get(
        &context, builder.getArrayAttr({literal(3)}), bits(8));
    receiver = module->getBody()->addArgument(table, builder.getUnknownLoc());
    facts.insert(receiver);
    OperationState state(builder.getUnknownLoc(), acir::ac::RuleOp::getOperationName());
    state.addRegion();
    holder = builder.create(state);
    scalar = new Block();
    holder->getRegion(0).push_back(scalar);
    row = scalar->addArgument(bits(8), builder.getUnknownLoc());
    facts.insert(row);
    auto insertion = builder.saveInsertionPoint();
    builder.setInsertionPointToEnd(scalar);
    if (identity) {
      result = row;
    } else {
      auto value = constant(8)->getResult(0);
      facts.insert(value);
      result = value;
      if (!splat) {
        OperationState binary(builder.getUnknownLoc(), acir::ac::BitsBinaryOp::getOperationName());
        binary.addOperands({row, value});
        binary.addTypes(bits(8));
        binary.addAttribute("opcode", builder.getStringAttr("add"));
        result = builder.create(binary)->getResult(0);
        facts.insert(result);
      }
    }
    builder.restoreInsertionPoint(insertion);
  }
  detail::TableQueryLowering stageOwner() {
    detail::TableQueryHooks hooks;
    hooks.literal = [&](uint64_t value) { return literal(value); };
    hooks.bits = [&](uint64_t width) { return bits(width); };
    hooks.copyFacts = [&](Value original, Value copied) {
      if (facts.contains(original)) facts.insert(copied);
      if (copied.getParentBlock() != scalar &&
          copied.getParentBlock()->getParentOp()->getName().getStringRef() ==
              acir::ac::RuleOp::getOperationName())
        temporaryFacts.insert(copied);
    };
    hooks.forgetFacts = [&](Value value) {
      forgotten.push_back(value);
      facts.erase(value);
      temporaryFacts.erase(value);
    };
    return detail::TableQueryLowering(builder, builder.getUnknownLoc(), *module,
                                      budget, std::move(hooks));
  }
  unsigned publishedMaps() {
    unsigned maps = 0;
    for (Operation &operation : *module->getBody())
      maps += isa<acir::ac::TableMapOp>(operation);
    return maps;
  }
  void expectCallerFacts() {
    EXPECT_TRUE(facts.contains(receiver));
    EXPECT_TRUE(facts.contains(row));
    for (Operation &operation : *scalar)
      for (Value value : operation.getResults()) EXPECT_TRUE(facts.contains(value));
    EXPECT_TRUE(temporaryFacts.empty());
  }
};

TEST_F(NamedFixedStage, PendingHoistCountsCopiesMappingsAndTemporaryCleanup) {
  makeTemplate();
  // T=2,a=1,R=4,V=15 and mapping bound 7T+2a+2=18.
  detail::TableQueryStageEnvelope envelope{2, 1, 4, 15, 18};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  ASSERT_TRUE(succeeded(output));
  ASSERT_EQ(output->size(), 1u);
  EXPECT_EQ(publishedMaps(), 1u);
  EXPECT_EQ(envelope.hoists, 1u);
  EXPECT_EQ(envelope.pending, 1u);
  EXPECT_EQ(envelope.inputs, 1u);
  EXPECT_EQ(envelope.captures, 1u);
  EXPECT_EQ(envelope.arguments, 3u);
  EXPECT_EQ(envelope.observedValues, 12u);
  EXPECT_EQ(envelope.mappingEntries, 12u);
  EXPECT_EQ(forgotten.size(), 4u); // ordinal, row, capture and scalar result
  expectCallerFacts();
  auto map = cast<acir::ac::TableMapOp>(output->front().getDefiningOp());
  for (Value value : map->getRegion(0).front().getArguments())
    EXPECT_FALSE(llvm::is_contained(forgotten, value));
  for (Operation &operation : map->getRegion(0).front())
    for (Value value : operation.getResults()) {
      EXPECT_TRUE(facts.contains(value));
      EXPECT_FALSE(llvm::is_contained(forgotten, value));
    }
}

TEST_F(NamedFixedStage, EmptyIdentityPublishesNoMapOrClone) {
  makeTemplate(true);
  detail::TableQueryStageEnvelope envelope{0, 1, 2, 5, 4};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  ASSERT_TRUE(succeeded(output));
  ASSERT_EQ(output->size(), 1u);
  EXPECT_EQ(output->front(), receiver);
  EXPECT_EQ(publishedMaps(), 0u);
  EXPECT_EQ(envelope.observedValues, 1u);
  EXPECT_EQ(envelope.mappingEntries, 0u);
  EXPECT_TRUE(forgotten.empty());
  expectCallerFacts();
}

TEST_F(NamedFixedStage, ConstantSplatHasOnlyOneHoist) {
  makeTemplate(false, true);
  detail::TableQueryStageEnvelope envelope{1, 1, 3, 10, 11};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  ASSERT_TRUE(succeeded(output));
  EXPECT_TRUE(isa<acir::ac::TableSplatOp>(output->front().getDefiningOp()));
  EXPECT_EQ(publishedMaps(), 0u);
  EXPECT_EQ(envelope.hoists, 1u);
  EXPECT_EQ(envelope.pending, 0u);
  EXPECT_EQ(envelope.observedValues, 3u);
  EXPECT_EQ(envelope.mappingEntries, 2u);
  expectCallerFacts();
}

TEST_F(NamedFixedStage, ExhaustedPublishedOperationReservationCleansTemporary) {
  makeTemplate();
  std::string reason;
  ASSERT_TRUE(budget.reserveProduct(Resource::Operations,
                                    Budget::limit(Resource::Operations) - 1,
                                    1, reason));
  detail::TableQueryStageEnvelope envelope{2, 1, 4, 15, 18};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  EXPECT_TRUE(failed(output));
  EXPECT_EQ(publishedMaps(), 0u);
  EXPECT_EQ(forgotten.size(), 4u);
  EXPECT_EQ(budget.used(Resource::Operations), Budget::limit(Resource::Operations));
  expectCallerFacts();
}

TEST_F(NamedFixedStage, FactEnvelopeFailureBeforePublicationCleansTemporary) {
  makeTemplate();
  detail::TableQueryStageEnvelope envelope{2, 1, 4, 8, 18};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  EXPECT_TRUE(failed(output));
  EXPECT_EQ(publishedMaps(), 0u);
  EXPECT_EQ(envelope.observedValues, 8u);
  EXPECT_EQ(forgotten.size(), 4u);
  expectCallerFacts();
}

TEST_F(NamedFixedStage, ScalarEnvelopeFailureCreatesNoStagedCopy) {
  makeTemplate();
  detail::TableQueryStageEnvelope envelope{0, 1, 4, 15, 18};
  auto owner = stageOwner();
  auto output = owner.stage(*scalar, ValueRange{row}, ValueRange{receiver},
                            ValueRange{result}, false, &envelope);
  EXPECT_TRUE(failed(output));
  EXPECT_EQ(publishedMaps(), 0u);
  EXPECT_EQ(envelope.mappingEntries, 0u);
  EXPECT_TRUE(forgotten.empty());
  expectCallerFacts();
}
} // namespace
