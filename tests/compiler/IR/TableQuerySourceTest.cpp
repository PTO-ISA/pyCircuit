// Independent AST/scope and private-budget invariants; public query execution
// belongs to the existing source-interface and Enum/collection owners.
#include "Compiler/PythonImportAST.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/TableQueryLowering.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/Support/raw_ostream.h"
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

TEST_F(TableQuerySourceBoundary, RejectsInvalidArityDefaultsAndAnnotations) {
  auto valid = arguments({argument()});
  rejects(lambda(arguments(), name("row")));
  rejects(lambda(arguments({argument(), argument("other")}), name("row")));
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
  auto imported = node("Import", {field("names", b.getArrayAttr({node("alias", {
      field("name", b.getStringAttr("pycircuit")), field("asname", b.getStringAttr("ac"))})}))});
  auto input = transport({imported, node("Expr", {field("value", callback)}),
                          node("Expr", {field("value", attribute("ac", "concat"))})});
  auto source = capture(*input);
  ASSERT_TRUE(succeeded(source));
  auto inside = source->module.item("body", 1).child("value").child("body").child("value");
  auto outside = source->module.item("body", 2).child("value").child("value");
  EXPECT_TRUE(detail::sourceBindingShadowed(*source, inside, "ac"));
  EXPECT_FALSE(detail::sourceBindingShadowed(*source, outside, "ac"));
  EXPECT_FALSE(detail::sourceBindingShadowed(*source, inside, "other"));
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
} // namespace
