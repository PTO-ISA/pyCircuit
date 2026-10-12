#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace acir::ac {
namespace {

// These are native intermediate-IR tests. They confer neither public source
// publication authority nor executable transaction support.
std::string nativeFixture() {
  std::string text = R"ir(
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @Child, ast_path = []}, expansion = []}, location = {path = "summary.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Child, ast_path = []}, expansion = []}, location = {path = "summary.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Child, ast_path = []}, expansion = []}, location = {path = "summary.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Child, ast_path = []}, expansion = []}, location = {path = "summary.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#span = {path = "summary.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#bit_constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
)ir";
  for (auto owner : {"Child", "Root"}) {
    std::string prefix = owner == std::string("Child") ? "c" : "r";
    for (unsigned i = 0; i != 24; ++i)
      text += "#" + prefix + std::to_string(i) + " = {site = {definition = @" +
              owner +
              ", ast_path = [{kind = \"index\", value = " + std::to_string(i) +
              " : i64}]}, expansion = []}\n";
  }
  text += R"ir(
module {
  ac.struct "Packet" fields [{name = "lanes", type = !lanes}]
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %flag: !b1, %seed: !b8, %index: !b8):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %no = "ac.bits.constant"() {value = #w0} : () -> !b1
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
    %initial_lanes = "ac.table.splat"(%initial) {shape = [#w2]} : (!b8) -> !lanes
    %initial_packet = "ac.struct.create"(%initial_lanes) : (!lanes) -> !ac.struct<"Packet">
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #c0} : (!b1, !b1, !b8) -> !b8
    %packet_q = "ac.state"(%clk, %rst, %initial_packet) {instance_name = "packet", occurrence = #c1} : (!b1, !b1, !ac.struct<"Packet">) -> !ac.struct<"Packet">
    "ac.method"(%rst, %yes, %flag, %seed, %initial) ({
    ^bb0(%allow: !b1, %value: !b8, %slot: !b8, %reset: !b1, %always: !b1, %captured_flag: !b1, %captured_seed: !b8, %captured_initial: !b8):
      %guard_read = "ac.state.read"(%allow) {state = "count", selectors = [], occurrence = #c3} : (!b1) -> !b8
      "ac.yield"(%always) : (!b1) -> ()
    }, {
    ^bb0(%allow: !b1, %value: !b8, %slot: !b8, %reset: !b1, %always: !b1, %captured_flag: !b1, %captured_seed: !b8, %captured_initial: !b8):
      %old = "ac.state.read"(%allow) {state = "count", selectors = [], occurrence = #c4} : (!b1) -> !b8
      "ac.state.write"(%allow, %value) {state = "count", selectors = [], occurrence = #c5} : (!b1, !b8) -> ()
      %called = "ac.method.call"(%allow, %value) {callee = @Child::@relay, instance_path = [], occurrence = #c6} : (!b1, !b8) -> !b8
      %lane = "ac.state.read"(%allow, %slot) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #c7} : (!b1, !b8) -> !b8
      "ac.state.write"(%allow, %lane, %slot) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #c8} : (!b1, !b8, !b8) -> ()
      %false = "ac.bits.constant"() {value = #w0} : () -> !b1
      %unused = "ac.method.call"(%false, %old) {callee = @Child::@relay, instance_path = [], occurrence = #c9} : (!b1, !b8) -> !b8
      %sum = "ac.bits.binary"(%value, %captured_seed) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.yield"(%sum) : (!b8) -> ()
    }) {sym_name = "act", function_type = (!b1, !b8, !b8) -> !b8, argument_names = ["allow", "value", "index"], ac.parameters = [{name = "allow", binding = "positional_or_keyword", constraint = #bit_constraint, default = {present = false}, origin = #c2, location = #span}, {name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c2, location = #span}, {name = "index", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c2, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", occurrence = #c2} : (!b1, !b1, !b1, !b8, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%value: !b8, %always: !b1):
      "ac.yield"(%always) : (!b1) -> ()
    }, {
    ^bb0(%value: !b8, %always: !b1):
      "ac.yield"(%value) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["value"], ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c10, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", occurrence = #c10} : (!b1) -> ()
    "ac.transaction"(%clk, %no, %yes) ({
    ^bb0(%always: !b1):
      "ac.yield"(%always) : (!b1) -> ()
    }, {
    ^bb0(%always: !b1):
      %unused = "ac.state.read"(%always) {state = "count", selectors = [], occurrence = #c13} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "inspect", declaration = #c11, occurrence = #c12} : (!b1, !b1, !b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "summary.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b8) -> !b8, input_names = ["clk", "rst", "flag", "seed", "index"], output_names = ["count"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %flag: !b1, %seed: !b8, %index: !b8):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b8
    %status = "ac.state"(%clk, %rst, %zero) {instance_name = "status", occurrence = #r0} : (!b1, !b1, !b8) -> !b8
    %left = "ac.instance"(%clk, %rst, %flag, %seed, %index) {instance_name = "left", callee = @Child, parameters = [], type_arguments = [], occurrence = #r1} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    %right = "ac.instance"(%clk, %rst, %flag, %seed, %index) {instance_name = "right", callee = @Child, parameters = [], type_arguments = [], occurrence = #r2} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    %answer = "ac.transaction"(%clk, %rst, %yes, %seed, %index) ({
    ^bb0(%path: !b1, %value: !b8, %slot: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %value: !b8, %slot: !b8):
      %a = "ac.method.call"(%path, %path, %value, %slot) {callee = @Child::@act, instance_path = ["left"], occurrence = #r5} : (!b1, !b1, !b8, !b8) -> !b8
      %again = "ac.method.call"(%path, %a) {callee = @Child::@relay, instance_path = ["left"], occurrence = #r6} : (!b1, !b8) -> !b8
      %b = "ac.method.call"(%path, %path, %again, %slot) {callee = @Child::@act, instance_path = ["right"], occurrence = #r7} : (!b1, !b1, !b8, !b8) -> !b8
      "ac.yield"(%b) : (!b8) -> ()
    }) {name = "run", declaration = #r3, occurrence = #r4} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    "ac.transaction"(%clk, %rst, %yes, %left) ({
    ^bb0(%path: !b1, %unused_child_result: !b8):
      %guard_read = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r10} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %unused_child_result: !b8):
      %unused = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r11} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #r8, occurrence = #r9} : (!b1, !b1, !b1, !b8) -> ()
    "ac.yield"(%answer) : (!b8) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "summary.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b8) -> !b8, input_names = ["clk", "rst", "flag", "seed", "index"], output_names = ["answer"]} : () -> ()
}
)ir";
  return text;
}

using Step = std::variant<std::string, unsigned>;
using Path = std::vector<Step>;

class MethodSummaryAuthorityTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OpBuilder builder{&context};
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ACIRDialect>(); }
  template <typename Function> auto capture(Function function) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &error) {
          llvm::raw_string_ostream stream(diagnostics);
          error.print(stream);
          return mlir::success();
        });
    return function();
  }
  mlir::OwningOpRef<mlir::ModuleOp> parse() {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        nativeFixture(), mlir::ParserConfig(&context, false));
    EXPECT_TRUE(file);
    if (file)
      EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
    return file;
  }
  ModuleOp module(mlir::ModuleOp file, llvm::StringRef name) {
    for (auto definition : file.getOps<ModuleOp>())
      if (definition.getSymName() == name)
        return definition;
    ADD_FAILURE() << "missing module " << name.str();
    return {};
  }
  template <typename Op> Op named(ModuleOp definition, llvm::StringRef name) {
    for (auto operation : definition.getBody().front().getOps<Op>()) {
      auto attribute = operation->template getAttrOfType<mlir::StringAttr>(
          std::is_same_v<Op, SourceMethodOp> ? "sym_name" : "name");
      if (attribute && attribute.getValue() == name)
        return operation;
    }
    ADD_FAILURE() << "missing operation " << name.str();
    return {};
  }
  mlir::DictionaryAttr dict(mlir::Attribute value) {
    return mlir::cast<mlir::DictionaryAttr>(value);
  }
  mlir::ArrayAttr array(mlir::DictionaryAttr value, llvm::StringRef field) {
    return mlir::cast<mlir::ArrayAttr>(value.get(field));
  }
  mlir::DictionaryAttr row(mlir::DictionaryAttr value, llvm::StringRef field,
                           unsigned ordinal = 0) {
    return dict(array(value, field)[ordinal]);
  }
  mlir::DictionaryAttr namedRow(mlir::DictionaryAttr summary,
                                llvm::StringRef section, llvm::StringRef name) {
    for (auto value : array(summary, section)) {
      auto entry = dict(value);
      if (entry.getAs<mlir::StringAttr>("name").getValue() == name)
        return entry;
    }
    ADD_FAILURE() << "missing summary row " << name.str();
    return {};
  }
  mlir::DictionaryAttr changed(mlir::DictionaryAttr value, llvm::StringRef name,
                               mlir::Attribute replacement) {
    mlir::NamedAttrList fields(value);
    if (replacement)
      fields.set(name, replacement);
    else
      fields.erase(name);
    return fields.getDictionary(&context);
  }
  mlir::Attribute at(mlir::Attribute value, const Path &path) {
    for (const auto &step : path) {
      if (auto *name = std::get_if<std::string>(&step))
        value = dict(value).get(*name);
      else
        value = mlir::cast<mlir::ArrayAttr>(value)[std::get<unsigned>(step)];
    }
    return value;
  }
  mlir::Attribute replace(mlir::Attribute value, const Path &path,
                          mlir::Attribute replacement, unsigned depth = 0) {
    if (depth == path.size())
      return replacement;
    if (auto *name = std::get_if<std::string>(&path[depth])) {
      auto dictionary = dict(value);
      return changed(
          dictionary, *name,
          replace(dictionary.get(*name), path, replacement, depth + 1));
    }
    auto values = mlir::cast<mlir::ArrayAttr>(value);
    llvm::SmallVector<mlir::Attribute> elements(values.begin(), values.end());
    auto index = std::get<unsigned>(path[depth]);
    elements[index] = replace(elements[index], path, replacement, depth + 1);
    return builder.getArrayAttr(elements);
  }
  void dictionaries(mlir::Attribute value, Path path,
                    std::vector<std::pair<Path, mlir::DictionaryAttr>> &out) {
    if (auto dictionary = mlir::dyn_cast<mlir::DictionaryAttr>(value)) {
      out.emplace_back(path, dictionary);
      for (auto field : dictionary) {
        auto child = path;
        child.emplace_back(field.getName().str());
        dictionaries(field.getValue(), std::move(child), out);
      }
    } else if (auto values = mlir::dyn_cast<mlir::ArrayAttr>(value)) {
      for (unsigned i = 0; i != values.size(); ++i) {
        auto child = path;
        child.emplace_back(i);
        dictionaries(values[i], std::move(child), out);
      }
    }
  }
  mlir::DictionaryAttr summary(SourceMethodSummariesAnalysis &analysis,
                               ModuleOp definition) {
    auto result =
        capture([&] { return analysis.getPublicationSummary(definition); });
    EXPECT_TRUE(mlir::succeeded(result)) << diagnostics;
    return mlir::succeeded(result) ? *result : mlir::DictionaryAttr{};
  }
  void structureAccepts(mlir::Operation *declaration,
                        mlir::DictionaryAttr value) {
    EXPECT_TRUE(mlir::succeeded(capture([&] {
      return verifySourceTransactionSummary(declaration, value);
    }))) << diagnostics;
  }
  void structureRejects(mlir::Operation *declaration,
                        mlir::DictionaryAttr value) {
    EXPECT_TRUE(mlir::failed(capture(
        [&] { return verifySourceTransactionSummary(declaration, value); })));
    EXPECT_FALSE(diagnostics.empty());
  }
  void bodyRejects(ModuleOp definition, mlir::DictionaryAttr value,
                   SourceMethodSummariesAnalysis &analysis) {
    structureAccepts(definition, value);
    EXPECT_TRUE(mlir::failed(capture([&] {
      return verifyPublishedTransactionSummary(definition, value, analysis);
    })));
    EXPECT_FALSE(diagnostics.empty());
  }
  std::string kind(mlir::DictionaryAttr value) {
    return value.getAs<mlir::StringAttr>("kind").getValue().str();
  }
  void known(mlir::DictionaryAttr value, llvm::ArrayRef<int64_t> arguments,
             llvm::ArrayRef<int64_t> captures) {
    auto dependencies = dict(value.get("dependencies"));
    EXPECT_EQ(kind(dependencies), "known");
    for (auto [field, expected] :
         {std::pair<llvm::StringRef, llvm::ArrayRef<int64_t>>("arguments",
                                                              arguments),
          std::pair<llvm::StringRef, llvm::ArrayRef<int64_t>>("captures",
                                                              captures)}) {
      auto entries = array(dependencies, field);
      ASSERT_EQ(entries.size(), expected.size()) << field.str();
      for (unsigned i = 0; i != entries.size(); ++i)
        EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(entries[i]).getInt(),
                  expected[i]);
    }
  }
  void unknown(mlir::DictionaryAttr value) {
    EXPECT_EQ(kind(dict(value.get("fact"))), "unknown");
    EXPECT_EQ(kind(dict(value.get("dependencies"))), "unknown");
  }
  mlir::DictionaryAttr constantFact(bool value) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("constant")),
         builder.getNamedAttr("value", builder.getBoolAttr(value))});
  }
  mlir::DictionaryAttr occurrence(llvm::StringRef definition,
                                  unsigned ordinal) {
    auto index = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("index")),
         builder.getNamedAttr("value", builder.getI64IntegerAttr(ordinal))});
    auto site = builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "definition", mlir::FlatSymbolRefAttr::get(&context, definition)),
         builder.getNamedAttr("ast_path", builder.getArrayAttr({index}))});
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("site", site),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  }
};

TEST_F(MethodSummaryAuthorityTest, CanonicalDirectOwnersAndChildComposition) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto root = module(*file, "Root");
  auto child = module(*file, "Child");
  auto published = summary(analysis, root);
  auto childSummary = summary(analysis, child);
  ASSERT_TRUE(published);
  ASSERT_TRUE(childSummary);
  structureAccepts(root, published);
  structureAccepts(child, childSummary);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyPublishedTransactionSummary(root, published, analysis);
  }))) << diagnostics;
  EXPECT_EQ(published, summary(analysis, root));

  auto owners = array(published, "owners");
  ASSERT_EQ(owners.size(), 3u);
  EXPECT_EQ(kind(dict(owners[0])), "state");
  EXPECT_EQ(dict(owners[0]).get("name"), builder.getStringAttr("status"));
  auto left = dict(owners[1]), right = dict(owners[2]);
  EXPECT_EQ(left.get("name"), builder.getStringAttr("left"));
  EXPECT_EQ(right.get("name"), builder.getStringAttr("right"));
  EXPECT_EQ(left.get("callee"),
            mlir::FlatSymbolRefAttr::get(&context, "Child"));
  EXPECT_EQ(left.get("callee"), right.get("callee"));
  EXPECT_NE(left.get("occurrence"), right.get("occurrence"));
  auto state = dict(dict(owners[0]).get("state"));
  EXPECT_EQ(kind(state), "owned");
  EXPECT_TRUE(array(state, "element").empty());
  EXPECT_EQ(state.get("declaration"), occurrence("Root", 0));
  EXPECT_TRUE(array(published, "methods").empty());
  ASSERT_EQ(array(published, "autonomous").size(), 2u);
  ASSERT_EQ(array(childSummary, "autonomous").size(), 1u);
  // Child autonomous behavior is available through the two distinct edges,
  // never copied into the parent's direct summary.
  EXPECT_EQ(namedRow(childSummary, "autonomous", "inspect").get("registration"),
            occurrence("Child", 12));
  EXPECT_NE(
      namedRow(childSummary, "autonomous", "inspect").get("declaration"),
      namedRow(childSummary, "autonomous", "inspect").get("registration"));
  auto childDomain =
      dict(namedRow(childSummary, "autonomous", "inspect").get("domain"));
  EXPECT_EQ(kind(dict(childDomain.get("clock"))), "input");
  EXPECT_EQ(dict(childDomain.get("clock")).get("ordinal"),
            builder.getI64IntegerAttr(0));
  EXPECT_EQ(dict(childDomain.get("reset")), constantFact(false));
  for (auto transaction : root.getBody().front().getOps<SourceTransactionOp>())
    EXPECT_TRUE(analysis.isTotal(transaction));
}

TEST_F(MethodSummaryAuthorityTest,
       CapturesPredicatesAndMayDependenciesKeepTheirScope) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto childSummary = summary(analysis, module(*file, "Child"));
  auto rootSummary = summary(analysis, module(*file, "Root"));
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  auto act = namedRow(childSummary, "methods", "act");
  auto captures = array(act, "captures");
  ASSERT_EQ(captures.size(), 5u);
  auto reset = dict(dict(captures[0]).get("value"));
  known(reset, {1}, {});
  EXPECT_EQ(kind(dict(reset.get("fact"))), "argument");
  auto flag = dict(dict(captures[2]).get("value"));
  known(flag, {2}, {});
  EXPECT_EQ(dict(flag.get("fact")).get("ordinal"),
            builder.getI64IntegerAttr(2));
  auto literal = dict(dict(captures[1]).get("value"));
  known(literal, {}, {});
  EXPECT_EQ(dict(literal.get("fact")), constantFact(true));
  auto wideLiteral = dict(dict(captures[4]).get("value"));
  known(wideLiteral, {}, {});
  EXPECT_EQ(kind(dict(wideLiteral.get("fact"))), "unknown");
  auto guard = dict(act.get("guard"));
  known(guard, {}, {1});
  EXPECT_EQ(dict(guard.get("fact")), constantFact(true));
  auto result = row(act, "results");
  known(result, {1}, {3});
  EXPECT_EQ(kind(dict(result.get("fact"))), "unknown");
  auto relay = namedRow(childSummary, "methods", "relay");
  known(row(relay, "results"), {0}, {});
  EXPECT_EQ(kind(dict(row(relay, "results").get("fact"))), "unknown");
  auto run = namedRow(rootSummary, "autonomous", "run");
  unknown(row(run, "results"));
  known(dict(run.get("guard")), {}, {0});
  auto watch = namedRow(rootSummary, "autonomous", "watch");
  ASSERT_EQ(array(watch, "captures").size(), 2u);
  unknown(dict(row(watch, "captures", 1).get("value")));
  EXPECT_TRUE(array(watch, "results").empty());
  EXPECT_TRUE(array(watch, "result_types").empty());
  auto instance = namedRow(rootSummary, "owners", "left");
  auto actuals = array(instance, "arguments");
  ASSERT_EQ(actuals.size(), 5u);
  for (unsigned i = 0; i != 5; ++i) {
    known(dict(actuals[i]), {int64_t(i)}, {});
    EXPECT_EQ(kind(dict(dict(actuals[i]).get("fact"))),
              i < 3 ? "argument" : "unknown");
  }
}

TEST_F(MethodSummaryAuthorityTest,
       EffectsKeepExactOrderWildcardIndicesAndFalsePaths) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, module(*file, "Child"));
  ASSERT_TRUE(published);
  auto act = namedRow(published, "methods", "act");
  auto guards = array(act, "guard_effects");
  ASSERT_EQ(guards.size(), 1u);
  EXPECT_EQ(kind(dict(guards[0])), "read");
  EXPECT_EQ(dict(guards[0]).get("occurrence"), occurrence("Child", 3));
  auto effects = array(act, "body_effects");
  ASSERT_EQ(effects.size(), 6u);
  const char *expected[] = {"read", "write", "call", "read", "write", "call"};
  for (unsigned i = 0; i != effects.size(); ++i) {
    auto effect = dict(effects[i]);
    EXPECT_EQ(kind(effect), expected[i]);
    EXPECT_EQ(effect.get("occurrence"), occurrence("Child", i + 4));
  }
  auto firstPath = dict(dict(effects[0]).get("path"));
  EXPECT_EQ(kind(firstPath), "argument");
  EXPECT_EQ(firstPath.get("ordinal"), builder.getI64IntegerAttr(0));
  EXPECT_EQ(dict(dict(effects[5]).get("path")), constantFact(false));
  auto selectors = array(dict(effects[3]), "selectors");
  ASSERT_EQ(selectors.size(), 2u);
  EXPECT_EQ(kind(dict(selectors[0])), "field");
  EXPECT_EQ(dict(selectors[0]).get("name"), builder.getStringAttr("lanes"));
  EXPECT_EQ(kind(dict(selectors[1])), "any_element");
  auto method = named<SourceMethodOp>(module(*file, "Child"), "act");
  EXPECT_EQ(dict(selectors[1]).get("index_type"),
            mlir::TypeAttr::get(method.getFunctionType().getInput(2)));
  known(dict(dict(selectors[1]).get("index")), {2}, {});
  EXPECT_EQ(kind(dict(dict(dict(selectors[1]).get("index")).get("fact"))),
            "unknown");
  auto call = dict(effects[2]);
  EXPECT_TRUE(array(dict(call.get("receiver")), "instance_path").empty());
  auto firstCall =
      *method.getBody().front().getOps<SourceMethodCallOp>().begin();
  EXPECT_EQ(call.get("callee"), firstCall.getCalleeAttr());
  known(row(call, "arguments"), {1}, {});
  unknown(row(dict(effects[5]), "arguments"));
  auto rootSummary = summary(analysis, module(*file, "Root"));
  ASSERT_TRUE(rootSummary);
  auto run = namedRow(rootSummary, "autonomous", "run");
  auto rootEffects = array(run, "body_effects");
  ASSERT_EQ(rootEffects.size(), 3u);
  auto receiver = [&](unsigned i) {
    return dict(dict(rootEffects[i]).get("receiver"));
  };
  EXPECT_EQ(receiver(0), receiver(1));
  EXPECT_NE(receiver(0), receiver(2));
  EXPECT_EQ(array(receiver(0), "instance_path")[0], occurrence("Root", 1));
  EXPECT_EQ(array(receiver(2), "instance_path")[0], occurrence("Root", 2));
  known(row(dict(rootEffects[0]), "arguments", 0), {}, {0});
  known(row(dict(rootEffects[0]), "arguments", 1), {}, {1});
  known(row(dict(rootEffects[0]), "arguments", 2), {}, {2});
  unknown(row(dict(rootEffects[1]), "arguments"));
}

TEST_F(MethodSummaryAuthorityTest,
       PrintedSummaryRoundTripsAndIsDeterministicAcrossAnalyses) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis first(file->getOperation());
  SourceMethodSummariesAnalysis second(file->getOperation());
  ASSERT_TRUE(first.isValid());
  ASSERT_TRUE(second.isValid());
  for (auto name : {"Root", "Child"}) {
    auto definition = module(*file, name);
    auto published = summary(first, definition);
    ASSERT_TRUE(published);
    EXPECT_EQ(published, summary(second, definition));
    std::string spelling;
    llvm::raw_string_ostream stream(spelling);
    published.print(stream);
    auto reparsed = mlir::parseAttribute(spelling, &context);
    ASSERT_TRUE(reparsed);
    EXPECT_EQ(published, reparsed);
    structureAccepts(definition, dict(reparsed));
  }
}

TEST_F(MethodSummaryAuthorityTest,
       EveryDictionaryVariantHasAClosedMandatorySchema) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  for (auto name : {"Root", "Child"}) {
    auto definition = module(*file, name);
    auto published = summary(analysis, definition);
    ASSERT_TRUE(published);
    std::vector<std::pair<Path, mlir::DictionaryAttr>> records;
    dictionaries(published, {}, records);
    ASSERT_FALSE(records.empty());
    for (auto &[path, record] : records) {
      for (auto field : record) {
        SCOPED_TRACE(name);
        SCOPED_TRACE(field.getName().str());
        structureRejects(definition,
                         dict(replace(published, path,
                                      changed(record, field.getName(), {}))));
        structureRejects(definition,
                         dict(replace(published, path,
                                      changed(record, field.getName(),
                                              builder.getUnitAttr()))));
      }
      structureRejects(definition,
                       dict(replace(published, path,
                                    changed(record, "unexpected_field",
                                            builder.getUnitAttr()))));
      if (record.getAs<mlir::StringAttr>("kind"))
        structureRejects(
            definition, dict(replace(published, path,
                                     changed(record, "kind",
                                             builder.getStringAttr(
                                                 "unsupported_record_kind")))));
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       StructurallyValidChangesNeedExactBodyAuthority) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto child = module(*file, "Child"), root = module(*file, "Root");
  auto childSummary = summary(analysis, child);
  auto rootSummary = summary(analysis, root);
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  const std::vector<std::pair<Path, mlir::Attribute>> childChanges = {
      {{std::string("methods"), 0u, std::string("guard"), std::string("fact")},
       constantFact(false)},
      {{std::string("owners"), 0u, std::string("domain"), std::string("clock"),
        std::string("ordinal")},
       builder.getI64IntegerAttr(1)},
      {{std::string("methods"), 0u, std::string("captures"), 2u,
        std::string("value"), std::string("dependencies"),
        std::string("arguments")},
       builder.getArrayAttr({builder.getI64IntegerAttr(1)})},
      {{std::string("methods"), 0u, std::string("results"), 0u,
        std::string("dependencies"), std::string("captures")},
       builder.getArrayAttr({builder.getI64IntegerAttr(4)})},
      {{std::string("methods"), 0u, std::string("body_effects"), 1u,
        std::string("value"), std::string("dependencies"),
        std::string("arguments")},
       builder.getArrayAttr({builder.getI64IntegerAttr(2)})},
      {{std::string("methods"), 0u, std::string("body_effects"), 0u,
        std::string("path")},
       constantFact(false)}};
  for (const auto &[path, replacement] : childChanges)
    bodyRejects(child, dict(replace(childSummary, path, replacement)),
                analysis);
  auto effectsPath =
      Path{std::string("methods"), 0u, std::string("body_effects")};
  auto effects = mlir::cast<mlir::ArrayAttr>(at(childSummary, effectsPath));
  llvm::SmallVector<mlir::Attribute> reordered(effects.begin(), effects.end());
  std::swap(reordered[0], reordered[1]);
  bodyRejects(
      child,
      dict(replace(childSummary, effectsPath, builder.getArrayAttr(reordered))),
      analysis);
  auto receiverPath =
      Path{std::string("autonomous"),   0u,
           std::string("body_effects"), 0u,
           std::string("receiver"),     std::string("instance_path")};
  bodyRejects(root,
              dict(replace(rootSummary, receiverPath,
                           builder.getArrayAttr({occurrence("Root", 2)}))),
              analysis);
  auto instanceActual = Path{std::string("owners"),    1u,
                             std::string("arguments"), 0u,
                             std::string("fact"),      std::string("ordinal")};
  bodyRejects(
      root,
      dict(replace(rootSummary, instanceActual, builder.getI64IntegerAttr(1))),
      analysis);
}

TEST_F(MethodSummaryAuthorityTest,
       ShapeCheckingUsesDeclarationsWithoutProviderBodies) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto child = module(*file, "Child");
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  mlir::OperationState declaration(child.getLoc(),
                                   ModuleImportOp::getOperationName());
  declaration.addAttributes(child->getAttrs());
  // A bodyless declaration is a real native IR operation, not an importer or
  // a substitute for body authority. It need not provide closure proofs here.
  declaration.addAttribute(
      "dependency_summary",
      mlir::parseAttribute(
          "[{output = {port = 0 : i64, path = []}, inputs = []}]", &context));
  builder.setInsertionPoint(child);
  auto *imported = builder.create(declaration);
  child->remove();
  structureAccepts(imported, published);
  // No provider exists in the package at this point. Shape checking still
  // validates local owner, source metadata, nominal type, and ordinal scopes.
  auto invalid =
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("results"), 0u,
                    std::string("dependencies"), std::string("arguments")},
                   builder.getArrayAttr({builder.getI64IntegerAttr(99)})));
  structureRejects(imported, invalid);
  file->getBody()->getOperations().insert(imported->getIterator(),
                                          child.getOperation());
  imported->erase();
}

TEST_F(MethodSummaryAuthorityTest,
       LogicalOneBitKindsArePreservedWithoutGuessing) {
  for (auto sourceKind : {"fixed_bits", "boolean", "integer"}) {
    SCOPED_TRACE(sourceKind);
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto relay = named<SourceMethodOp>(child, "relay");
    auto bit = child.getBody().front().getArgument(0).getType();
    auto *copy = relay->clone();
    copy->setAttr("sym_name", builder.getStringAttr("predicate"));
    copy->setAttr("occurrence", occurrence("Child", 14));
    copy->setAttr("function_type", mlir::TypeAttr::get(mlir::FunctionType::get(
                                       &context, {bit}, {bit})));
    for (auto &region : copy->getRegions())
      region.front().getArgument(0).setType(bit);
    mlir::NamedAttrList constraint;
    constraint.set("kind", builder.getStringAttr("hardware"));
    constraint.set("type", mlir::TypeAttr::get(bit));
    constraint.set("source_kind", builder.getStringAttr(sourceKind));
    if (std::string(sourceKind) == "boolean")
      constraint.set(
          "domain", mlir::parseAttribute("#ac.source_domain<{kind = \"bool\"}>",
                                         &context));
    if (std::string(sourceKind) == "integer")
      constraint.set(
          "domain",
          mlir::parseAttribute("#ac.source_domain<{kind = \"integer\", lower = "
                               "#ac.math_int<0>, upper = #ac.math_int<2>}>",
                               &context));
    auto oldParameter = mlir::cast<mlir::DictionaryAttr>(
        relay->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
    auto newParameter =
        changed(oldParameter, "constraint", constraint.getDictionary(&context));
    newParameter = changed(newParameter, "origin", occurrence("Child", 14));
    copy->setAttr("ac.parameters", builder.getArrayAttr({newParameter}));
    copy->setAttr("ac.result_constraints",
                  builder.getArrayAttr({constraint.getDictionary(&context)}));
    child.getBody().front().getOperations().insert(relay->getIterator(), copy);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    auto published = summary(analysis, child);
    ASSERT_TRUE(published);
    auto predicate = namedRow(published, "methods", "predicate");
    EXPECT_EQ(row(predicate, "ac.parameters").get("constraint"),
              constraint.getDictionary(&context));
    EXPECT_EQ(array(predicate, "ac.result_constraints")[0],
              constraint.getDictionary(&context));
    auto returned = row(predicate, "results");
    EXPECT_EQ(kind(dict(returned.get("fact"))), "argument");
    EXPECT_EQ(dict(returned.get("fact")).get("ordinal"),
              builder.getI64IntegerAttr(0));
    known(returned, {0}, {});
    structureAccepts(child, published);
    // A same-width, independently valid source kind remains a different
    // provider contract and must be caught by exact rederivation.
    auto otherConstraint =
        changed(constraint.getDictionary(&context), "source_kind",
                builder.getStringAttr("fixed_bits"));
    otherConstraint = changed(otherConstraint, "domain", {});
    if (std::string(sourceKind) != "fixed_bits")
      bodyRejects(child,
                  dict(replace(published,
                               {std::string("methods"), 1u,
                                std::string("ac.result_constraints"), 0u},
                               otherConstraint)),
                  analysis);
  }
}

TEST_F(MethodSummaryAuthorityTest,
       CaptureArgumentsNeverBecomeMethodFormalsOrStateProofs) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto relay = named<SourceMethodOp>(child, "relay");
  auto bit = child.getBody().front().getArgument(0).getType();
  // Keep the total guard, but make the return a captured module predicate.
  // Module input ordinal 2 must not turn into Method formal ordinal 2.
  auto *predicate = relay->clone();
  predicate->setAttr("sym_name", builder.getStringAttr("captured_predicate"));
  predicate->setAttr("occurrence", occurrence("Child", 14));
  predicate->setAttr(
      "function_type",
      mlir::TypeAttr::get(mlir::FunctionType::get(&context, {bit}, {bit})));
  auto constraint = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
       builder.getNamedAttr("type", mlir::TypeAttr::get(bit)),
       builder.getNamedAttr("source_kind",
                            builder.getStringAttr("fixed_bits"))});
  auto parameter =
      changed(dict(relay->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]),
              "constraint", constraint);
  parameter = changed(parameter, "origin", occurrence("Child", 14));
  predicate->setAttr("ac.parameters", builder.getArrayAttr({parameter}));
  predicate->setAttr("ac.result_constraints",
                     builder.getArrayAttr({constraint}));
  llvm::SmallVector<mlir::Value> captures(predicate->getOperands());
  captures.push_back(child.getBody().front().getArgument(2));
  predicate->setOperands(captures);
  for (auto &region : predicate->getRegions()) {
    region.front().getArgument(0).setType(bit);
    region.front().addArgument(bit, predicate->getLoc());
  }
  predicate->getRegion(1).front().getTerminator()->setOperand(
      0, predicate->getRegion(1).front().getArgument(2));
  child.getBody().front().getOperations().insert(relay->getIterator(),
                                                 predicate);
  auto act = named<SourceMethodOp>(child, "act");
  auto state = *child.getBody().front().getOps<SourceStateOp>().begin();
  act->setOperand(4, state.getQ());
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto captured = namedRow(published, "methods", "captured_predicate");
  auto result = row(captured, "results");
  known(result, {}, {1});
  EXPECT_EQ(kind(dict(result.get("fact"))), "unknown");
  auto inputCapture = dict(row(captured, "captures", 1).get("value"));
  known(inputCapture, {2}, {});
  EXPECT_EQ(dict(inputCapture.get("fact")).get("ordinal"),
            builder.getI64IntegerAttr(2));
  unknown(dict(
      row(namedRow(published, "methods", "act"), "captures", 4).get("value")));
}

TEST_F(MethodSummaryAuthorityTest,
       LiteralTableIndexStillPublishesWildcardAndFullType) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto &body = act.getBody().front();
  unsigned reads = 0;
  for (auto read : body.getOps<SourceStateReadOp>())
    if (++reads == 2)
      read->setOperand(1, body.getArgument(7));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto effect = row(namedRow(published, "methods", "act"), "body_effects", 3);
  auto selector = row(effect, "selectors", 1);
  EXPECT_EQ(kind(selector), "any_element");
  EXPECT_EQ(selector.get("index_type"),
            mlir::TypeAttr::get(body.getArgument(7).getType()));
  auto index = dict(selector.get("index"));
  known(index, {}, {4});
  EXPECT_EQ(kind(dict(index.get("fact"))), "unknown");
}

TEST_F(MethodSummaryAuthorityTest,
       CanonicalOrderUsesIdentityRatherThanIRPlacement) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  SourceMethodSummariesAnalysis before(file->getOperation());
  ASSERT_TRUE(before.isValid());
  auto childSummary = summary(before, child),
       rootSummary = summary(before, root);
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  auto firstState = *child.getBody().front().getOps<SourceStateOp>().begin();
  auto secondState =
      *std::next(child.getBody().front().getOps<SourceStateOp>().begin());
  secondState->moveBefore(firstState);
  named<SourceMethodOp>(child, "relay")
      ->moveBefore(named<SourceMethodOp>(child, "act"));
  auto firstInstance = *root.getBody().front().getOps<InstanceOp>().begin();
  auto secondInstance =
      *std::next(root.getBody().front().getOps<InstanceOp>().begin());
  secondInstance->moveBefore(firstInstance);
  named<SourceTransactionOp>(root, "watch")
      ->moveBefore(named<SourceTransactionOp>(root, "run"));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  // Rebuild the owning analysis whenever the body changes.
  SourceMethodSummariesAnalysis after(file->getOperation());
  ASSERT_TRUE(after.isValid());
  EXPECT_EQ(childSummary, summary(after, child));
  EXPECT_EQ(rootSummary, summary(after, root));
}

TEST_F(MethodSummaryAuthorityTest,
       BodyMutationInvalidatesPreviouslyPublishedFacts) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  mlir::DictionaryAttr published;
  {
    SourceMethodSummariesAnalysis original(file->getOperation());
    ASSERT_TRUE(original.isValid());
    published = summary(original, child);
  }
  ASSERT_TRUE(published);
  auto act = named<SourceMethodOp>(child, "act");
  for (auto operation : act.getBody().front().getOps<BitsBinaryOp>())
    operation->setOperand(1, act.getBody().front().getArgument(1));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis refreshed(file->getOperation());
  ASSERT_TRUE(refreshed.isValid());
  bodyRejects(child, published, refreshed);
  auto current = summary(refreshed, child);
  ASSERT_TRUE(current);
  known(row(namedRow(current, "methods", "act"), "results"), {1}, {});
  EXPECT_NE(current, published);
}

TEST_F(MethodSummaryAuthorityTest,
       UnrepresentedDomainsDoNotChangeTotalityAdmission) {
  for (bool stateDomain : {false, true}) {
    SCOPED_TRACE(stateDomain);
    auto file = parse();
    ASSERT_TRUE(file);
    auto root = module(*file, "Root");
    auto transaction = named<SourceTransactionOp>(root, "run");
    auto &body = root.getBody().front();
    builder.setInsertionPoint(&body.front());
    mlir::OperationState pure(root.getLoc(), BitsUnaryOp::getOperationName());
    pure.addOperands(body.getArgument(0));
    pure.addTypes(body.getArgument(0).getType());
    pure.addAttribute("opcode", builder.getStringAttr("not"));
    auto computedClock = builder.create(pure)->getResult(0);
    if (stateDomain) {
      auto state = *body.getOps<SourceStateOp>().begin();
      state->setOperand(0, computedClock);
    } else
      transaction->setOperand(0, computedClock);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    ASSERT_TRUE(analysis.isTotal(transaction));
    EXPECT_TRUE(mlir::failed(
        capture([&] { return analysis.getPublicationSummary(root); })));
    EXPECT_NE(llvm::StringRef(diagnostics).lower().find("domain"),
              std::string::npos)
        << diagnostics;
    EXPECT_TRUE(analysis.isValid());
    EXPECT_TRUE(analysis.isTotal(transaction));
    EXPECT_TRUE(mlir::failed(analysis.getPublicationSummary(root)));
    // A separate definition remains publishable after a memoized failure.
    EXPECT_TRUE(summary(analysis, module(*file, "Child")));
  }
}

TEST_F(MethodSummaryAuthorityTest,
       ForeignDefinitionCannotReuseAnAnalysisBySymbolName) {
  auto file = parse(), foreignFile = parse();
  ASSERT_TRUE(file);
  ASSERT_TRUE(foreignFile);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto root = module(*file, "Root");
  ASSERT_TRUE(summary(analysis, root));
  auto foreign = module(*foreignFile, "Root");
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis.getPublicationSummary(foreign); })));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_TRUE(analysis.isValid());
  EXPECT_TRUE(analysis.isTotal(named<SourceTransactionOp>(root, "run")));
  EXPECT_TRUE(summary(analysis, root));
}

TEST_F(MethodSummaryAuthorityTest, ScopeAndIdentityMutationsFailClosed) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto child = module(*file, "Child"), root = module(*file, "Root");
  auto childSummary = summary(analysis, child),
       rootSummary = summary(analysis, root);
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  auto dependencyPath =
      Path{std::string("methods"),      0u,
           std::string("results"),      0u,
           std::string("dependencies"), std::string("arguments")};
  const std::vector<mlir::Attribute> badArgumentSets = {
      builder.getArrayAttr({builder.getI64IntegerAttr(-1)}),
      builder.getArrayAttr({builder.getI64IntegerAttr(3)}),
      builder.getArrayAttr(
          {builder.getI64IntegerAttr(1), builder.getI64IntegerAttr(1)}),
      builder.getArrayAttr(
          {builder.getI64IntegerAttr(2), builder.getI64IntegerAttr(0)}),
      builder.getArrayAttr({builder.getI32IntegerAttr(1)}),
      builder.getArrayAttr({mlir::IntegerAttr::get(
          mlir::IntegerType::get(&context, 64, mlir::IntegerType::Signed),
          1)})};
  for (auto invalid : badArgumentSets)
    structureRejects(child,
                     dict(replace(childSummary, dependencyPath, invalid)));
  const std::vector<std::pair<Path, mlir::Attribute>> invalidChild = {
      {{std::string("methods"), 0u, std::string("results"), 0u,
        std::string("dependencies"), std::string("captures")},
       builder.getArrayAttr({builder.getI64IntegerAttr(5)})},
      {{std::string("methods"), 0u, std::string("captures"), 0u,
        std::string("value"), std::string("dependencies"),
        std::string("captures")},
       builder.getArrayAttr({builder.getI64IntegerAttr(0)})},
      {{std::string("methods"), 0u, std::string("body_effects"), 0u,
        std::string("path"), std::string("ordinal")},
       builder.getI64IntegerAttr(1)},
      {{std::string("owners"), 0u, std::string("domain"), std::string("clock"),
        std::string("ordinal")},
       builder.getI64IntegerAttr(3)},
      {{std::string("owners"), 0u, std::string("domain"), std::string("reset"),
        std::string("ordinal")},
       builder.getI64IntegerAttr(-1)},
      {{std::string("owners"), 0u, std::string("state"),
        std::string("declaration")},
       occurrence("Root", 0)},
      {{std::string("owners"), 1u, std::string("name")},
       builder.getStringAttr("count")},
      {{std::string("methods"), 0u, std::string("body_effects"), 3u,
        std::string("selectors"), 0u, std::string("name")},
       builder.getStringAttr("absent")}};
  for (const auto &[path, invalid] : invalidChild)
    structureRejects(child, dict(replace(childSummary, path, invalid)));
  auto receiver =
      Path{std::string("autonomous"),   0u,
           std::string("body_effects"), 0u,
           std::string("receiver"),     std::string("instance_path")};
  for (auto invalidOccurrence :
       {occurrence("Root", 20), occurrence("Child", 0), occurrence("Root", 0)})
    structureRejects(root,
                     dict(replace(rootSummary, receiver,
                                  builder.getArrayAttr({invalidOccurrence}))));
  auto autonomousArgument =
      Path{std::string("autonomous"), 0u, std::string("guard"),
           std::string("dependencies"), std::string("arguments")};
  structureRejects(
      root,
      dict(replace(rootSummary, autonomousArgument,
                   builder.getArrayAttr({builder.getI64IntegerAttr(0)}))));
  // Nonpredicate physical values cannot claim constant or argument truth.
  structureRejects(
      child, dict(replace(childSummary,
                          {std::string("methods"), 0u, std::string("results"),
                           0u, std::string("fact")},
                          constantFact(true))));
  structureRejects(
      root, dict(replace(rootSummary,
                         {std::string("owners"), 1u, std::string("arguments"),
                          3u, std::string("fact")},
                         constantFact(true))));
}

TEST_F(MethodSummaryAuthorityTest,
       NullRequestsAndInvalidAnalysesDiagnoseSafely) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, module(*file, "Child"));
  ASSERT_TRUE(published);
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis.getPublicationSummary(ModuleOp{}); })));
  EXPECT_TRUE(mlir::failed(capture(
      [&] { return verifySourceTransactionSummary(nullptr, published); })));
  EXPECT_TRUE(mlir::failed(capture([&] {
    return verifySourceTransactionSummary(module(*file, "Child"), {});
  })));
  // Break a required source Method contract to produce an invalid analysis.
  auto child = module(*file, "Child");
  named<SourceMethodOp>(child, "act")->removeAttr("ac.parameters");
  auto invalid = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  ASSERT_FALSE(invalid->isValid());
  EXPECT_TRUE(mlir::failed(
      capture([&] { return invalid->getPublicationSummary(child); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(MethodSummaryAuthorityTest, PureRegionValuesHaveUnknownMayDependencies) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto watch = named<SourceTransactionOp>(root, "watch");
  auto input = root.getBody().front().getArgument(3);
  builder.setInsertionPoint(watch);
  mlir::OperationState pure(root.getLoc(), RuleOp::getOperationName());
  pure.addOperands(input);
  pure.addTypes(input.getType());
  pure.addAttribute("name", builder.getStringAttr("compute"));
  pure.addAttribute("occurrence", occurrence("Root", 20));
  pure.addRegion();
  auto *operation = builder.create(pure);
  auto *block = new mlir::Block;
  operation->getRegion(0).push_back(block);
  auto argument = block->addArgument(input.getType(), root.getLoc());
  builder.setInsertionPointToEnd(block);
  mlir::OperationState yield(root.getLoc(), YieldOp::getOperationName());
  yield.addOperands(argument);
  builder.create(yield);
  watch->setOperand(3, operation->getResult(0));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, root);
  ASSERT_TRUE(published);
  unknown(dict(row(namedRow(published, "autonomous", "watch"), "captures", 1)
                   .get("value")));
}

TEST_F(MethodSummaryAuthorityTest,
       RecursiveInstanceGraphRejectsBeforeExpansion) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  auto exemplar = *root.getBody().front().getOps<InstanceOp>().begin();
  auto *recursive = exemplar->clone();
  recursive->setOperands(child.getBody().front().getArguments());
  recursive->setAttr("instance_name", builder.getStringAttr("cycle"));
  recursive->setAttr("occurrence", occurrence("Child", 20));
  child.getBody().front().getOperations().insert(
      child.getBody().front().getTerminator()->getIterator(), recursive);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis.getPublicationSummary(root); })));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recursive"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(llvm::StringRef(diagnostics).lower().find("budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(analysis.isTotal(named<SourceTransactionOp>(root, "run")));
}

TEST_F(MethodSummaryAuthorityTest,
       RecursiveMethodsRemainInvalidOnKnownFalsePaths) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto relay = named<SourceMethodOp>(child, "relay");
  auto last =
      *std::next(act.getBody().front().getOps<SourceMethodCallOp>().begin());
  auto *recursive = last->clone();
  recursive->setOperand(1, relay.getBody().front().getArgument(0));
  auto *literal = last.getPath().getDefiningOp()->clone();
  relay.getBody().front().getOperations().insert(
      relay.getBody().front().getTerminator()->getIterator(), literal);
  recursive->setOperand(0, literal->getResult(0));
  recursive->setAttr("occurrence", occurrence("Child", 21));
  relay.getBody().front().getOperations().insert(
      relay.getBody().front().getTerminator()->getIterator(), recursive);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  EXPECT_FALSE(analysis->isValid());
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recursive"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis->getPublicationSummary(child); })));
}

TEST_F(MethodSummaryAuthorityTest,
       InstanceDepthIsBoundedWithoutFlatteningChildSummaries) {
  for (unsigned edges : {128u, 129u}) {
    SCOPED_TRACE(edges);
    auto file = parse();
    ASSERT_TRUE(file);
    auto root = module(*file, "Root");
    auto exemplar = *root.getBody().front().getOps<InstanceOp>().begin();
    llvm::SmallVector<ModuleOp> levels;
    for (unsigned i = 0; i <= edges; ++i) {
      auto name = "Level" + std::to_string(i);
      mlir::OperationState state(root.getLoc(), ModuleOp::getOperationName());
      state.addAttributes(root->getAttrs());
      state.attributes.set("sym_name", builder.getStringAttr(name));
      state.addRegion();
      builder.setInsertionPoint(root);
      auto level = mlir::cast<ModuleOp>(builder.create(state));
      auto *block = new mlir::Block;
      level.getBody().push_back(block);
      for (auto type : root.getFunctionType().getInputs())
        block->addArgument(type, root.getLoc());
      builder.setInsertionPointToEnd(block);
      mlir::OperationState yield(root.getLoc(), YieldOp::getOperationName());
      yield.addOperands(block->getArgument(3));
      builder.create(yield);
      levels.push_back(level);
    }
    for (unsigned i = 0; i < edges; ++i) {
      auto *instance = exemplar->clone();
      auto &block = levels[i].getBody().front();
      instance->setOperands(block.getArguments());
      instance->setAttr("instance_name", builder.getStringAttr("child"));
      instance->setAttr("callee",
                        mlir::FlatSymbolRefAttr::get(
                            &context, "Level" + std::to_string(i + 1)));
      instance->setAttr("occurrence",
                        occurrence("Level" + std::to_string(i), 0));
      block.getOperations().insert(block.getTerminator()->getIterator(),
                                   instance);
    }
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    auto result =
        capture([&] { return analysis.getPublicationSummary(levels[0]); });
    if (edges == 128) {
      ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
      ASSERT_EQ(array(*result, "owners").size(), 1u);
      EXPECT_TRUE(array(*result, "methods").empty());
      EXPECT_TRUE(array(*result, "autonomous").empty());
    } else {
      EXPECT_TRUE(mlir::failed(result));
      EXPECT_NE(llvm::StringRef(diagnostics).lower().find("depth"),
                std::string::npos)
          << diagnostics;
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       StructuralDepthAndWorkBudgetDiagnoseBeforeSchemaTraversal) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto root = module(*file, "Root");
  auto published = summary(analysis, root);
  ASSERT_TRUE(published);
  mlir::Attribute nested = builder.getUnitAttr();
  for (unsigned i = 0; i != 140; ++i)
    nested = builder.getArrayAttr({nested});
  structureRejects(root, changed(published, "extra", nested));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("depth"),
            std::string::npos)
      << diagnostics;
  auto repeated = namedRow(published, "autonomous", "watch");
  llvm::SmallVector<mlir::Attribute> records(32768, repeated);
  structureRejects(
      root, changed(published, "autonomous", builder.getArrayAttr(records)));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("budget"),
            std::string::npos)
      << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       PublicationWorkBudgetIsIndependentOfTotalityBudget) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto child = module(*file, "Child");
  auto relay = named<SourceMethodOp>(child, "relay");
  auto *method = relay->clone();
  method->setAttr("sym_name", builder.getStringAttr("wide_capture"));
  method->setAttr("occurrence", occurrence("Root", 20));
  method->setAttr("function_type", mlir::TypeAttr::get(mlir::FunctionType::get(
                                       &context, {}, {})));
  method->setAttr("argument_names", builder.getArrayAttr({}));
  method->setAttr("ac.parameters", builder.getArrayAttr({}));
  method->setAttr("ac.result_constraints", builder.getArrayAttr({}));
  method->setAttr("ac.return_form", builder.getStringAttr("none"));
  auto input = root.getBody().front().getArgument(3);
  // Repeated unused captures are legal payload operands whose ordered records
  // must all be emitted. One callable avoids quadratic sibling-name checks.
  // Existing totality work is ~210k units; publication emits >1M record units.
  constexpr unsigned captureCount = 70000;
  llvm::SmallVector<mlir::Value> captures(captureCount, input);
  method->setOperands(captures);
  auto trueValue = *root.getBody().front().getOps<BitsConstantOp>().begin();
  for (unsigned region = 0; region != 2; ++region) {
    auto &body = method->getRegion(region);
    body.getBlocks().clear();
    auto *block = new mlir::Block;
    body.push_back(block);
    for (unsigned i = 0; i != captureCount; ++i)
      block->addArgument(input.getType(), root.getLoc());
    builder.setInsertionPointToEnd(block);
    mlir::OperationState yield(root.getLoc(), YieldOp::getOperationName());
    if (region == 0) {
      auto *literal = trueValue->clone();
      block->getOperations().push_back(literal);
      yield.addOperands(literal->getResult(0));
    }
    builder.create(yield);
  }
  root.getBody().front().getOperations().insert(
      root.getBody().front().getTerminator()->getIterator(), method);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto run = named<SourceTransactionOp>(root, "run");
  ASSERT_TRUE(analysis.isTotal(run));
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis.getPublicationSummary(root); })));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(analysis.isValid());
  EXPECT_TRUE(analysis.isTotal(run));
}

TEST_F(MethodSummaryAuthorityTest,
       UnusedUnknownValuesStillRequireResolvablePositiveHardwareTypes) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto root = module(*file, "Root");
  auto published = summary(analysis, root);
  ASSERT_TRUE(published);
  auto constants = root.getBody().front().getOps<BitsConstantOp>();
  auto zero = *std::next(constants.begin());
  const mlir::Type invalidTypes[] = {
      StructType::get(&context, builder.getStringAttr("MissingStruct")),
      EnumType::get(&context, builder.getStringAttr("MissingEnum")),
      BitsType::get(&context, zero.getValueAttr())};
  const Path unusedTypes[] = {
      {std::string("owners"), 0u, std::string("type")},
      {std::string("autonomous"), 1u, std::string("captures"), 1u,
       std::string("type")},
      {std::string("owners"), 1u, std::string("result_types"), 0u}};
  for (const auto &path : unusedTypes)
    for (auto type : invalidTypes)
      structureRejects(
          root, dict(replace(published, path, mlir::TypeAttr::get(type))));
}

TEST_F(MethodSummaryAuthorityTest,
       VoidMethodsKeepTheirRequiredSourceContracts) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto relay = named<SourceMethodOp>(child, "relay");
  auto *idle = relay->clone();
  idle->setAttr("sym_name", builder.getStringAttr("idle"));
  idle->setAttr("occurrence", occurrence("Child", 14));
  auto signature = relay.getFunctionType();
  idle->setAttr("function_type", mlir::TypeAttr::get(mlir::FunctionType::get(
                                     &context, signature.getInputs(), {})));
  idle->setAttr("ac.result_constraints", builder.getArrayAttr({}));
  idle->setAttr("ac.return_form", builder.getStringAttr("none"));
  auto parameter =
      changed(dict(relay->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]),
              "origin", occurrence("Child", 14));
  idle->setAttr("ac.parameters", builder.getArrayAttr({parameter}));
  idle->getRegion(1).front().getTerminator()->setOperands({});
  child.getBody().front().getOperations().insert(relay->getIterator(), idle);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto row = namedRow(published, "methods", "idle");
  EXPECT_EQ(row.get("ac.return_form"), builder.getStringAttr("none"));
  EXPECT_TRUE(array(row, "ac.result_constraints").empty());
  EXPECT_TRUE(array(row, "results").empty());
  EXPECT_TRUE(array(row, "body_effects").empty());
  structureAccepts(child, published);
  structureRejects(child, dict(replace(published,
                                       {std::string("methods"), 1u,
                                        std::string("ac.return_form")},
                                       builder.getStringAttr("single"))));
}

} // namespace
} // namespace acir::ac
