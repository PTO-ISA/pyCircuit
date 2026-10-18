#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>
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
    }) {sym_name = "act", function_type = (!b1, !b8, !b8) -> !b8, argument_names = ["allow", "value", "index"], ac.parameters = [{name = "allow", binding = "positional_or_keyword", constraint = #bit_constraint, default = {present = false}, origin = #c2, location = #span}, {name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c2, location = #span}, {name = "index", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c2, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #c2} : (!b1, !b1, !b1, !b8, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%value: !b8, %always: !b1):
      "ac.yield"(%always) : (!b1) -> ()
    }, {
    ^bb0(%value: !b8, %always: !b1):
      "ac.yield"(%value) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["value"], ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #c10, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #c10} : (!b1) -> ()
    "ac.transaction"(%clk, %no, %yes) ({
    ^bb0(%always: !b1):
      "ac.yield"(%always) : (!b1) -> ()
    }, {
    ^bb0(%always: !b1):
      %unused = "ac.state.read"(%always) {state = "count", selectors = [], occurrence = #c13} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "inspect", declaration = #c11, occurrence = #c12, ac.guard_location = #span} : (!b1, !b1, !b1) -> ()
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
    }) {name = "run", declaration = #r3, occurrence = #r4, ac.guard_location = #span} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    "ac.transaction"(%clk, %rst, %yes, %left) ({
    ^bb0(%path: !b1, %unused_child_result: !b8):
      %guard_read = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r10} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %unused_child_result: !b8):
      %unused = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r11} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #r8, occurrence = #r9, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> ()
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

  SourceMethodOp formalProposal(ModuleOp owner, llvm::StringRef form,
                                unsigned publicCount, bool nominal = false) {
    auto relay = named<SourceMethodOp>(owner, "relay");
    auto method = mlir::cast<SourceMethodOp>(relay->clone());
    method->setAttr("sym_name", builder.getStringAttr("proposal"));
    method->setAttr("occurrence", occurrence("Child", 20));
    auto byte = relay.getFunctionType().getInput(0);
    auto payload =
        nominal ? mlir::parseType("!ac.struct<\"Packet\">", &context) : byte;
    auto bit = owner.getBody().front().getArgument(0).getType();
    llvm::SmallVector<mlir::Type> types(publicCount + 1, payload);
    types.push_back(bit);
    method->setAttr("function_type",
                    mlir::TypeAttr::get(
                        mlir::FunctionType::get(&context, {payload}, types)));
    auto data =
        relay->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints")[0];
    if (nominal) {
      data = changed(mlir::cast<mlir::DictionaryAttr>(data), "type",
                     mlir::TypeAttr::get(payload));
      data = changed(mlir::cast<mlir::DictionaryAttr>(data), "source_kind",
                     builder.getStringAttr("nominal"));
      auto parameters = method->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
      method->setAttr("ac.parameters",
                      builder.getArrayAttr({changed(
                          mlir::cast<mlir::DictionaryAttr>(parameters[0]),
                          "constraint", data)}));
      for (auto &region : method->getRegions())
        region.front().getArgument(0).setType(payload);
    }
    auto intent = changed(mlir::cast<mlir::DictionaryAttr>(data), "type",
                          mlir::TypeAttr::get(bit));
    intent =
        changed(intent, "source_kind", builder.getStringAttr("fixed_bits"));
    llvm::SmallVector<mlir::Attribute> constraints(publicCount + 1, data);
    constraints.push_back(intent);
    method->setAttr("ac.result_constraints", builder.getArrayAttr(constraints));
    method->setAttr("ac.return_form", builder.getStringAttr(form));
    method->setAttr("ac.formal_proposals",
                    builder.getArrayAttr({builder.getI64IntegerAttr(0)}));
    auto &body = method.getBody().front();
    llvm::SmallVector<mlir::Value> yields(publicCount + 1, body.getArgument(0));
    yields.push_back(body.getArgument(1));
    body.getTerminator()->setOperands(yields);
    owner.getBody().front().getOperations().insert(relay->getIterator(),
                                                   method.getOperation());
    return method;
  }

  SourceFormalWriteOp formalWrite(SourceMethodOp method, unsigned site,
                                  bool scalarRhs = false) {
    auto &body = method.getBody().front();
    builder.setInsertionPoint(body.getTerminator());
    mlir::Value payload = body.getArgument(0);
    llvm::SmallVector<mlir::Attribute> selectors;
    if (scalarRhs) {
      auto declaration =
          *method->getParentOfType<mlir::ModuleOp>().getOps<StructOp>().begin();
      auto lanes = declaration.getFields()[0];
      auto table = mlir::cast<mlir::DictionaryAttr>(lanes)
                       .getAs<mlir::TypeAttr>("type")
                       .getValue();
      mlir::OperationState project(method.getLoc(),
                                   StructGetOp::getOperationName());
      project.addOperands(payload);
      project.addTypes(table);
      project.addAttribute("field", builder.getStringAttr("lanes"));
      auto value = builder.create(project)->getResult(0);
      mlir::OperationState element(method.getLoc(),
                                   TableGetOp::getOperationName());
      element.addOperands({value, body.getArgument(1)});
      element.addTypes({mlir::cast<TableType>(table).getElementType(),
                        body.getArgument(1).getType()});
      payload = builder.create(element)->getResult(0);
      selectors.push_back(builder.getDictionaryAttr(
          {builder.getNamedAttr("field", builder.getStringAttr("lanes"))}));
      selectors.push_back(builder.getDictionaryAttr(
          {builder.getNamedAttr("index", builder.getI64IntegerAttr(0))}));
    }
    mlir::OperationState state(method.getLoc(),
                               SourceFormalWriteOp::getOperationName());
    state.addOperands({payload, body.getArgument(1), body.getArgument(0)});
    if (scalarRhs)
      state.addOperands(body.getArgument(1));
    state.addAttribute("formal", builder.getI64IntegerAttr(0));
    state.addAttribute("selectors", builder.getArrayAttr(selectors));
    state.addAttribute(
        "operandSegmentSizes",
        builder.getDenseI32ArrayAttr({1, 1, 1, scalarRhs ? 1 : 0, 0}));
    state.addAttribute("occurrence", occurrence("Child", site));
    state.addAttribute("ac.location", method->getAttr("ac.guard_location"));
    return mlir::cast<SourceFormalWriteOp>(builder.create(state));
  }

  // Obtain a bodyless transaction declaration from real verified native IR,
  // then remove the provider. The summary never comes from a hand-built oracle.
  mlir::OwningOpRef<mlir::ModuleOp> importFixture(bool zeroOutputs = false) {
    auto file = parse();
    if (!file)
      return {};
    module(*file, "Root")->erase();
    auto child = module(*file, "Child");
    if (zeroOutputs) {
      auto type = child.getFunctionType();
      child->setAttr("function_type",
                     mlir::TypeAttr::get(mlir::FunctionType::get(
                         &context, type.getInputs(), {})));
      child->setAttr("output_names", builder.getArrayAttr({}));
      child.getBody().front().getTerminator()->setOperands({});
    }
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    EXPECT_TRUE(analysis.isValid());
    auto published = summary(analysis, child);
    if (!published)
      return {};
    mlir::OperationState declaration(child.getLoc(),
                                     ModuleImportOp::getOperationName());
    declaration.addAttributes(child->getAttrs());
    declaration.addAttribute("semantic_kind",
                             builder.getStringAttr("transactions"));
    declaration.addAttribute("transaction_summary", published);
    builder.setInsertionPoint(child);
    builder.create(declaration);
    child->erase();
    return file;
  }

  ModuleImportOp imported(mlir::ModuleOp file) {
    return *file.getOps<ModuleImportOp>().begin();
  }

  ModuleOp emptyDefinition(mlir::ModuleOp file, llvm::StringRef name,
                           mlir::FunctionType type,
                           mlir::ArrayAttr inputNames) {
    auto declaration = imported(file);
    mlir::OperationState state(declaration.getLoc(),
                               ModuleOp::getOperationName());
    state.addAttributes(declaration->getAttrs());
    state.attributes.erase("semantic_kind");
    state.attributes.erase("transaction_summary");
    state.attributes.erase("dependency_summary");
    state.attributes.set("sym_name", builder.getStringAttr(name));
    state.attributes.set("function_type", mlir::TypeAttr::get(type));
    state.attributes.set("input_names", inputNames);
    state.attributes.set("output_names", builder.getArrayAttr({}));
    state.addRegion();
    builder.setInsertionPointToEnd(file.getBody());
    auto definition = mlir::cast<ModuleOp>(builder.create(state));
    auto *block = new mlir::Block;
    definition.getBody().push_back(block);
    for (auto input : type.getInputs())
      block->addArgument(input, definition.getLoc());
    builder.setInsertionPointToEnd(block);
    mlir::OperationState yield(definition.getLoc(),
                               YieldOp::getOperationName());
    builder.create(yield);
    return definition;
  }

  void instance(ModuleOp owner, llvm::StringRef callee) {
    builder.setInsertionPoint(owner.getBody().front().getTerminator());
    mlir::OperationState state(owner.getLoc(), InstanceOp::getOperationName());
    state.addOperands(owner.getBody().front().getArguments());
    state.addAttribute("callee",
                       mlir::FlatSymbolRefAttr::get(&context, callee));
    state.addAttribute("instance_name", builder.getStringAttr("child"));
    state.addAttribute("parameters", builder.getArrayAttr({}));
    state.addAttribute("type_arguments", builder.getArrayAttr({}));
    state.addAttribute("occurrence", occurrence(owner.getSymName(), 0));
    builder.create(state);
  }

  ModuleImportOp replaceDefinition(ModuleOp definition,
                                   mlir::DictionaryAttr published) {
    mlir::OperationState state(definition.getLoc(),
                               ModuleImportOp::getOperationName());
    state.addAttributes(definition->getAttrs());
    state.addAttribute("semantic_kind", builder.getStringAttr("transactions"));
    state.addAttribute("transaction_summary", published);
    builder.setInsertionPoint(definition);
    auto declaration = mlir::cast<ModuleImportOp>(builder.create(state));
    definition->erase();
    return declaration;
  }

  SourceBranchOp branch(SourceMethodOp callable, mlir::Block::iterator where,
                        mlir::Value condition, mlir::Value whenTrue,
                        mlir::Value whenFalse, unsigned ordinal) {
    mlir::OperationState state(callable.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(condition);
    state.addTypes(whenTrue.getType());
    state.addAttribute("occurrence", occurrence("Child", ordinal));
    state.addAttribute("ac.condition_location",
                       callable->getAttr("ac.guard_location"));
    state.addRegion();
    state.addRegion();
    builder.setInsertionPoint(&callable.getBody().front(), where);
    auto split = mlir::cast<SourceBranchOp>(builder.create(state));
    for (auto [region, value] :
         llvm::zip(split->getRegions(),
                   std::array<mlir::Value, 2>{whenTrue, whenFalse})) {
      auto *arm = new mlir::Block;
      region.push_back(arm);
      builder.setInsertionPointToEnd(arm);
      mlir::OperationState yield(split.getLoc(), YieldOp::getOperationName());
      yield.addOperands(value);
      builder.create(yield);
    }
    return split;
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
  auto guardLocationPath =
      Path{std::string("methods"), 0u, std::string("guard_location")};
  auto guardLocation = dict(at(childSummary, guardLocationPath));
  auto changedGuardLocation =
      changed(guardLocation, "line", builder.getI64IntegerAttr(2));
  changedGuardLocation =
      changed(changedGuardLocation, "end_line", builder.getI64IntegerAttr(2));
  bodyRejects(
      child,
      dict(replace(childSummary, guardLocationPath, changedGuardLocation)),
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
       PublishedGuardLocationsRejectMissingMalformedAndForeignSpans) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto child = module(*file, "Child");
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  for (llvm::StringRef section : {"methods", "autonomous"}) {
    auto rowPath = Path{section.str(), 0u};
    auto callable = dict(at(published, rowPath));
    auto location = callable.getAs<mlir::DictionaryAttr>("guard_location");
    ASSERT_TRUE(location);
    const mlir::Attribute replacements[] = {
        mlir::Attribute{},
        builder.getUnitAttr(),
        changed(location, "end_column", {}),
        changed(location, "path", builder.getStringAttr("foreign.py")),
    };
    for (auto replacement : replacements) {
      SCOPED_TRACE(section.str());
      structureRejects(child, dict(replace(published, rowPath,
                                           changed(callable, "guard_location",
                                                   replacement))));
    }
  }
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
  declaration.addAttribute("semantic_kind",
                           builder.getStringAttr("transactions"));
  declaration.addAttribute("transaction_summary", published);
  // A bodyless declaration is a real native IR operation, not an importer or
  // a substitute for body authority. It need not provide closure proofs here.
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
  EXPECT_EQ(analysis.lookup(SourceMethodOp{}), nullptr);
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
  auto analysis = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  EXPECT_FALSE(analysis->isValid());
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recursive"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(llvm::StringRef(diagnostics).lower().find("budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(mlir::failed(
      capture([&] { return analysis->getPublicationSummary(root); })));
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
    auto analysis = capture([&] {
      return std::make_unique<SourceMethodSummariesAnalysis>(
          file->getOperation());
    });
    if (edges == 128) {
      ASSERT_TRUE(analysis->isValid()) << diagnostics;
      auto result =
          capture([&] { return analysis->getPublicationSummary(levels[0]); });
      ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
      ASSERT_EQ(array(*result, "owners").size(), 1u);
      EXPECT_TRUE(array(*result, "methods").empty());
      EXPECT_TRUE(array(*result, "autonomous").empty());
    } else {
      EXPECT_FALSE(analysis->isValid());
      EXPECT_NE(llvm::StringRef(diagnostics).lower().find("depth"),
                std::string::npos)
          << diagnostics;
      EXPECT_TRUE(mlir::failed(
          capture([&] { return analysis->getPublicationSummary(levels[0]); })));
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
       ChecksAndObservationsPublishClosedOrderedBodyEffects) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto &body = act.getBody().front();
  auto path = body.getArgument(0);
  auto value = body.getArgument(1);
  auto sourceCondition = body.getArgument(5);
  auto location = act->getAttr("ac.guard_location");
  builder.setInsertionPoint(body.getTerminator());

  mlir::OperationState check(act.getLoc(), SourceExpectOp::getOperationName());
  check.addOperands({sourceCondition, path});
  check.addAttribute("kind", builder.getStringAttr("assert"));
  check.addAttribute("location", location);
  check.addAttribute("occurrence", occurrence("Child", 20));
  builder.create(check);

  auto valueItem = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("value")),
       builder.getNamedAttr("ordinal", builder.getI32IntegerAttr(0))});
  mlir::OperationState observe(act.getLoc(),
                               SourceObserveOp::getOperationName());
  observe.addOperands({path, value});
  observe.addAttribute("kind", builder.getStringAttr("log"));
  observe.addAttribute(
      "spec",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("level", builder.getStringAttr("info")),
           builder.getNamedAttr("event", builder.getStringAttr("act")),
           builder.getNamedAttr("items", builder.getArrayAttr({valueItem}))}));
  observe.addAttribute("occurrence", occurrence("Child", 21));
  observe.addAttribute("ac.location", location);
  builder.create(observe);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  structureAccepts(child, published);
  auto method = namedRow(published, "methods", "act");
  auto effects = array(method, "body_effects");
  ASSERT_GE(effects.size(), 2u);
  auto checkRow = dict(effects[effects.size() - 2]);
  auto observeRow = dict(effects[effects.size() - 1]);
  EXPECT_EQ(kind(checkRow), "check");
  EXPECT_EQ(checkRow.getAs<mlir::StringAttr>("check_kind").getValue(),
            "assert");
  EXPECT_TRUE(mlir::isa<mlir::UnitAttr>(checkRow.get("message")));
  EXPECT_TRUE(checkRow.get("condition"));
  EXPECT_TRUE(checkRow.get("path"));
  EXPECT_FALSE(checkRow.get("ac.check_id"));
  EXPECT_EQ(kind(observeRow), "observe");
  EXPECT_EQ(observeRow.getAs<mlir::StringAttr>("observation_kind").getValue(),
            "log");
  EXPECT_EQ(array(observeRow, "values").size(), 1u);
  EXPECT_FALSE(observeRow.get("ac.observation_id"));

  auto effectsPath =
      Path{std::string("methods"), 0u, std::string("body_effects")};
  unsigned checkIndex = effects.size() - 2;
  unsigned observeIndex = effects.size() - 1;
  auto guardEffects = array(method, "guard_effects");
  auto guardEffectsPath =
      Path{std::string("methods"), 0u, std::string("guard_effects")};
  for (auto forbidden :
       {mlir::Attribute(checkRow), mlir::Attribute(observeRow)}) {
    llvm::SmallVector<mlir::Attribute> moved(guardEffects.begin(),
                                             guardEffects.end());
    moved.push_back(forbidden);
    structureRejects(child, dict(replace(published, guardEffectsPath,
                                         builder.getArrayAttr(moved))));
  }
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("message")},
                   builder.getStringAttr(""))),
      analysis);
  auto condition = dict(checkRow.get("condition"));
  auto changedCondition = changed(condition, "fact", constantFact(true));
  if (changedCondition == condition)
    changedCondition = changed(condition, "fact", constantFact(false));
  ASSERT_NE(changedCondition, condition);
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("condition")},
                   changedCondition)),
      analysis);
  auto pathFact = dict(checkRow.get("path"));
  auto changedPath = constantFact(true);
  if (changedPath == pathFact)
    changedPath = constantFact(false);
  ASSERT_NE(changedPath, pathFact);
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("path")},
                   changedPath)),
      analysis);
  auto effectLocation = dict(checkRow.get("location"));
  auto changedLocation =
      changed(changed(effectLocation, "line", builder.getI64IntegerAttr(2)),
              "end_line", builder.getI64IntegerAttr(2));
  ASSERT_NE(changedLocation, effectLocation);
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("location")},
                   changedLocation)),
      analysis);
  auto changedDependencies = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("unknown"))});
  if (changedDependencies ==
      condition.getAs<mlir::DictionaryAttr>("dependencies"))
    changedDependencies = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("known")),
         builder.getNamedAttr("arguments", builder.getArrayAttr({})),
         builder.getNamedAttr("captures", builder.getArrayAttr({}))});
  auto conditionWithChangedDependencies =
      changed(condition, "dependencies", changedDependencies);
  ASSERT_NE(conditionWithChangedDependencies, condition);
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("condition")},
                   conditionWithChangedDependencies)),
      analysis);
  auto changedSpec = changed(dict(observeRow.get("spec")), "event",
                             builder.getStringAttr("different"));
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    observeIndex, std::string("spec")},
                   changedSpec)),
      analysis);
  auto changedValue = changed(row(observeRow, "values"), "type",
                              mlir::TypeAttr::get(path.getType()));
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    observeIndex, std::string("values"), 0u},
                   changedValue)),
      analysis);
  llvm::SmallVector<mlir::Attribute> reordered(effects.begin(), effects.end());
  std::swap(reordered[checkIndex], reordered[observeIndex]);
  bodyRejects(
      child,
      dict(replace(published, effectsPath, builder.getArrayAttr(reordered))),
      analysis);

  for (auto invalid :
       {changed(checkRow, "message", {}),
        changed(checkRow, "extra", builder.getUnitAttr()),
        changed(checkRow, "check_kind", builder.getStringAttr("range")),
        changed(observeRow, "values", builder.getArrayAttr({})),
        changed(observeRow, "observation_kind",
                builder.getStringAttr("trace"))}) {
    llvm::SmallVector<mlir::Attribute> malformed(effects.begin(),
                                                 effects.end());
    unsigned index = kind(invalid) == "check" ? checkIndex : observeIndex;
    malformed[index] = invalid;
    structureRejects(child, dict(replace(published, effectsPath,
                                         builder.getArrayAttr(malformed))));
  }

  std::string hugeSpelling(16 * 1024 * 1024, 'x');
  auto hugeName = builder.getStringAttr(hugeSpelling);
  auto budgetRejects = [&](mlir::DictionaryAttr candidate) {
    structureRejects(child, candidate);
    auto message = llvm::StringRef(diagnostics).lower();
    EXPECT_NE(message.find("budget"), std::string::npos) << diagnostics;
    for (llvm::StringRef laterError :
         {"source owner", "nominal", "unexpected field", "unknown field"})
      EXPECT_EQ(message.find(laterError.str()), std::string::npos)
          << diagnostics;
    EXPECT_EQ(published, summary(analysis, child));
  };

  budgetRejects(
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    checkIndex, std::string("occurrence"), std::string("site"),
                    std::string("definition")},
                   mlir::FlatSymbolRefAttr::get(&context, hugeSpelling))));

  auto observedTypePath = Path{
      std::string("methods"), 0u, std::string("body_effects"), observeIndex,
      std::string("values"),  0u, std::string("type")};
  for (mlir::Type type :
       {mlir::Type(EnumType::get(&context, hugeName)),
        mlir::Type(TypeParamType::get(
            &context, mlir::FlatSymbolRefAttr::get(&context, "Owner"),
            hugeName)),
        mlir::Type(TypeParamType::get(
            &context, mlir::FlatSymbolRefAttr::get(&context, hugeSpelling),
            builder.getStringAttr("T")))})
    budgetRejects(
        dict(replace(published, observedTypePath, mlir::TypeAttr::get(type))));
}

TEST_F(MethodSummaryAuthorityTest,
       BranchRowsHaveSevenFieldsOrderedArmsAndAuthenticatedResults) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto &body = act.getBody().front();
  auto split =
      branch(act, body.getTerminator()->getIterator(), body.getArgument(5),
             body.getArgument(1), body.getArgument(6), 19);
  auto originalWrite = *body.getOps<SourceStateWriteOp>().begin();
  for (auto [arm, value, ordinal] :
       {std::tuple<mlir::Region *, mlir::Value, unsigned>(
            &split.getThenRegion(), body.getArgument(1), 20),
        std::tuple<mlir::Region *, mlir::Value, unsigned>(
            &split.getElseRegion(), body.getArgument(6), 21)}) {
    auto *copy = originalWrite->clone();
    copy->setOperand(0, body.getArgument(0));
    copy->setOperand(1, value);
    copy->setAttr("occurrence", occurrence("Child", ordinal));
    arm->front().getOperations().insert(
        arm->front().getTerminator()->getIterator(), copy);
  }
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto methodRow = namedRow(published, "methods", "act");
  auto effects = array(methodRow, "body_effects");
  ASSERT_FALSE(effects.empty());
  unsigned branchIndex = effects.size() - 1;
  auto row = dict(effects[branchIndex]);
  EXPECT_EQ(kind(row), "branch");
  EXPECT_EQ(row.size(), 7u);
  EXPECT_FALSE(row.get("path"));
  EXPECT_EQ(row.get("occurrence"), split.getOccurrence());
  EXPECT_EQ(row.get("condition_location"), split.getConditionLocation());
  ASSERT_EQ(array(row, "then_effects").size(), 1u);
  ASSERT_EQ(array(row, "else_effects").size(), 1u);
  EXPECT_EQ(kind(dict(array(row, "then_effects")[0])), "write");
  EXPECT_EQ(kind(dict(array(row, "else_effects")[0])), "write");
  EXPECT_EQ(dict(array(row, "then_effects")[0]).get("occurrence"),
            occurrence("Child", 20));
  EXPECT_EQ(dict(array(row, "else_effects")[0]).get("occurrence"),
            occurrence("Child", 21));
  auto results = array(row, "results");
  ASSERT_EQ(results.size(), 1u);
  auto result = dict(results[0]);
  EXPECT_EQ(result.size(), 3u);
  EXPECT_TRUE(result.get("type"));
  EXPECT_TRUE(result.get("when_true"));
  EXPECT_TRUE(result.get("when_false"));
  structureAccepts(child, published);

  auto branchPath = Path{std::string("methods"), 0u,
                         std::string("body_effects"), branchIndex};
  for (llvm::StringRef field :
       {"kind", "occurrence", "condition_location", "condition", "then_effects",
        "else_effects", "results"})
    structureRejects(
        child, dict(replace(published, branchPath, changed(row, field, {}))));
  structureRejects(child,
                   dict(replace(published, branchPath,
                                changed(row, "path", constantFact(true)))));

  auto swapped = changed(changed(row, "then_effects", row.get("else_effects")),
                         "else_effects", row.get("then_effects"));
  bodyRejects(child, dict(replace(published, branchPath, swapped)), analysis);
  auto changedResult = changed(result, "when_true", result.get("when_false"));
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    branchIndex, std::string("results"), 0u},
                   changedResult)),
      analysis);
}

TEST_F(MethodSummaryAuthorityTest,
       NominalBranchSummaryRetainsOneTypedSlotAndProviderAuthority) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto &body = act.getBody().front();
  auto byte = body.getArgument(1).getType();
  auto packet = StructType::get(&context, builder.getStringAttr("LocalPacket"));
  auto foreign =
      StructType::get(&context, builder.getStringAttr("ForeignPacket"));
  builder.setInsertionPointToStart(file->getBody());
  for (auto name : {"LocalPacket", "ForeignPacket"}) {
    mlir::OperationState declaration(act.getLoc(),
                                     StructOp::getOperationName());
    declaration.addAttribute("sym_name", builder.getStringAttr(name));
    declaration.addAttribute(
        "fields",
        builder.getArrayAttr({builder.getDictionaryAttr(
            {builder.getNamedAttr("name", builder.getStringAttr("payload")),
             builder.getNamedAttr("type", mlir::TypeAttr::get(byte))})}));
    builder.create(declaration);
  }
  builder.setInsertionPoint(body.getTerminator());
  mlir::OperationState make(act.getLoc(), StructCreateOp::getOperationName());
  make.addOperands(body.getArgument(1));
  make.addTypes(packet);
  auto left = builder.create(make)->getResult(0);
  mlir::OperationState other(act.getLoc(), StructCreateOp::getOperationName());
  other.addOperands(body.getArgument(6));
  other.addTypes(packet);
  auto right = builder.create(other)->getResult(0);
  auto split = branch(act, body.getTerminator()->getIterator(),
                      body.getArgument(5), left, right, 19);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto effects = array(namedRow(published, "methods", "act"), "body_effects");
  auto ordinal = effects.size() - 1;
  auto row = dict(effects[ordinal]);
  EXPECT_EQ(row.size(), 7u);
  auto results = array(row, "results");
  ASSERT_EQ(results.size(), 1u);
  auto result = dict(results[0]);
  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result.getAs<mlir::TypeAttr>("type").getValue(),
            split.getResult(0).getType());
  structureAccepts(child, published);
  // A coherent same-layout nominal substitution still fails against the body.
  auto changedResult = changed(result, "type", mlir::TypeAttr::get(foreign));
  bodyRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    static_cast<unsigned>(ordinal), std::string("results"), 0u},
                   changedResult)),
      analysis);
  auto tablePacket = StructType::get(
      &context, builder.getStringAttr(
                    "Packet")); // Existing declaration contains a Table.
  structureRejects(
      child,
      dict(replace(published,
                   {std::string("methods"), 0u, std::string("body_effects"),
                    static_cast<unsigned>(ordinal), std::string("results"), 0u},
                   changed(result, "type", mlir::TypeAttr::get(tablePacket)))));
}

TEST_F(MethodSummaryAuthorityTest,
       DeadArmDynamicCallsRemainInNativeAndPublishedTotalityInventories) {
  for (bool nested : {false, true}) {
    for (unsigned selector : {0u, 1u, 2u}) {
      SCOPED_TRACE(nested);
      SCOPED_TRACE(selector);
      auto file = parse();
      ASSERT_TRUE(file);
      auto child = module(*file, "Child");
      auto act = named<SourceMethodOp>(child, "act");
      auto relay = named<SourceMethodOp>(child, "relay");
      auto *dynamic = relay->clone();
      dynamic->setAttr("sym_name", builder.getStringAttr("dynamic"));
      dynamic->setAttr("occurrence", occurrence("Child", 19));
      dynamic->setOperand(0, child.getBody().front().getArgument(2));
      child.getBody().front().getOperations().insert(relay->getIterator(),
                                                     dynamic);
      auto &body = act.getBody().front();
      auto exemplar = *body.getOps<SourceMethodCallOp>().begin();
      auto target = llvm::StringRef("dynamic");
      if (nested) {
        auto *outer = relay->clone();
        outer->setAttr("sym_name", builder.getStringAttr("nested"));
        outer->setAttr("occurrence", occurrence("Child", 20));
        auto &nestedBody = outer->getRegion(1).front();
        auto *call = exemplar->clone();
        call->setOperands(
            {nestedBody.getArgument(1), nestedBody.getArgument(0)});
        call->setAttr("callee",
                      mlir::SymbolRefAttr::get(
                          &context, "Child",
                          {mlir::FlatSymbolRefAttr::get(&context, "dynamic")}));
        call->setAttr("occurrence", occurrence("Child", 21));
        nestedBody.getOperations().insert(
            nestedBody.getTerminator()->getIterator(), call);
        child.getBody().front().getOperations().insert(relay->getIterator(),
                                                       outer);
        target = "nested";
      }
      auto zero = *body.getOps<BitsConstantOp>().begin();
      mlir::Value condition = selector == 0   ? mlir::Value(zero.getResult())
                              : selector == 1 ? body.getArgument(4)
                                              : body.getArgument(5);
      auto split = branch(act, body.getTerminator()->getIterator(), condition,
                          body.getArgument(1), body.getArgument(1), 22);
      // Zero selects else, One selects then. Dynamic is reached and cannot
      // prove a total transaction, even when its result is unused.
      auto &arm = selector == 1 ? split.getElseRegion().front()
                                : split.getThenRegion().front();
      auto *call = exemplar->clone();
      call->setAttr("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Child",
                        {mlir::FlatSymbolRefAttr::get(&context, target)}));
      call->setAttr("occurrence", occurrence("Child", 23));
      arm.getOperations().insert(arm.getTerminator()->getIterator(), call);
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
      auto analysis = capture([&] {
        return std::make_unique<SourceMethodSummariesAnalysis>(
            file->getOperation());
      });
      if (selector == 2) {
        EXPECT_FALSE(analysis->isValid());
        EXPECT_NE(diagnostics.find("total"), std::string::npos) << diagnostics;
        continue;
      }
      ASSERT_TRUE(analysis->isValid()) << diagnostics;
      auto run = named<SourceTransactionOp>(module(*file, "Root"), "run");
      EXPECT_TRUE(analysis->isTotal(run));
      auto published = summary(*analysis, child);
      ASSERT_TRUE(published);
      auto effects =
          array(namedRow(published, "methods", "act"), "body_effects");
      auto row = dict(effects[effects.size() - 1]);
      EXPECT_EQ(kind(row), "branch");
      EXPECT_EQ(row.size(), 7u);
      auto armRows =
          array(row, selector == 1 ? "else_effects" : "then_effects");
      ASSERT_EQ(armRows.size(), 1u);
      EXPECT_EQ(kind(dict(armRows[0])), "call");
      structureAccepts(child, published);
      replaceDefinition(child, published);
      auto consumer = capture([&] {
        return std::make_unique<SourceMethodSummariesAnalysis>(
            file->getOperation());
      });
      ASSERT_TRUE(consumer->isValid()) << diagnostics;
      EXPECT_TRUE(consumer->isTotal(run));
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       DeclarationOnlyImportCarriesClosedBranchRowsWithoutBodyAuthority) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto act = named<SourceMethodOp>(child, "act");
  auto &body = act.getBody().front();
  branch(act, body.getTerminator()->getIterator(), body.getArgument(5),
         body.getArgument(1), body.getArgument(6), 19);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis provider(file->getOperation());
  ASSERT_TRUE(provider.isValid());
  auto published = summary(provider, child);
  ASSERT_TRUE(published);
  auto declaration = replaceDefinition(child, published);
  structureAccepts(declaration, published);
  EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  SourceMethodSummariesAnalysis consumer(file->getOperation());
  EXPECT_TRUE(consumer.isValid()) << diagnostics;
  // A declaration supplies its closed header. It cannot be passed to the
  // provider-body rederivation API, whose type requires an actual ModuleOp.
  EXPECT_EQ(declaration->getAttr("transaction_summary"), published);
}

TEST_F(MethodSummaryAuthorityTest,
       EffectMetadataStringsConsumePublicationBudgetBeforeSchemaErrors) {
  auto file = parse();
  ASSERT_TRUE(file);
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto child = module(*file, "Child");
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  std::string huge(16 * 1024 * 1024, 'x');
  structureRejects(
      child, changed(published, "unexpected", builder.getStringAttr(huge)));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(llvm::StringRef(diagnostics).lower().find("unexpected field"),
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

TEST_F(MethodSummaryAuthorityTest,
       TransactionImportsVerifyTheirDerivedSchemaWithoutAProvider) {
  for (bool zeroOutputs : {false, true}) {
    auto file = importFixture(zeroOutputs);
    ASSERT_TRUE(file);
    auto declaration = imported(*file);
    EXPECT_TRUE(file->getOps<ModuleOp>().empty());
    EXPECT_EQ(declaration->getAttr("semantic_kind"),
              builder.getStringAttr("transactions"));
    EXPECT_FALSE(declaration->hasAttr("dependency_summary"));
    ASSERT_TRUE(declaration->getAttrOfType<mlir::DictionaryAttr>(
        "transaction_summary"));
    EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    structureAccepts(declaration,
                     declaration->getAttrOfType<mlir::DictionaryAttr>(
                         "transaction_summary"));
    auto malformed = changed(
        declaration->getAttrOfType<mlir::DictionaryAttr>("transaction_summary"),
        "unexpected", builder.getUnitAttr());
    declaration->setAttr("transaction_summary", malformed);
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       ImportTagsAndSummaryKindsHaveNoDefaultsOrMixedEncoding) {
  for (bool transactions : {false, true}) {
    for (unsigned mutation = 0; mutation != 10; ++mutation) {
      SCOPED_TRACE(transactions);
      SCOPED_TRACE(mutation);
      auto file = importFixture(/*zeroOutputs=*/true);
      ASSERT_TRUE(file);
      auto declaration = imported(*file);
      auto native = declaration->getAttr("transaction_summary");
      auto summaryName =
          transactions ? "transaction_summary" : "dependency_summary";
      if (!transactions) {
        declaration->setAttr("semantic_kind",
                             builder.getStringAttr("hardware"));
        declaration->removeAttr("transaction_summary");
        declaration->setAttr("dependency_summary", builder.getArrayAttr({}));
      }
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
      switch (mutation) {
      case 0:
        declaration->removeAttr("semantic_kind");
        break;
      case 1:
        declaration->setAttr("semantic_kind", builder.getStringAttr("unknown"));
        break;
      case 2:
        declaration->setAttr("semantic_kind", builder.getUnitAttr());
        break;
      case 3:
        declaration->removeAttr(summaryName);
        break;
      case 4:
        declaration->setAttr(summaryName, builder.getUnitAttr());
        break;
      case 5:
        // Both summaries are forbidden even when the other is otherwise valid.
        declaration->setAttr(
            transactions ? "dependency_summary" : "transaction_summary",
            transactions ? mlir::Attribute(builder.getArrayAttr({})) : native);
        break;
      case 6:
        declaration->removeAttr(summaryName);
        declaration->setAttr(
            transactions ? "dependency_summary" : "transaction_summary",
            transactions ? mlir::Attribute(builder.getArrayAttr({})) : native);
        break;
      case 7:
        declaration->setAttr(
            summaryName, transactions
                             ? mlir::Attribute(builder.getArrayAttr({}))
                             : mlir::Attribute(builder.getDictionaryAttr({})));
        break;
      case 8:
        declaration->setAttr(
            "semantic_kind",
            builder.getStringAttr(transactions ? "hardware" : "transactions"));
        break;
      case 9:
        declaration->removeAttr("semantic_kind");
        declaration->removeAttr(summaryName);
        break;
      }
      EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
      EXPECT_FALSE(diagnostics.empty());
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       TransactionImportsCannotClaimTrustedPrimitiveAuthority) {
  for (auto kind : {"dff", "dffe", "sync_mem", "sync_mem_dp", "byte_mem"}) {
    SCOPED_TRACE(kind);
    auto file = importFixture();
    ASSERT_TRUE(file);
    auto declaration = imported(*file);
    declaration->setAttr("primitive_kind", builder.getStringAttr(kind));
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    EXPECT_NE(llvm::StringRef(diagnostics).lower().find("transaction"),
              std::string::npos)
        << diagnostics;
  }
}

TEST_F(MethodSummaryAuthorityTest,
       LegitimateEmptyHardwareDependenciesRemainExplicit) {
  for (bool zeroOutputs : {false, true}) {
    auto file = importFixture(zeroOutputs);
    ASSERT_TRUE(file);
    auto declaration = imported(*file);
    declaration->setAttr("semantic_kind", builder.getStringAttr("hardware"));
    declaration->removeAttr("transaction_summary");
    auto rows =
        zeroOutputs
            ? mlir::Attribute(builder.getArrayAttr({}))
            : mlir::parseAttribute(
                  "[{output = {port = 0 : i64, path = []}, inputs = []}]",
                  &context);
    declaration->setAttr("dependency_summary", rows);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    HardwareAnalysis hardware(*file);
    auto dependencies =
        capture([&] { return hardware.getImportDependencies(declaration); });
    ASSERT_TRUE(mlir::succeeded(dependencies)) << diagnostics;
    ASSERT_EQ(dependencies->size(), zeroOutputs ? 0u : 1u);
    if (!zeroOutputs)
      EXPECT_TRUE((*dependencies)[0].inputs.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       TransactionSummariesNeverActAsHardwareDependencies) {
  for (bool zeroOutputs : {false, true}) {
    SCOPED_TRACE(zeroOutputs);
    auto file = importFixture(zeroOutputs);
    ASSERT_TRUE(file);
    auto declaration = imported(*file);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    HardwareAnalysis hardware(*file);
    EXPECT_TRUE(mlir::failed(
        capture([&] { return hardware.getImportDependencies(declaration); })));
    EXPECT_NE(llvm::StringRef(diagnostics).lower().find("transaction"),
              std::string::npos)
        << diagnostics;
  }
}

TEST_F(MethodSummaryAuthorityTest,
       NestedZeroOutputTransactionInstancesCannotEvadeHardwareAnalysis) {
  auto file = importFixture(/*zeroOutputs=*/true);
  ASSERT_TRUE(file);
  auto declaration = imported(*file);
  auto signature = declaration.getFunctionType();
  auto direct =
      emptyDefinition(*file, "Direct", signature, declaration.getInputNames());
  instance(direct, declaration.getSymName());
  auto nested =
      emptyDefinition(*file, "Nested", signature, declaration.getInputNames());
  instance(nested, direct.getSymName());
  auto unrelated =
      emptyDefinition(*file, "Unrelated", builder.getFunctionType({}, {}),
                      builder.getArrayAttr({}));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  HardwareAnalysis hardware(*file);
  for (auto definition : {direct, nested}) {
    auto result = capture([&] { return hardware.analyzeModule(definition); });
    EXPECT_TRUE(mlir::failed(result));
    EXPECT_NE(llvm::StringRef(diagnostics).lower().find("transaction"),
              std::string::npos)
        << diagnostics;
  }
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return hardware.analyzeModule(unrelated);
  }))) << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       FinalHardwareRejectsEvenUnusedTransactionDeclarations) {
  auto file = importFixture(/*zeroOutputs=*/true);
  ASSERT_TRUE(file);
  auto unrelated =
      emptyDefinition(*file, "Unrelated", builder.getFunctionType({}, {}),
                      builder.getArrayAttr({}));
  ASSERT_TRUE(imported(*file)->use_empty());
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  HardwareAnalysis hardware(*file);
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return hardware.analyzeModule(unrelated);
  }))) << diagnostics;
  EXPECT_TRUE(
      mlir::failed(capture([&] { return verifyHardwarePackage(*file); })));
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("transaction"),
            std::string::npos)
      << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       ZeroOutputCollectionsCannotHideTransactionDescendants) {
  for (bool throughModule : {false, true}) {
    SCOPED_TRACE(throughModule);
    auto file = importFixture(/*zeroOutputs=*/true);
    ASSERT_TRUE(file);
    auto declaration = imported(*file);
    auto direct =
        emptyDefinition(*file, "Direct", declaration.getFunctionType(),
                        declaration.getInputNames());
    instance(direct, declaration.getSymName());
    auto packet = *file->getOps<StructOp>().begin();
    auto field = mlir::cast<mlir::DictionaryAttr>(packet.getFields()[0]);
    auto lanes =
        mlir::cast<TableType>(field.getAs<mlir::TypeAttr>("type").getValue());
    auto shape = lanes.getShape();
    llvm::SmallVector<mlir::Type> inputTypes;
    for (auto input : declaration.getFunctionType().getInputs())
      inputTypes.push_back(TableType::get(&context, shape, input));
    auto signature = builder.getFunctionType(inputTypes, {});
    auto grouped = emptyDefinition(*file, "Grouped", signature,
                                   declaration.getInputNames());
    builder.setInsertionPoint(grouped.getBody().front().getTerminator());
    mlir::OperationState collection(grouped.getLoc(),
                                    CollectionOp::getOperationName());
    collection.addOperands(grouped.getBody().front().getArguments());
    collection.addAttribute(
        "callee", mlir::FlatSymbolRefAttr::get(
                      &context, throughModule ? direct.getSymName()
                                              : declaration.getSymName()));
    collection.addAttribute("instance_name", builder.getStringAttr("children"));
    collection.addAttribute("parameters", builder.getArrayAttr({}));
    collection.addAttribute("type_arguments", builder.getArrayAttr({}));
    collection.addAttribute("shape", shape);
    collection.addAttribute("occurrence", occurrence("Grouped", 0));
    builder.create(collection);
    auto nested = emptyDefinition(*file, "Nested", signature,
                                  declaration.getInputNames());
    instance(nested, grouped.getSymName());
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    HardwareAnalysis hardware(*file);
    for (auto definition : {grouped, nested}) {
      EXPECT_TRUE(mlir::failed(
          capture([&] { return hardware.analyzeModule(definition); })));
      EXPECT_NE(llvm::StringRef(diagnostics).lower().find("transaction"),
                std::string::npos)
          << diagnostics;
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       HeaderOnlyMethodResolutionPreservesRealReceiversAndContracts) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  SourceMethodSummariesAnalysis original(file->getOperation());
  ASSERT_TRUE(original.isValid());
  auto published = summary(original, child);
  ASSERT_TRUE(published);
  auto definitionView =
      capture([&] { return lookupSourceMethod(child, "act", root); });
  ASSERT_TRUE(mlir::succeeded(definitionView)) << diagnostics;
  EXPECT_TRUE(definitionView->definition);
  EXPECT_FALSE(definitionView->row);
  EXPECT_EQ(definitionView->getGuardLocation(),
            definitionView->definition->getAttrOfType<mlir::DictionaryAttr>(
                "ac.guard_location"));
  EXPECT_EQ(definitionView->getGuardLocation(),
            namedRow(published, "methods", "act")
                .getAs<mlir::DictionaryAttr>("guard_location"));
  auto definitionGuardLocation = definitionView->getGuardLocation();
  auto declaration = replaceDefinition(child, published);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto view =
      capture([&] { return lookupSourceMethod(declaration, "act", root); });
  ASSERT_TRUE(mlir::succeeded(view)) << diagnostics;
  EXPECT_FALSE(view->definition);
  EXPECT_EQ(view->row, namedRow(published, "methods", "act"));
  EXPECT_EQ(view->getParameters(), array(view->row, "ac.parameters"));
  EXPECT_EQ(view->getResultConstraints(),
            array(view->row, "ac.result_constraints"));
  EXPECT_EQ(view->getReturnForm(), builder.getStringAttr("single"));
  EXPECT_EQ(view->getGuardLocation(),
            view->row.getAs<mlir::DictionaryAttr>("guard_location"));
  EXPECT_EQ(view->getGuardLocation(), definitionGuardLocation);
  auto run = named<SourceTransactionOp>(root, "run");
  llvm::SmallVector<ResolvedSourceMethod> resolved;
  for (auto call : run.getBody().front().getOps<SourceMethodCallOp>()) {
    auto result = capture([&] { return resolveSourceMethodCall(call); });
    ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
    EXPECT_FALSE(result->method.definition);
    EXPECT_EQ(result->method.module, declaration.getOperation());
    resolved.push_back(*result);
  }
  ASSERT_EQ(resolved.size(), 3u);
  EXPECT_EQ(resolved[0].receiver, resolved[1].receiver);
  EXPECT_NE(resolved[0].receiver, resolved[2].receiver);
  EXPECT_EQ(array(resolved[0].receiver, "instance_path")[0],
            occurrence("Root", 1));
  EXPECT_EQ(array(resolved[2].receiver, "instance_path")[0],
            occurrence("Root", 2));
  SourceMethodSummariesAnalysis current(file->getOperation());
  ASSERT_TRUE(current.isValid());
  EXPECT_TRUE(current.isTotal(run));
  auto proof = current.lookup(view->getSymbol());
  ASSERT_NE(proof, nullptr);
  EXPECT_FALSE(proof->mayBlock);
}

TEST_F(MethodSummaryAuthorityTest,
       ReceiverResolutionEagerlyRejectsDuplicateDirectInventoryIdentity) {
  for (bool duplicateName : {false, true}) {
    SCOPED_TRACE(duplicateName ? "name" : "occurrence");
    auto file = parse();
    ASSERT_TRUE(file);
    auto root = module(*file, "Root");
    auto left = named<SourceTransactionOp>(root, "run")
                    .getBody()
                    .front()
                    .getOps<SourceMethodCallOp>()
                    .begin();
    auto exemplar = *root.getBody().front().getOps<InstanceOp>().begin();
    auto first = mlir::cast<InstanceOp>(exemplar->clone());
    auto second = mlir::cast<InstanceOp>(exemplar->clone());
    first->setAttr("instance_name", builder.getStringAttr("unused_first"));
    second->setAttr("instance_name",
                    builder.getStringAttr(duplicateName ? "unused_first"
                                                        : "unused_second"));
    first->setAttr("occurrence", occurrence("Root", 20));
    second->setAttr("occurrence", occurrence("Root", duplicateName ? 21 : 20));
    auto where = root.getBody().front().getTerminator()->getIterator();
    root.getBody().front().getOperations().insert(where, first);
    root.getBody().front().getOperations().insert(where, second);

    auto resolved = capture([&] { return resolveSourceMethodCall(*left); });
    EXPECT_TRUE(mlir::failed(resolved));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       ReceiverProofContextRejectsDuplicateImportedInventoryIdentity) {
  for (bool duplicateName : {false, true}) {
    SCOPED_TRACE(duplicateName ? "name" : "occurrence");
    auto file = parse();
    ASSERT_TRUE(file);
    auto root = module(*file, "Root");
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid()) << diagnostics;
    auto published = summary(analysis, root);
    ASSERT_TRUE(published);
    auto publishedOwners = array(published, "owners");
    llvm::SmallVector<mlir::Attribute> owners(publishedOwners.begin(),
                                              publishedOwners.end());
    llvm::SmallVector<unsigned> instances;
    for (auto [index, raw] : llvm::enumerate(owners))
      if (dict(raw).getAs<mlir::StringAttr>("kind").getValue() == "instance")
        instances.push_back(index);
    ASSERT_GE(instances.size(), 2u);
    auto first = dict(owners[instances[0]]);
    mlir::NamedAttrList second(dict(owners[instances[1]]));
    if (duplicateName)
      second.set("name", first.get("name"));
    else
      second.set("occurrence", first.get("occurrence"));
    owners[instances[1]] = second.getDictionary(&context);
    published = changed(published, "owners", builder.getArrayAttr(owners));
    auto imported = replaceDefinition(root, published);

    auto moduleMiss = [&](mlir::Operation *, mlir::Operation *) {
      return mlir::success();
    };
    auto selectedMethod = [&](SourceMethodOp, mlir::Operation *) {
      return mlir::success();
    };
    auto workStorage = [&](mlir::Operation *, uint64_t, uint64_t) {
      return mlir::success();
    };
    auto instanceBinding = [&](mlir::Operation *, mlir::Operation *,
                               mlir::Operation *,
                               mlir::Operation *) { return mlir::success(); };
    SourceReceiverProofFees fees{moduleMiss, selectedMethod, workStorage,
                                 instanceBinding};
    SourceReceiverProofContext proof;
    EXPECT_TRUE(mlir::failed(
        capture([&] { return proof.prepare(imported, imported, fees); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       ReceiverProofContextWarmsInventoryButKeepsActualPathsDistinct) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto run = named<SourceTransactionOp>(root, "run");
  auto calls = run.getBody().front().getOps<SourceMethodCallOp>();
  auto left = calls.begin();
  auto right = std::next(left, 2);
  ASSERT_NE(right, calls.end());

  unsigned misses = 0, selected = 0, bindings = 0;
  bool rejectBinding = false;
  uint64_t chargedWork = 0, chargedBytes = 0;
  auto moduleMiss = [&](mlir::Operation *, mlir::Operation *) {
    ++misses;
    return mlir::success();
  };
  auto selectedMethod = [&](SourceMethodOp, mlir::Operation *) {
    ++selected;
    return mlir::success();
  };
  auto workStorage = [&](mlir::Operation *, uint64_t work, uint64_t bytes) {
    chargedWork += work;
    chargedBytes += bytes;
    return mlir::success();
  };
  auto instanceBinding = [&](mlir::Operation *, mlir::Operation *,
                             mlir::Operation *, mlir::Operation *) {
    ++bindings;
    return rejectBinding ? mlir::failure() : mlir::success();
  };
  SourceReceiverProofFees fees{moduleMiss, selectedMethod, workStorage,
                               instanceBinding};
  SourceReceiverProofContext proof;
  HardwareBindings parent;
  parent.owner = root;
  mlir::SymbolTableCollection symbols;
  auto lookup = [&](mlir::FlatSymbolRefAttr symbol) -> mlir::Operation * {
    return symbols.lookupSymbolIn(*file, symbol);
  };

  auto first = capture([&] {
    return resolveSourceMethodCall(*left, parent, lookup, &proof, &fees);
  });
  ASSERT_TRUE(mlir::succeeded(first)) << diagnostics;
  unsigned warmMisses = misses;
  unsigned firstBindings = bindings;
  ASSERT_GT(warmMisses, 0u);
  ASSERT_GT(firstBindings, 0u);
  auto second = capture([&] {
    return resolveSourceMethodCall(*right, parent, lookup, &proof, &fees);
  });
  ASSERT_TRUE(mlir::succeeded(second)) << diagnostics;
  EXPECT_EQ(misses, warmMisses);
  EXPECT_GT(bindings, firstBindings);
  EXPECT_NE(first->receiver, second->receiver);
  EXPECT_EQ(array(first->receiver, "instance_path")[0], occurrence("Root", 1));
  EXPECT_EQ(array(second->receiver, "instance_path")[0], occurrence("Root", 2));
  EXPECT_GT(selected, 0u);
  EXPECT_GT(chargedWork + chargedBytes, 0u);

  rejectBinding = true;
  unsigned paidBindings = bindings;
  EXPECT_TRUE(mlir::failed(capture([&] {
    return resolveSourceMethodCall(*left, parent, lookup, &proof, &fees);
  })));
  EXPECT_EQ(misses, warmMisses);
  EXPECT_GT(bindings, paidBindings);
}

TEST_F(MethodSummaryAuthorityTest,
       ReceiverProofContextDoesNotPublishBudgetFailedInventory) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  SourceReceiverProofContext proof;
  unsigned misses = 0;
  bool rejectStorage = true;
  auto moduleMiss = [&](mlir::Operation *, mlir::Operation *) {
    ++misses;
    return mlir::success();
  };
  auto selectedMethod = [&](SourceMethodOp, mlir::Operation *) {
    return mlir::success();
  };
  auto workStorage = [&](mlir::Operation *, uint64_t, uint64_t bytes) {
    return rejectStorage && bytes > 0 ? mlir::failure() : mlir::success();
  };
  auto instanceBinding = [&](mlir::Operation *, mlir::Operation *,
                             mlir::Operation *,
                             mlir::Operation *) { return mlir::success(); };
  SourceReceiverProofFees fees{moduleMiss, selectedMethod, workStorage,
                               instanceBinding};

  EXPECT_TRUE(
      mlir::failed(capture([&] { return proof.prepare(root, root, fees); })));
  unsigned failedMisses = misses;
  ASSERT_GT(failedMisses, 0u);
  rejectStorage = false;
  auto prepared = capture([&] { return proof.prepare(root, root, fees); });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  EXPECT_GT(misses, failedMisses);
  EXPECT_EQ((*prepared)->instanceCount, 2u);
}

TEST_F(MethodSummaryAuthorityTest,
       HeaderClosureChecksReceiversSignaturesAndAllCalls) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  SourceMethodSummariesAnalysis original(file->getOperation());
  ASSERT_TRUE(original.isValid());
  auto childSummary = summary(original, child),
       rootSummary = summary(original, root);
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  auto declaration = replaceDefinition(child, childSummary);
  auto lookup = [&](mlir::FlatSymbolRefAttr symbol) -> mlir::Operation * {
    if (symbol.getValue() == "Child")
      return declaration;
    if (symbol.getValue() == "Root")
      return root;
    return nullptr;
  };
  llvm::SmallVector<mlir::Operation *> modules{declaration, root};
  SourceMethodSummariesAnalysis canonical(modules, lookup);
  ASSERT_TRUE(canonical.isValid());
  auto call = row(namedRow(rootSummary, "autonomous", "run"), "body_effects");
  HardwareBindings parent;
  parent.owner = root;
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return resolveSourceMethodCall(root, call, parent, lookup, root);
  }))) << diagnostics;
  auto actuals = array(call, "arguments");
  llvm::SmallVector<mlir::Attribute> tooFew(actuals.begin(), actuals.end());
  tooFew.pop_back();
  auto widePredicate = changed(dict(actuals[1]), "fact", constantFact(true));
  llvm::SmallVector<mlir::Attribute> typedActuals(actuals.begin(),
                                                  actuals.end());
  typedActuals[1] = widePredicate;
  auto receiver = dict(call.get("receiver"));
  const mlir::DictionaryAttr badCalls[] = {
      changed(call, "arguments", builder.getArrayAttr(tooFew)),
      changed(call, "arguments", builder.getArrayAttr(typedActuals)),
      changed(call, "callee",
              mlir::SymbolRefAttr::get(
                  &context, "Child",
                  {mlir::FlatSymbolRefAttr::get(&context, "missing")})),
      changed(call, "receiver",
              changed(receiver, "instance_path",
                      builder.getArrayAttr({occurrence("Root", 20)}))),
      changed(call, "receiver",
              changed(receiver, "instance_path",
                      builder.getArrayAttr(
                          {occurrence("Root", 1), occurrence("Child", 20)}))),
      changed(call, "receiver",
              changed(receiver, "instance_path", builder.getArrayAttr({}))),
  };
  for (auto invalid : badCalls) {
    EXPECT_TRUE(mlir::failed(capture([&] {
      return resolveSourceMethodCall(root, invalid, parent, lookup, root);
    })));
    EXPECT_FALSE(diagnostics.empty());
  }
  auto missing = [&](mlir::FlatSymbolRefAttr symbol) -> mlir::Operation * {
    return symbol.getValue() == "Root" ? root.getOperation() : nullptr;
  };
  EXPECT_TRUE(mlir::failed(capture([&] {
    return resolveSourceMethodCall(root, call, parent, missing, root);
  })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(MethodSummaryAuthorityTest, HeaderRecursionRemainsIllegalOnFalsePaths) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  SourceMethodSummariesAnalysis original(file->getOperation());
  ASSERT_TRUE(original.isValid());
  auto published = summary(original, child);
  ASSERT_TRUE(published);
  auto act = namedRow(published, "methods", "act");
  auto falseCall = row(act, "body_effects", 5);
  ASSERT_EQ(dict(falseCall.get("path")), constantFact(false));
  auto recursive = dict(replace(
      published, {std::string("methods"), 1u, std::string("body_effects")},
      builder.getArrayAttr({falseCall})));
  auto declaration = replaceDefinition(child, recursive);
  structureAccepts(declaration, recursive);
  auto analysis = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  EXPECT_FALSE(analysis->isValid());
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recursive"),
            std::string::npos)
      << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       MalformedMethodHeadersDiagnoseBeforeTypedCallResolution) {
  for (unsigned mutation = 0; mutation != 4; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto method = named<SourceMethodOp>(child, "act");
    auto root = module(*file, "Root");
    auto call = *named<SourceTransactionOp>(root, "run")
                     .getBody()
                     .front()
                     .getOps<SourceMethodCallOp>()
                     .begin();
    if (mutation == 0)
      method->setAttr("sym_name", builder.getUnitAttr());
    if (mutation == 1)
      method->removeAttr("sym_name");
    if (mutation == 2)
      method.getGuard().getBlocks().clear();
    if (mutation == 3)
      method.getBody().getBlocks().clear();
    EXPECT_TRUE(
        mlir::failed(capture([&] { return resolveSourceMethodCall(call); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       ImportedNativeCallsEnforceBoundSSAArityAndTypes) {
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child"), root = module(*file, "Root");
    SourceMethodSummariesAnalysis original(file->getOperation());
    ASSERT_TRUE(original.isValid());
    auto published = summary(original, child);
    ASSERT_TRUE(published);
    replaceDefinition(child, published);
    auto call = *named<SourceTransactionOp>(root, "run")
                     .getBody()
                     .front()
                     .getOps<SourceMethodCallOp>()
                     .begin();
    if (mutation == 0) {
      llvm::SmallVector<mlir::Value> operands(call->getOperands());
      operands.pop_back();
      call->setOperands(operands);
    }
    if (mutation == 1)
      call->setOperand(2, call->getOperand(1));
    if (mutation == 2)
      call->getResult(0).setType(call->getOperand(0).getType());
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       HeaderInstanceCyclesAreRejectedEvenWithoutCallResults) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  SourceMethodSummariesAnalysis original(file->getOperation());
  ASSERT_TRUE(original.isValid());
  auto childSummary = summary(original, child),
       rootSummary = summary(original, root);
  ASSERT_TRUE(childSummary);
  ASSERT_TRUE(rootSummary);
  auto cycle = dict(replace(rootSummary,
                            {std::string("owners"), 1u, std::string("callee")},
                            mlir::FlatSymbolRefAttr::get(&context, "Root")));
  cycle = dict(replace(
      cycle, {std::string("autonomous"), 0u, std::string("body_effects")},
      builder.getArrayAttr({})));
  auto childDeclaration = replaceDefinition(child, childSummary);
  auto rootDeclaration = replaceDefinition(root, cycle);
  structureAccepts(childDeclaration, childSummary);
  structureAccepts(rootDeclaration, cycle);
  auto analysis = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  EXPECT_FALSE(analysis->isValid());
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("recursive"),
            std::string::npos)
      << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       FormalProposalSuffixPreservesAuthoredReturnFormAndPublication) {
  for (unsigned count = 0; count != 3; ++count) {
    SCOPED_TRACE(count);
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto method = formalProposal(child,
                                 count == 0   ? "none"
                                 : count == 1 ? "single"
                                              : "tuple",
                                 count);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceMethodView view{child, method, {}};
    EXPECT_EQ(view.getNumPublicResults(), count);
    EXPECT_EQ(view.getFormalProposals(),
              builder.getArrayAttr({builder.getI64IntegerAttr(0)}));
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    auto published = summary(analysis, child);
    ASSERT_TRUE(published);
    auto row = namedRow(published, "methods", "proposal");
    EXPECT_EQ(row.get("ac.formal_proposals"), view.getFormalProposals());
    EXPECT_EQ(array(row, "ac.result_constraints").size(), count + 2);
    auto imported = replaceDefinition(child, published);
    structureAccepts(imported, published);
    auto resolved = capture(
        [&] { return lookupSourceMethod(imported, "proposal", imported); });
    ASSERT_TRUE(mlir::succeeded(resolved)) << diagnostics;
    EXPECT_EQ(resolved->getNumPublicResults(), count);
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalProposalSchemaMutationsRejectInDefinitionsAndInterfaces) {
  for (unsigned mutation = 0; mutation != 11; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto method = formalProposal(child, "none", 0);
    SourceMethodSummariesAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    auto published = summary(analysis, child);
    ASSERT_TRUE(published);
    auto baseline = namedRow(published, "methods", "proposal");
    llvm::StringRef key = "ac.formal_proposals";
    mlir::Attribute bad;
    switch (mutation) {
    case 0:
      bad = builder.getStringAttr("0");
      break;
    case 1:
      bad = builder.getArrayAttr({builder.getI32IntegerAttr(0)});
      break;
    case 2:
      bad = builder.getArrayAttr({builder.getI64IntegerAttr(-1)});
      break;
    case 3:
      bad = builder.getArrayAttr({builder.getI64IntegerAttr(1)});
      break;
    case 4:
      bad = builder.getArrayAttr(
          {builder.getI64IntegerAttr(0), builder.getI64IntegerAttr(0)});
      break;
    case 5:
      key = "ac.return_form";
      bad = builder.getStringAttr("single");
      break;
    case 6:
      key = "ac.return_form";
      bad = builder.getStringAttr("tuple");
      break;
    case 7: {
      key = "ac.result_constraints";
      auto values = array(baseline, key);
      auto logicalIntent = changed(dict(values[1]), "source_kind",
                                   builder.getStringAttr("boolean"));
      logicalIntent =
          changed(logicalIntent, "domain",
                  mlir::parseAttribute("#ac.source_domain<{kind = \"bool\"}>",
                                       &context));
      bad = builder.getArrayAttr({values[0], logicalIntent});
      break;
    }
    case 8: {
      key = "function_type";
      auto signature = method.getFunctionType();
      bad = mlir::TypeAttr::get(mlir::FunctionType::get(
          &context, signature.getInputs(),
          {signature.getResult(0), signature.getResult(0)}));
      // Keep the ordinary result constraint valid; the intent is now eight
      // bits.
      auto values =
          method->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints");
      method->setAttr("ac.result_constraints",
                      builder.getArrayAttr({values[0], values[0]}));
      baseline = changed(baseline, "ac.result_constraints",
                         builder.getArrayAttr({values[0], values[0]}));
      break;
    }
    case 9: {
      key = "ac.result_constraints";
      auto values = array(baseline, key);
      bad = builder.getArrayAttr({values[0]});
      break;
    }
    case 10: {
      key = "function_type";
      auto signature = method.getFunctionType();
      auto bit = signature.getResult(1);
      bad = mlir::TypeAttr::get(
          mlir::FunctionType::get(&context, signature.getInputs(), {bit, bit}));
      auto values =
          method->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints");
      method->setAttr("ac.result_constraints",
                      builder.getArrayAttr({values[1], values[1]}));
      baseline = changed(baseline, "ac.result_constraints",
                         builder.getArrayAttr({values[1], values[1]}));
      break;
    }
    }
    method->setAttr(key, bad);
    EXPECT_TRUE(mlir::failed(capture([&] { return method.verify(); })))
        << mutation;
    EXPECT_FALSE(diagnostics.empty());
    llvm::SmallVector<mlir::Attribute> rows;
    for (auto entry : array(published, "methods"))
      rows.push_back(dict(entry).getAs<mlir::StringAttr>("name").getValue() ==
                             "proposal"
                         ? changed(baseline, key, bad)
                         : entry);
    auto invalid = changed(published, "methods", builder.getArrayAttr(rows));
    structureRejects(child, invalid);
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalProposalDataCannotReinterpretLogicalKindsOrIntegerDomains) {
  for (auto sourceKind : {"boolean", "integer"}) {
    SCOPED_TRACE(sourceKind);
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto method = formalProposal(child, "none", 0);
    auto bit = child.getBody().front().getArgument(0).getType();
    auto physical = std::string(sourceKind) == "boolean"
                        ? bit
                        : method.getFunctionType().getInput(0);
    method->setAttr("function_type",
                    mlir::TypeAttr::get(mlir::FunctionType::get(
                        &context, {physical}, {physical, bit})));
    for (auto &region : method->getRegions())
      region.front().getArgument(0).setType(physical);
    auto params = method->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
    auto parameter = dict(params[0]);
    auto fixed = changed(dict(parameter.get("constraint")), "type",
                         mlir::TypeAttr::get(physical));
    auto logical =
        changed(fixed, "source_kind", builder.getStringAttr(sourceKind));
    logical = changed(
        logical, "domain",
        mlir::parseAttribute(std::string(sourceKind) == "boolean"
                                 ? "#ac.source_domain<{kind = \"bool\"}>"
                                 : "#ac.source_domain<{kind = \"integer\", "
                                   "lower = #ac.math_int<0>, "
                                   "upper = #ac.math_int<256>}>",
                             &context));
    method->setAttr(
        "ac.parameters",
        builder.getArrayAttr({changed(parameter, "constraint", logical)}));
    auto intent =
        method->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints")[1];
    method->setAttr("ac.result_constraints",
                    builder.getArrayAttr({logical, intent}));
    ASSERT_TRUE(mlir::succeeded(capture([&] { return method.verify(); })))
        << diagnostics;
    method->setAttr("ac.result_constraints",
                    builder.getArrayAttr({fixed, intent}));
    EXPECT_TRUE(mlir::failed(capture([&] { return method.verify(); })));
    EXPECT_NE(diagnostics.find("source kind and logical domain"),
              std::string::npos)
        << diagnostics;
    if (std::string(sourceKind) == "integer") {
      auto narrowed = changed(
          logical, "domain",
          mlir::parseAttribute(
              "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<0>, "
              "upper = #ac.math_int<128>}>",
              &context));
      method->setAttr("ac.result_constraints",
                      builder.getArrayAttr({narrowed, intent}));
      EXPECT_TRUE(mlir::failed(capture([&] { return method.verify(); })));
      // The ordinary source contract already rejects a domain that no longer
      // spans its physical Integer representation.
      EXPECT_NE(diagnostics.find("Integer domain"), std::string::npos)
          << diagnostics;
    }
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalProposalDataPreservesNominalTypeIdentity) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto method = formalProposal(child, "none", 0);
  auto declaration = *file->getOps<StructOp>().begin();
  auto *other = declaration->clone();
  other->setAttr("sym_name", builder.getStringAttr("OtherPacket"));
  file->getBody()->getOperations().insert(declaration->getIterator(), other);
  auto packet = mlir::parseType("!ac.struct<\"Packet\">", &context);
  auto otherPacket = mlir::parseType("!ac.struct<\"OtherPacket\">", &context);
  auto bit = child.getBody().front().getArgument(0).getType();
  method->setAttr("function_type", mlir::TypeAttr::get(mlir::FunctionType::get(
                                       &context, {packet}, {packet, bit})));
  for (auto &region : method->getRegions())
    region.front().getArgument(0).setType(packet);
  auto parameters = method->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  auto parameter = dict(parameters[0]);
  auto nominal = changed(dict(parameter.get("constraint")), "source_kind",
                         builder.getStringAttr("nominal"));
  nominal = changed(nominal, "type", mlir::TypeAttr::get(packet));
  method->setAttr(
      "ac.parameters",
      builder.getArrayAttr({changed(parameter, "constraint", nominal)}));
  auto intent =
      method->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints")[1];
  method->setAttr("ac.result_constraints",
                  builder.getArrayAttr({nominal, intent}));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return method.verify(); })))
      << diagnostics;
  method->setAttr("function_type",
                  mlir::TypeAttr::get(mlir::FunctionType::get(
                      &context, {packet}, {otherPacket, bit})));
  method->setAttr(
      "ac.result_constraints",
      builder.getArrayAttr(
          {changed(nominal, "type", mlir::TypeAttr::get(otherPacket)),
           intent}));
  EXPECT_TRUE(mlir::failed(capture([&] { return method.verify(); })));
  EXPECT_NE(diagnostics.find("match its input contract"), std::string::npos)
      << diagnostics;
}

TEST_F(MethodSummaryAuthorityTest,
       FormalWritesRetainEachRhsAndWholeCandidateInOrderedPublication) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto method = formalProposal(child, "none", 0, true);
  auto first = formalWrite(method, 21, true);
  auto second = formalWrite(method, 22);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_NE(first.getPayload().getType(), first.getCandidate().getType());
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto effects =
      array(namedRow(published, "methods", "proposal"), "body_effects");
  ASSERT_EQ(effects.size(), 2u);
  EXPECT_EQ(kind(dict(effects[0])), "formal_write");
  EXPECT_EQ(dict(effects[0]).size(), 9u);
  auto selected = array(dict(effects[0]), "selectors");
  ASSERT_EQ(selected.size(), 2u);
  EXPECT_EQ(kind(dict(selected[0])), "field");
  EXPECT_EQ(kind(dict(selected[1])), "any_element");
  EXPECT_EQ(dict(effects[0]).get("occurrence"), first.getOccurrence());
  EXPECT_EQ(dict(effects[1]).get("occurrence"), second.getOccurrence());
  EXPECT_EQ(dict(effects[0]).getAs<mlir::TypeAttr>("payload_type").getValue(),
            first.getPayload().getType());
  auto candidate = dict(dict(effects[1]).get("candidate"));
  // Value facts describe predicates only; the nominal candidate retains its
  // exact formal dependency without pretending to be a one-bit predicate fact.
  known(candidate, {0}, {});
  EXPECT_EQ(kind(dict(candidate.get("fact"))), "unknown");
  auto candidateType = first.getCandidate().getType();
  auto imported = replaceDefinition(child, published);
  structureAccepts(imported, published);
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    auto broken = selected;
    if (mutation == 0)
      broken = builder.getArrayAttr(
          {changed(dict(selected[0]), "name", builder.getStringAttr("missing")),
           selected[1]});
    if (mutation == 1)
      broken = builder.getArrayAttr(
          {selected[0], changed(dict(selected[1]), "index", {})});
    if (mutation == 2)
      broken = builder.getArrayAttr(
          {selected[0], changed(dict(selected[1]), "index_type",
                                mlir::TypeAttr::get(candidateType))});
    auto row = namedRow(published, "methods", "proposal");
    auto badEffects = builder.getArrayAttr(
        {changed(dict(effects[0]), "selectors", broken), effects[1]});
    llvm::SmallVector<mlir::Attribute> changedMethods;
    for (auto raw : array(published, "methods"))
      changedMethods.push_back(
          raw == row ? changed(dict(raw), "body_effects", badEffects) : raw);
    structureRejects(imported, changed(published, "methods",
                                       builder.getArrayAttr(changedMethods)));
  }
  // Locate by name rather than rely on source declaration placement.
  llvm::SmallVector<mlir::Attribute> methods;
  for (mlir::Attribute raw : array(published, "methods")) {
    auto row = dict(raw);
    if (row.getAs<mlir::StringAttr>("name").getValue() == "proposal") {
      auto badEffects = array(row, "body_effects");
      row = changed(row, "body_effects",
                    builder.getArrayAttr({changed(dict(badEffects[0]), "formal",
                                                  builder.getI64IntegerAttr(1)),
                                          badEffects[1]}));
    }
    methods.push_back(row);
  }
  auto invalid = changed(published, "methods", builder.getArrayAttr(methods));
  structureRejects(imported, invalid);
}

TEST_F(MethodSummaryAuthorityTest,
       ForwardedFormalSnapshotPublishesPairedPreviousAndWholeType) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto method = formalProposal(child, "none", 0);
  auto &body = method.getBody().front();
  builder.setInsertionPoint(body.getTerminator());
  mlir::OperationState state(method.getLoc(),
                             SourceFormalWriteOp::getOperationName());
  state.addOperands({body.getArgument(0), body.getArgument(1),
                     body.getArgument(0), body.getArgument(0)});
  state.addTypes(body.getArgument(0).getType());
  state.addAttribute("formal", builder.getI64IntegerAttr(0));
  state.addAttribute("selectors", builder.getArrayAttr({}));
  state.addAttribute("operandSegmentSizes",
                     builder.getDenseI32ArrayAttr({1, 1, 1, 0, 1}));
  state.addAttribute("occurrence", occurrence("Child", 21));
  state.addAttribute("ac.location", method->getAttr("ac.guard_location"));
  auto marker = mlir::cast<SourceFormalWriteOp>(builder.create(state));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_EQ(marker.getPrevious(), body.getArgument(0));
  EXPECT_EQ(marker.getSnapshot().getType(), marker.getCandidate().getType());
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  auto row = dict(
      array(namedRow(published, "methods", "proposal"), "body_effects")[0]);
  EXPECT_EQ(row.size(), 11u);
  EXPECT_TRUE(row.getAs<mlir::DictionaryAttr>("previous"));
  EXPECT_EQ(row.getAs<mlir::TypeAttr>("snapshot_type").getValue(),
            marker.getCandidate().getType());
  auto imported = replaceDefinition(child, published);
  structureAccepts(imported, published);
}

TEST_F(MethodSummaryAuthorityTest,
       ForwardedFormalSnapshotRejectsMissingPairAndChangedWholeType) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto method = formalProposal(module(*file, "Child"), "none", 0);
    auto &body = method.getBody().front();
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState state(method.getLoc(),
                               SourceFormalWriteOp::getOperationName());
    state.addOperands(
        {body.getArgument(0), body.getArgument(1), body.getArgument(0)});
    if (mutation != 1)
      state.addOperands(body.getArgument(mutation == 2 ? 1 : 0));
    if (mutation != 0)
      state.addTypes(body.getArgument(mutation == 3 ? 1 : 0).getType());
    state.addAttribute("formal", builder.getI64IntegerAttr(0));
    state.addAttribute("selectors", builder.getArrayAttr({}));
    state.addAttribute(
        "operandSegmentSizes",
        builder.getDenseI32ArrayAttr({1, 1, 1, 0, mutation == 1 ? 0 : 1}));
    state.addAttribute("occurrence", occurrence("Child", 21));
    state.addAttribute("ac.location", method->getAttr("ac.guard_location"));
    builder.create(state);
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalWriteSchemaRejectsWrongFormalCandidateMetadataAndPlacement) {
  for (unsigned mutation = 0; mutation != 8; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto method = formalProposal(module(*file, "Child"), "none", 0);
    auto marker = formalWrite(method, 21);
    switch (mutation) {
    case 0:
      marker->setAttr("formal", builder.getI64IntegerAttr(-1));
      break;
    case 1:
      marker->setAttr("formal", builder.getI64IntegerAttr(1));
      break;
    case 2:
      method->removeAttr("ac.formal_proposals");
      break;
    case 3:
      marker->setOperand(2, method.getBody().front().getArgument(1));
      break;
    case 4:
      marker->removeAttr("ac.location");
      break;
    case 5:
      marker->setAttr("ac.location", builder.getStringAttr("source.py"));
      break;
    case 6:
      marker->setAttr("state", builder.getStringAttr("count"));
      break;
    case 7: {
      auto &guard = method.getGuard().front();
      marker->setOperands(
          {guard.getArgument(0), guard.getArgument(1), guard.getArgument(0)});
      marker->moveBefore(guard.getTerminator());
      break;
    }
    }
    EXPECT_TRUE(mlir::failed(capture([&] { return marker.verify(); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalWriteSelectedSchemaRejectsChangedFieldIndexPayloadAndSegments) {
  for (unsigned mutation = 0; mutation != 6; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto method = formalProposal(module(*file, "Child"), "none", 0, true);
    auto marker = formalWrite(method, 21, true);
    auto selectors = marker.getSelectors();
    if (mutation == 0)
      marker->setAttr("selectors",
                      builder.getArrayAttr(
                          {builder.getDictionaryAttr({builder.getNamedAttr(
                               "field", builder.getStringAttr("missing"))}),
                           selectors[1]}));
    if (mutation == 1)
      marker->setAttr(
          "selectors",
          builder.getArrayAttr(
              {selectors[0], builder.getDictionaryAttr({builder.getNamedAttr(
                                 "index", builder.getI64IntegerAttr(1))})}));
    if (mutation == 2)
      marker->setOperand(3, marker.getCandidate());
    if (mutation == 3)
      marker->setOperand(0, marker.getCandidate());
    if (mutation == 4) {
      // Inherent segment properties ignore removeAttr(null) in MLIR 22.
      // Rebuild the serialized omission instead of performing a no-op edit.
      mlir::OperationState missing(marker.getLoc(),
                                   SourceFormalWriteOp::getOperationName());
      missing.addOperands(marker.getOperands());
      missing.addTypes(marker.getResultTypes());
      for (auto attr : marker->getAttrs())
        if (attr.getName().getValue() != "operandSegmentSizes")
          missing.addAttribute(attr.getName(), attr.getValue());
      mlir::OpBuilder at(marker);
      auto rebuilt = mlir::cast<SourceFormalWriteOp>(at.create(missing));
      marker.erase();
      marker = rebuilt;
    }
    if (mutation == 5)
      marker->setAttr("operandSegmentSizes",
                      builder.getDenseI32ArrayAttr({1, 1, 1, 0, 1}));
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(MethodSummaryAuthorityTest,
       FormalOwnerBindingRowsHaveClosedSortedLexicalSchema) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto method = formalProposal(module(*file, "Child"), "none", 0);
  auto proposals =
      method->getAttrOfType<mlir::ArrayAttr>("ac.formal_proposals");
  auto state = builder.getDictionaryAttr(
      {builder.getNamedAttr("formal", builder.getI64IntegerAttr(0)),
       builder.getNamedAttr("state", builder.getStringAttr("count"))});
  auto forwarded = changed(state, "state", {});
  forwarded = changed(forwarded, "caller_formal", builder.getI64IntegerAttr(0));
  auto check = [&](mlir::Attribute rows) {
    return capture([&] {
      return verifySourceFormalOwnerBindings(
          rows, 1, proposals, method.getFunctionType(), proposals,
          [](llvm::StringRef name) { return name == "count"; }, method);
    });
  };
  EXPECT_TRUE(mlir::succeeded(check(builder.getArrayAttr({state}))));
  EXPECT_TRUE(mlir::succeeded(check(builder.getArrayAttr({forwarded}))));
  for (mlir::Attribute invalid :
       {mlir::Attribute(builder.getStringAttr("count")),
        mlir::Attribute(builder.getArrayAttr({state, state})),
        mlir::Attribute(builder.getArrayAttr(
            {changed(state, "formal", builder.getI32IntegerAttr(0))})),
        mlir::Attribute(builder.getArrayAttr(
            {changed(state, "formal", builder.getI64IntegerAttr(1))})),
        mlir::Attribute(builder.getArrayAttr(
            {changed(state, "state", builder.getStringAttr("absent"))})),
        mlir::Attribute(builder.getArrayAttr({changed(
            forwarded, "caller_formal", builder.getI64IntegerAttr(1))})),
        mlir::Attribute(builder.getArrayAttr({changed(
            forwarded, "caller_formal", builder.getI32IntegerAttr(0))})),
        mlir::Attribute(builder.getArrayAttr({changed(
            state, "caller_formal", builder.getI64IntegerAttr(0))}))}) {
    EXPECT_TRUE(mlir::failed(check(invalid)));
    EXPECT_FALSE(diagnostics.empty());
  }
  EXPECT_TRUE(mlir::failed(capture([&] {
    return verifySourceFormalOwnerBindings(
        builder.getArrayAttr({state}), 1, mlir::ArrayAttr(), {}, {},
        [](llvm::StringRef) { return true; }, method);
  })));
}

TEST_F(MethodSummaryAuthorityTest,
       PrivateMethodVisibilityPublishesAndRejectsCrossInstanceCalls) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child"), root = module(*file, "Root");
  auto method = formalProposal(child, "none", 0);
  method->setAttr("sym_visibility", builder.getStringAttr("private"));
  SourceMethodSummariesAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid());
  auto published = summary(analysis, child);
  ASSERT_TRUE(published);
  EXPECT_EQ(namedRow(published, "methods", "proposal").get("visibility"),
            builder.getStringAttr("private"));
  auto relay = named<SourceMethodOp>(child, "relay");
  relay->setAttr("sym_visibility", builder.getStringAttr("private"));
  auto local = *named<SourceMethodOp>(child, "act")
                    .getBody()
                    .front()
                    .getOps<SourceMethodCallOp>()
                    .begin();
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return resolveSourceMethodCall(local);
  }))) << diagnostics;
  auto calls = named<SourceTransactionOp>(root, "run")
                   .getBody()
                   .front()
                   .getOps<SourceMethodCallOp>();
  auto external = *std::next(calls.begin());
  EXPECT_TRUE(
      mlir::failed(capture([&] { return resolveSourceMethodCall(external); })));
  EXPECT_NE(diagnostics.find("private Method"), std::string::npos)
      << diagnostics;
  method->setAttr("sym_visibility", builder.getStringAttr("nested"));
  EXPECT_TRUE(mlir::failed(capture([&] { return method.verify(); })));
  auto imported = replaceDefinition(child, published);
  structureAccepts(imported, published);
  auto found = capture(
      [&] { return lookupSourceMethod(imported, "proposal", imported); });
  ASSERT_TRUE(mlir::succeeded(found));
  EXPECT_EQ(found->getVisibility(), builder.getStringAttr("private"));
}

} // namespace
} // namespace acir::ac
