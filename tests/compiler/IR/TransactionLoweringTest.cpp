#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "Dialect/ACIR/SourceTransactionSummary.h"
#include "Transforms/LowerTransactions.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace acir::ac {
namespace {
namespace compiler = acir::compiler;

// Intermediate-IR tests of the registered native transform. Public execution
// evidence belongs to the independent source/dual-target test lane.
std::string loweringFixture() {
  std::string text;
  for (unsigned value : {0u, 1u, 8u})
    text += "#w" + std::to_string(value) +
            " = #ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<" +
            std::to_string(value) +
            ">}, origin = {site = {definition = @Owner, ast_path = []}, "
            "expansion = []}, location = {path = \"lower.py\", line = 1 : i64, "
            "column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>\n";
  text += R"ir(
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
#byte = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
#span = {path = "lower.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
)ir";
  for (unsigned i = 0; i < 32; ++i)
    text += "#o" + std::to_string(i) +
            " = {site = {definition = @Owner, ast_path = [{kind = \"index\", "
            "value = " +
            std::to_string(i) + " : i64}]}, expansion = []}\n";
  text += R"ir(
module attributes {ac.source_units = [{package = "", path = "lower.py"}]} {
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%seed: !b8, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %no = "ac.bits.constant"() {value = #w0} : () -> !b1
    %bias = "ac.bits.constant"() {value = #w1} : () -> !b8
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b8
    %q = "ac.state"(%clk, %rst, %zero) {instance_name = "count", occurrence = #o0} : (!b1, !b1, !b8) -> !b8
    %saved = "ac.state"(%clk, %rst, %zero) {instance_name = "saved", occurrence = #o1} : (!b1, !b1, !b8) -> !b8
    %seen = "ac.state"(%clk, %rst, %zero) {instance_name = "seen", occurrence = #o2} : (!b1, !b1, !b8) -> !b8
    %hold = "ac.state"(%clk, %rst, %bias) {instance_name = "hold", occurrence = #o3} : (!b1, !b1, !b8) -> !b8
    "ac.method"(%yes, %bias) ({
    ^bb0(%delta: !b8, %path: !b1, %lexical_bias: !b8):
      %guard_read = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #o5} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1, %lexical_bias: !b8):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #o6} : (!b1) -> !b8
      %step = "ac.bits.binary"(%delta, %lexical_bias) {opcode = "add"} : (!b8, !b8) -> !b8
      %next = "ac.bits.binary"(%old, %step) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.state.write"(%path, %next) {state = "count", selectors = [], occurrence = #o7} : (!b1, !b8) -> ()
      "ac.yield"(%next) : (!b8) -> ()
    }) {sym_name = "bump", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #o4, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", ac.guard_location = #span, occurrence = #o4} : (!b1, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %nested = "ac.method.call"(%path, %delta) {callee = @Owner::@bump, instance_path = [], occurrence = #o9} : (!b1, !b8) -> !b8
      "ac.yield"(%nested) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #o8, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", ac.guard_location = #span, occurrence = #o8} : (!b1) -> ()
    "ac.method"(%rst) ({
    ^bb0(%admit: !b1):
      "ac.yield"(%admit) : (!b1) -> ()
    }, {
    ^bb0(%admit: !b1):
      "ac.yield"() : () -> ()
    }) {sym_name = "uncalled", function_type = () -> (), argument_names = [], ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", ac.guard_location = #span, occurrence = #o10} : (!b1) -> ()
    %first, %last = "ac.transaction"(%clk, %rst, %yes, %seed) ({
    ^bb0(%path: !b1, %caller_bias: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %caller_bias: !b8):
      %first = "ac.method.call"(%path, %caller_bias) {callee = @Owner::@bump, instance_path = [], occurrence = #o13} : (!b1, !b8) -> !b8
      %last = "ac.method.call"(%path, %first) {callee = @Owner::@relay, instance_path = [], occurrence = #o14} : (!b1, !b8) -> !b8
      "ac.state.write"(%path, %first) {state = "saved", selectors = [], occurrence = #o15} : (!b1, !b8) -> ()
      "ac.yield"(%first, %last) : (!b8, !b8) -> ()
    }) {name = "run", declaration = #o11, occurrence = #o12, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> (!b8, !b8)
    "ac.transaction"(%clk, %rst, %yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #o18} : (!b1) -> !b8
      "ac.state.write"(%path, %old) {state = "seen", selectors = [], occurrence = #o19} : (!b1, !b8) -> ()
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #o16, occurrence = #o17, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> ()
    "ac.yield"(%q, %first, %last, %hold) : (!b8, !b8, !b8, !b8) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "lower.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1) -> (!b8, !b8, !b8, !b8), input_names = ["seed", "pyc_clk", "pyc_rst"], output_names = ["count", "first", "last", "hold"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
  "ac.system"() {entry = {callee = @Owner, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
  return text;
}

} // namespace

namespace test {
std::string transactionLoweringFixture() { return loweringFixture(); }
} // namespace test

namespace {

struct InputIdentity {
  mlir::Operation *operation;
  mlir::DictionaryAttr attributes;
  mlir::Location location;
  std::vector<mlir::Value> operands, results, arguments;
  std::vector<mlir::Region *> regions;
  std::vector<mlir::Block *> blocks;
};

class TransactionLoweringTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OpBuilder builder{&context};
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ACIRDialect>(); }
  template <typename Function> decltype(auto) capture(Function function) {
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
        loweringFixture(), mlir::ParserConfig(&context, false));
    EXPECT_TRUE(file);
    if (file)
      EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
    return file;
  }
  ModuleOp owner(mlir::ModuleOp file) {
    auto *definition = mlir::SymbolTable::lookupSymbolIn(file, "Owner");
    if (!definition)
      definition = mlir::SymbolTable::lookupSymbolIn(file, "lower.Owner");
    return mlir::cast<ModuleOp>(definition);
  }
  template <typename Op> Op named(mlir::ModuleOp file, llvm::StringRef name) {
    for (auto op : owner(file).getBody().front().getOps<Op>()) {
      auto sym = op->template getAttrOfType<mlir::StringAttr>("sym_name");
      auto plain = op->template getAttrOfType<mlir::StringAttr>("name");
      if ((sym && sym.getValue() == name) ||
          (plain && plain.getValue() == name))
        return op;
    }
    ADD_FAILURE() << "missing " << name.str();
    return {};
  }
  InstanceOp leaf(mlir::ModuleOp file, llvm::StringRef name) {
    for (auto op : owner(file).getBody().front().getOps<InstanceOp>())
      if (op.getInstanceName() == name)
        return op;
    ADD_FAILURE() << "missing leaf " << name.str();
    return {};
  }
  template <typename Op> Op first(mlir::Region &region) {
    return *region.front().getOps<Op>().begin();
  }
  std::string print(mlir::Operation *op, bool debug = false) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    mlir::OpPrintingFlags flags;
    if (debug)
      flags.enableDebugInfo();
    op->print(stream, flags);
    stream << '\n';
    return text;
  }
  std::vector<InputIdentity> identities(mlir::ModuleOp file) {
    std::vector<InputIdentity> identities;
    file.walk([&](mlir::Operation *op) {
      InputIdentity record{
          op, op->getAttrDictionary(), op->getLoc(), {}, {}, {}, {}, {}};
      record.operands.assign(op->operand_begin(), op->operand_end());
      record.results.assign(op->result_begin(), op->result_end());
      for (auto &region : op->getRegions()) {
        record.regions.push_back(&region);
        for (auto &block : region) {
          record.blocks.push_back(&block);
          for (auto arg : block.getArguments())
            record.arguments.push_back(arg);
        }
      }
      identities.push_back(std::move(record));
    });
    return identities;
  }
  void unchanged(mlir::ModuleOp file,
                 const std::vector<InputIdentity> &before) {
    auto after = identities(file);
    ASSERT_EQ(after.size(), before.size());
    for (unsigned i = 0; i < before.size(); ++i) {
      EXPECT_EQ(after[i].operation, before[i].operation);
      EXPECT_EQ(after[i].attributes, before[i].attributes);
      EXPECT_EQ(after[i].location, before[i].location);
      EXPECT_EQ(after[i].operands, before[i].operands);
      EXPECT_EQ(after[i].results, before[i].results);
      EXPECT_EQ(after[i].arguments, before[i].arguments);
      EXPECT_EQ(after[i].regions, before[i].regions);
      EXPECT_EQ(after[i].blocks, before[i].blocks);
    }
  }
  mlir::LogicalResult lower(mlir::ModuleOp file) {
    return capture([&] {
      mlir::PassManager manager(&context);
      manager.addPass(acir::createRouteTransactionsPass());
      manager.addPass(acir::createLowerTransactionsPass());
      return manager.run(file);
    });
  }
  void rejects(mlir::ModuleOp file, llvm::StringRef reason = {}) {
    auto before = identities(file);
    auto text = print(file.getOperation());
    EXPECT_TRUE(mlir::failed(lower(file))) << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
    if (!reason.empty())
      EXPECT_NE(diagnostics.find(reason.str()), std::string::npos)
          << diagnostics;
    unchanged(file, before);
    EXPECT_EQ(text, print(file.getOperation()));
    EXPECT_FALSE(file->hasAttr("ac.stage"));
  }
  mlir::Value captureActual(RuleOp rule, mlir::Value value) {
    auto arg = mlir::dyn_cast<mlir::BlockArgument>(value);
    if (!arg || arg.getOwner() != &rule.getBody().front())
      return {};
    return rule->getOperand(arg.getArgNumber());
  }
  mlir::Value yielded(InstanceOp instance, unsigned pin = 3) {
    auto value = instance->getOperand(pin);
    auto result = mlir::dyn_cast<mlir::OpResult>(value);
    if (!result)
      return value;
    if (auto rule = mlir::dyn_cast<RuleOp>(result.getOwner()))
      return rule.getBody().front().getTerminator()->getOperand(
          result.getResultNumber());
    return value;
  }
  ModuleOp module(mlir::ModuleOp file, llvm::StringRef name) {
    return mlir::cast<ModuleOp>(mlir::SymbolTable::lookupSymbolIn(file, name));
  }
  template <typename Op> Op named(ModuleOp definition, llvm::StringRef name) {
    for (auto op : definition.getBody().front().getOps<Op>()) {
      auto symbol = op->template getAttrOfType<mlir::StringAttr>("sym_name");
      auto plain = op->template getAttrOfType<mlir::StringAttr>("name");
      if ((symbol && symbol.getValue() == name) ||
          (plain && plain.getValue() == name))
        return op;
    }
    ADD_FAILURE() << "missing " << name.str();
    return {};
  }
  InstanceOp instance(ModuleOp definition, llvm::StringRef name) {
    for (auto op : definition.getBody().front().getOps<InstanceOp>())
      if (op.getInstanceName() == name)
        return op;
    ADD_FAILURE() << "missing instance " << name.str();
    return {};
  }
  ModuleOp emptyModule(mlir::ModuleOp file, ModuleOp model,
                       llvm::StringRef name, mlir::TypeRange results) {
    builder.setInsertionPointToEnd(file.getBody());
    mlir::OperationState state(model.getLoc(), ModuleOp::getOperationName());
    state.addAttributes(model->getAttrs());
    state.attributes.set("sym_name", builder.getStringAttr(name));
    state.attributes.set("function_type",
                         mlir::TypeAttr::get(builder.getFunctionType(
                             model.getFunctionType().getInputs(), results)));
    llvm::SmallVector<mlir::Attribute> names;
    for (unsigned i = 0; i < results.size(); ++i)
      names.push_back(builder.getStringAttr("out" + std::to_string(i)));
    state.attributes.set("output_names", builder.getArrayAttr(names));
    state.addRegion();
    auto definition = mlir::cast<ModuleOp>(builder.create(state));
    auto *block = new mlir::Block;
    definition.getBody().push_back(block);
    for (auto type : definition.getFunctionType().getInputs())
      block->addArgument(type, definition.getLoc());
    return definition;
  }
  InstanceOp addInstance(ModuleOp parent, ModuleOp child, llvm::StringRef name,
                         mlir::ValueRange inputs, unsigned index) {
    builder.setInsertionPointToEnd(&parent.getBody().front());
    mlir::OperationState state(parent.getLoc(), InstanceOp::getOperationName());
    state.addOperands(inputs);
    state.addTypes(child.getFunctionType().getResults());
    state.addAttribute(
        "callee", mlir::FlatSymbolRefAttr::get(&context, child.getSymName()));
    state.addAttribute("instance_name", builder.getStringAttr(name));
    state.addAttribute("parameters", builder.getArrayAttr({}));
    state.addAttribute("type_arguments", builder.getArrayAttr({}));
    state.addAttribute("occurrence", occurrence(parent.getSymName(), index));
    return mlir::cast<InstanceOp>(builder.create(state));
  }
  BitsBinaryOp add(mlir::Value left, mlir::Value right, mlir::Location loc) {
    mlir::OperationState state(loc, BitsBinaryOp::getOperationName());
    state.addOperands({left, right});
    state.addTypes(left.getType());
    state.addAttribute("opcode", builder.getStringAttr("add"));
    return mlir::cast<BitsBinaryOp>(builder.create(state));
  }
  void yield(mlir::ValueRange values, mlir::Location loc) {
    mlir::OperationState state(loc, YieldOp::getOperationName());
    state.addOperands(values);
    builder.create(state);
  }
  SourceMethodCallOp call(mlir::Value path, mlir::ValueRange args,
                          ModuleOp lexical, llvm::StringRef provider,
                          llvm::StringRef method, llvm::StringRef receiver,
                          mlir::TypeRange results, unsigned index) {
    mlir::OperationState state(lexical.getLoc(),
                               SourceMethodCallOp::getOperationName());
    state.addOperands(path);
    state.addOperands(args);
    state.addTypes(results);
    state.addAttribute("callee",
                       mlir::SymbolRefAttr::get(
                           &context, provider,
                           {mlir::FlatSymbolRefAttr::get(&context, method)}));
    state.addAttribute("instance_path",
                       builder.getArrayAttr({builder.getStringAttr(receiver)}));
    state.addAttribute("occurrence", occurrence(lexical.getSymName(), index));
    return mlir::cast<SourceMethodCallOp>(builder.create(state));
  }
  // Every hierarchy hop crosses its immediate child's public Method boundary.
  // The wrapper keeps the provider's scalar contract and adds one real frame.
  SourceMethodOp forwardingMethod(ModuleOp lexical, SourceMethodOp contract,
                                  ModuleOp child, llvm::StringRef receiver,
                                  llvm::StringRef name, unsigned index) {
    auto *copy = contract->clone();
    auto wrapper = mlir::cast<SourceMethodOp>(copy);
    wrapper->setAttr("sym_name", builder.getStringAttr(name));
    wrapper->setAttr("occurrence", occurrence(lexical.getSymName(), index));
    llvm::SmallVector<mlir::Attribute> parameters;
    for (auto raw : wrapper->getAttrOfType<mlir::ArrayAttr>("ac.parameters")) {
      mlir::NamedAttrList fields(mlir::cast<mlir::DictionaryAttr>(raw));
      fields.set("origin", occurrence(lexical.getSymName(), index));
      parameters.push_back(fields.getDictionary(&context));
    }
    wrapper->setAttr("ac.parameters", builder.getArrayAttr(parameters));
    auto *yes = contract.getCaptures()[0].getDefiningOp()->clone();
    auto &body = lexical.getBody().front();
    auto where = body.empty() || !body.back().hasTrait<mlir::OpTrait::IsTerminator>()
                     ? body.end() : std::prev(body.end());
    body.getOperations().insert(where, yes);
    wrapper->setOperands(yes->getResults());
    for (auto &region : wrapper->getRegions()) {
      region.getBlocks().clear();
      auto *local = new mlir::Block;
      region.push_back(local);
      for (auto type : wrapper.getFunctionType().getInputs())
        local->addArgument(type, wrapper.getLoc());
      auto path = local->addArgument(yes->getResult(0).getType(), wrapper.getLoc());
      builder.setInsertionPointToEnd(local);
      if (&region == &wrapper.getGuard())
        yield(path, wrapper.getLoc());
      else {
        auto nested = call(path, local->getArguments().drop_back(), lexical,
                           child.getSymName(), contract.getSymName(), receiver,
                           wrapper.getFunctionType().getResults(), index + 1);
        yield(nested->getResults(), wrapper.getLoc());
      }
    }
    body.getOperations().insert(where, copy);
    return wrapper;
  }
  // A provider seed is a lexical ordinary value, deliberately different from
  // the caller's delta. The same definition receives different seeds at its
  // two occurrences. No test oracle depends on generated port names/order.
  mlir::OwningOpRef<mlir::ModuleOp> crossFixture() {
    auto file = parse();
    if (!file)
      return file;
    auto provider = owner(*file);
    auto &body = provider.getBody().front();
    auto count = first<SourceStateOp>(provider.getBody());
    body.getTerminator()->setOperands(count.getQ());
    provider->setAttr(
        "function_type",
        mlir::TypeAttr::get(builder.getFunctionType(
            provider.getFunctionType().getInputs(), {count.getQ().getType()})));
    provider->setAttr("output_names",
                      builder.getArrayAttr({builder.getStringAttr("count")}));
    for (auto transaction :
         llvm::make_early_inc_range(body.getOps<SourceTransactionOp>()))
      transaction->erase();
    for (auto state : llvm::make_early_inc_range(body.getOps<SourceStateOp>()))
      if (state != count)
        state->erase();
    auto bump = named<SourceMethodOp>(provider, "bump");
    bump->setOperand(1, body.getArgument(0));
    named<SourceMethodOp>(provider, "relay")->erase();
    auto peek = named<SourceMethodOp>(provider, "uncalled");
    peek->setAttr("sym_name", builder.getStringAttr("peek"));
    auto yes = *body.getOps<BitsConstantOp>().begin();
    peek->setOperands({yes.getResult(), count.getQ()});
    peek->setAttr("function_type", mlir::TypeAttr::get(builder.getFunctionType(
                                       {}, {count.getQ().getType()})));
    peek->setAttr("ac.result_constraints",
                  bump->getAttr("ac.result_constraints"));
    peek->setAttr("ac.return_form", builder.getStringAttr("single"));
    for (auto &region : peek->getRegions())
      region.front().addArgument(count.getQ().getType(), peek.getLoc());
    peek.getBody().front().getTerminator()->setOperands(
        peek.getBody().front().getArgument(1));

    auto byte = count.getQ().getType();
    auto relay = emptyModule(*file, provider, "Relay", {byte});
    auto &relayBody = relay.getBody().front();
    builder.setInsertionPointToEnd(&relayBody);
    auto doubled =
        add(relayBody.getArgument(0), relayBody.getArgument(0), relay.getLoc());
    auto counter = addInstance(relay, provider, "counter",
                               {doubled.getResult(), relayBody.getArgument(1),
                                relayBody.getArgument(2)},
                               0);
    auto *relayYes = yes->clone();
    relayBody.push_back(relayYes);
    auto *methodCopy = bump->clone();
    auto forward = mlir::cast<SourceMethodOp>(methodCopy);
    forward->setAttr("sym_name", builder.getStringAttr("forward"));
    forward->setAttr("occurrence", occurrence("Relay", 1));
    auto parameter = mlir::cast<mlir::DictionaryAttr>(
        forward->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
    mlir::NamedAttrList fields(parameter);
    fields.set("origin", occurrence("Relay", 1));
    forward->setAttr("ac.parameters",
                     builder.getArrayAttr({fields.getDictionary(&context)}));
    forward->setOperands(relayYes->getResults());
    for (auto &region : forward->getRegions()) {
      region.getBlocks().clear();
      auto *local = new mlir::Block;
      region.push_back(local);
      local->addArgument(byte, relay.getLoc());
      local->addArgument(yes.getResult().getType(), relay.getLoc());
      builder.setInsertionPointToEnd(local);
      if (&region == &forward.getGuard())
        yield(local->getArgument(1), relay.getLoc());
      else {
        auto nested = call(local->getArgument(1), local->getArgument(0), relay,
                           "Owner", "bump", "counter", {byte}, 2);
        yield(nested->getResults(), relay.getLoc());
      }
    }
    relayBody.push_back(methodCopy);
    forwardingMethod(relay, peek, provider, "counter", "peek", 20);
    builder.setInsertionPointToEnd(&relayBody);
    yield(counter->getResults(), relay.getLoc());

    auto root = emptyModule(*file, provider, "Root", {byte, byte, byte, byte});
    auto &rootBody = root.getBody().front();
    builder.setInsertionPointToEnd(&rootBody);
    auto rightSeed =
        add(rootBody.getArgument(0), rootBody.getArgument(0), root.getLoc());
    addInstance(root, relay, "left", rootBody.getArguments(), 0);
    addInstance(root, relay, "right",
                {rightSeed.getResult(), rootBody.getArgument(1),
                 rootBody.getArgument(2)},
                1);
    auto *rootYes = yes->clone();
    rootBody.push_back(rootYes);
    llvm::SmallVector<mlir::Value> results;
    for (unsigned side = 0; side < 2; ++side) {
      builder.setInsertionPointToEnd(&rootBody);
      mlir::OperationState state(root.getLoc(),
                                 SourceTransactionOp::getOperationName());
      state.addOperands({rootBody.getArgument(1), rootBody.getArgument(2),
                         rootYes->getResult(0), rootBody.getArgument(0)});
      state.addTypes(llvm::SmallVector<mlir::Type>(side ? 1 : 3, byte));
      state.addAttribute("name",
                         builder.getStringAttr(side ? "right_run" : "run"));
      state.addAttribute("declaration", occurrence("Root", 2 + side * 8));
      state.addAttribute("occurrence", occurrence("Root", 3 + side * 8));
      state.addAttribute("ac.guard_location",
                         bump->getAttr("ac.guard_location"));
      state.addRegion();
      state.addRegion();
      auto transaction = mlir::cast<SourceTransactionOp>(builder.create(state));
      for (auto &region : transaction->getRegions()) {
        auto *local = new mlir::Block;
        region.push_back(local);
        auto path =
            local->addArgument(yes.getResult().getType(), root.getLoc());
        auto delta = local->addArgument(byte, root.getLoc());
        builder.setInsertionPointToEnd(local);
        if (&region == &transaction.getGuard()) {
          yield(path, root.getLoc());
          continue;
        }
        auto firstCall = call(path, delta, root, "Relay", "forward",
                              side ? "right" : "left", {byte}, 4 + side * 8);
        if (side) {
          yield(firstCall->getResults(), root.getLoc());
        } else {
          auto secondCall = call(path, firstCall->getResults(), root, "Relay",
                                 "forward", "left", {byte}, 5);
          auto old = call(path, {}, root, "Relay", "peek", "left", {byte}, 6);
          yield({firstCall->getResult(0), secondCall->getResult(0),
                 old->getResult(0)},
                root.getLoc());
        }
      }
      results.append(transaction->result_begin(), transaction->result_end());
    }
    builder.setInsertionPointToEnd(&rootBody);
    yield(results, root.getLoc());
    auto system = *file->getOps<SystemOp>().begin();
    mlir::NamedAttrList entry(system.getEntry());
    entry.set("callee", mlir::FlatSymbolRefAttr::get(&context, "Root"));
    system->setAttr("entry", entry.getDictionary(&context));
    EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    return file;
  }
  SourceTransactionOp observer(ModuleOp provider, llvm::StringRef destination) {
    auto state = first<SourceStateOp>(provider.getBody());
    auto yes = *provider.getBody().front().getOps<BitsConstantOp>().begin();
    builder.setInsertionPoint(provider.getBody().front().getTerminator());
    mlir::OperationState transaction(provider.getLoc(),
                                     SourceTransactionOp::getOperationName());
    transaction.addOperands({state.getClk(), state.getRst(), yes.getResult()});
    transaction.addAttribute("name", builder.getStringAttr("autonomous"));
    transaction.addAttribute("declaration", occurrence("Owner", 40));
    transaction.addAttribute("occurrence", occurrence("Owner", 41));
    transaction.addAttribute(
        "ac.guard_location",
        named<SourceMethodOp>(provider, "bump")->getAttr("ac.guard_location"));
    transaction.addRegion();
    transaction.addRegion();
    auto op = mlir::cast<SourceTransactionOp>(builder.create(transaction));
    for (unsigned i = 0; i < 2; ++i) {
      auto *local = new mlir::Block;
      op->getRegion(i).push_back(local);
      auto path =
          local->addArgument(yes.getResult().getType(), provider.getLoc());
      builder.setInsertionPointToEnd(local);
      if (i == 1) {
        mlir::OperationState read(provider.getLoc(),
                                  SourceStateReadOp::getOperationName());
        read.addOperands(path);
        read.addTypes(state.getQ().getType());
        read.addAttribute("state", builder.getStringAttr("count"));
        read.addAttribute("selectors", builder.getArrayAttr({}));
        read.addAttribute("occurrence", occurrence("Owner", 42));
        auto *old = builder.create(read);
        mlir::OperationState write(provider.getLoc(),
                                   SourceStateWriteOp::getOperationName());
        write.addOperands({path, old->getResult(0)});
        write.addAttribute("state", builder.getStringAttr(destination));
        write.addAttribute("selectors", builder.getArrayAttr({}));
        write.addAttribute("occurrence", occurrence("Owner", 43));
        builder.create(write);
      }
      yield(i == 0 ? mlir::ValueRange(path) : mlir::ValueRange{},
            provider.getLoc());
    }
    return op;
  }
  CollectionOp addCollection(mlir::ModuleOp file, ModuleOp child) {
    auto grouped = emptyModule(file, child, "Grouped", {});
    auto yes = *owner(file).getBody().front().getOps<BitsConstantOp>().begin();
    auto shape = builder.getArrayAttr({yes.getValue()});
    llvm::SmallVector<mlir::Type> inputs, outputs;
    for (auto input : child.getFunctionType().getInputs())
      inputs.push_back(TableType::get(&context, shape, input));
    for (auto output : child.getFunctionType().getResults())
      outputs.push_back(TableType::get(&context, shape, output));
    grouped->setAttr(
        "function_type",
        mlir::TypeAttr::get(builder.getFunctionType(inputs, outputs)));
    grouped->removeAttr("ac.domain_inputs");
    grouped->setAttr("output_names", child.getOutputNames());
    for (auto [argument, type] :
         llvm::zip_equal(grouped.getBody().front().getArguments(), inputs))
      argument.setType(type);
    builder.setInsertionPointToEnd(&grouped.getBody().front());
    mlir::OperationState state(grouped.getLoc(),
                               CollectionOp::getOperationName());
    state.addOperands(grouped.getBody().front().getArguments());
    state.addTypes(outputs);
    state.addAttribute(
        "callee", mlir::FlatSymbolRefAttr::get(&context, child.getSymName()));
    state.addAttribute("instance_name", builder.getStringAttr("lanes"));
    state.addAttribute("parameters", builder.getArrayAttr({}));
    state.addAttribute("type_arguments", builder.getArrayAttr({}));
    state.addAttribute("shape", shape);
    state.addAttribute("occurrence", occurrence("Grouped", 0));
    auto collection = mlir::cast<CollectionOp>(builder.create(state));
    yield(collection->getResults(), grouped.getLoc());
    return collection;
  }
  struct EvaluationScope {
    ModuleOp definition;
    InstanceOp via;
    std::shared_ptr<EvaluationScope> parent;
    std::string path;
  };
  // Only the existing scalar combinational primitives are evaluated. Storage Q
  // is supplied independently per physical occurrence, never inferred from D.
  uint64_t evaluate(mlir::ModuleOp file, mlir::Value value,
                    const std::shared_ptr<EvaluationScope> &scope,
                    uint64_t seed = 3) {
    if (auto arg = mlir::dyn_cast<mlir::BlockArgument>(value)) {
      auto *parent = arg.getOwner()->getParentOp();
      if (mlir::isa<ModuleOp>(parent)) {
        if (scope->via)
          return evaluate(file, scope->via->getOperand(arg.getArgNumber()),
                          scope->parent, seed);
        return arg.getArgNumber() == 0 ? seed : arg.getArgNumber() == 1 ? 1 : 0;
      }
      if (auto rule = mlir::dyn_cast<RuleOp>(parent))
        return evaluate(file, rule->getOperand(arg.getArgNumber()), scope,
                        seed);
    }
    auto result = mlir::dyn_cast<mlir::OpResult>(value);
    if (result) {
      auto *op = result.getOwner();
      if (auto constant = mlir::dyn_cast<BitsConstantOp>(op)) {
        HardwareAnalysis hardware(file);
        HardwareBindings bindings;
        bindings.owner = scope->definition;
        auto literal =
            hardware.evaluateStatic(constant.getValue(), bindings, op);
        EXPECT_TRUE(mlir::succeeded(literal));
        if (mlir::succeeded(literal)) {
          if (auto boolean = mlir::dyn_cast<mlir::BoolAttr>(*literal))
            return boolean.getValue();
          return std::stoull(
              mlir::cast<MathIntAttr>(*literal).getCanonicalValue().str());
        }
      }
      if (mlir::isa<TableSplatOp, TableFoldOp>(op))
        return evaluate(file, op->getOperand(0), scope, seed);
      if (mlir::isa<StructCreateOp>(op)) {
        EXPECT_EQ(op->getNumOperands(), 1u);
        return evaluate(file, op->getOperand(0), scope, seed);
      }
      if (auto field = mlir::dyn_cast<StructGetOp>(op)) {
        EXPECT_EQ(field.getField(), "value");
        return evaluate(file, field->getOperand(0), scope, seed);
      }
      auto mask = [&](mlir::Type type) {
        HardwareAnalysis hardware(file);
        HardwareBindings bindings;
        bindings.owner = scope->definition;
        auto width = hardware.getPackedWidth(type, bindings, op);
        EXPECT_TRUE(mlir::succeeded(width));
        if (mlir::failed(width) || !*width || *width > 64) {
          ADD_FAILURE() << "unsupported oracle bit width";
          return uint64_t(0);
        }
        return *width == 64 ? std::numeric_limits<uint64_t>::max()
                            : (uint64_t(1) << *width) - 1;
      };
      if (auto merge = mlir::dyn_cast<ValueMergeOp>(op)) {
        // This oracle evaluates scalar, whole-value merges. Preserve the
        // independently chosen Q values when a routed proposal is disabled.
        EXPECT_EQ(merge.getGuards().size(), 1u);
        EXPECT_EQ(merge.getValues().size(), 1u);
        EXPECT_EQ(merge.getPaths().size(), 1u);
        if (merge.getGuards().size() != 1 || merge.getValues().size() != 1 ||
            merge.getPaths().size() != 1)
          return 0;
        EXPECT_TRUE(mlir::cast<mlir::ArrayAttr>(merge.getPaths()[0]).empty());
        auto enabled = evaluate(file, merge.getGuards()[0], scope, seed);
        if (value == merge.getEn())
          return enabled;
        auto selected = enabled ? merge.getValues()[0] : merge.getBase();
        return evaluate(file, selected, scope, seed) & mask(value.getType());
      }
      if (auto required = mlir::dyn_cast<ValueRequireOp>(op)) {
        // This two-state oracle evaluates the requirement, never its payload:
        // an unreached use succeeds; a reached use requires availability.
        return evaluate(file, required.getPath(), scope, seed)
                   ? evaluate(file, required.getAvailability(), scope, seed)
                   : 1;
      }
      if (auto binary = mlir::dyn_cast<BitsBinaryOp>(op)) {
        auto left = evaluate(file, binary->getOperand(0), scope, seed);
        auto right = evaluate(file, binary->getOperand(1), scope, seed);
        auto resultMask = mask(binary.getResult().getType());
        if (binary.getOpcode() == "add")
          return (left + right) & resultMask;
        if (binary.getOpcode() == "and")
          return (left & right) & resultMask;
        ADD_FAILURE() << "unsupported oracle binary opcode "
                      << binary.getOpcode().str();
        return 0;
      }
      if (auto compare = mlir::dyn_cast<BitsCompareOp>(op)) {
        if (compare.getPredicate() != "case_eq") {
          ADD_FAILURE() << "unsupported oracle comparison predicate "
                        << compare.getPredicate().str();
          return 0;
        }
        auto operandMask = mask(compare->getOperand(0).getType());
        auto left =
            evaluate(file, compare->getOperand(0), scope, seed) & operandMask;
        auto right =
            evaluate(file, compare->getOperand(1), scope, seed) & operandMask;
        return left == right;
      }
      if (auto rule = mlir::dyn_cast<RuleOp>(op))
        return evaluate(file,
                        rule.getBody().front().getTerminator()->getOperand(
                            result.getResultNumber()),
                        scope, seed);
      if (auto child = mlir::dyn_cast<InstanceOp>(op)) {
        auto *callee =
            mlir::SymbolTable::lookupSymbolIn(file, child.getCallee());
        if (auto imported = mlir::dyn_cast<ModuleImportOp>(callee)) {
          EXPECT_EQ(imported->getAttrOfType<mlir::StringAttr>("primitive_kind")
                        .getValue(),
                    "dffe");
          return scope->path == "left.counter" ? 11 : 19;
        }
        auto definition = mlir::cast<ModuleOp>(callee);
        auto next = std::make_shared<EvaluationScope>(EvaluationScope{
            definition, child, scope,
            scope->path.empty()
                ? child.getInstanceName().str()
                : scope->path + "." + child.getInstanceName().str()});
        return evaluate(
            file,
            definition.getBody().front().getTerminator()->getOperand(
                result.getResultNumber()),
            next, seed);
      }
    }
    if (auto *operation = value.getDefiningOp())
      ADD_FAILURE() << "unsupported oracle operation "
                    << operation->getName().getStringRef().str()
                    << " result " << mlir::cast<mlir::OpResult>(value).getResultNumber()
                    << ": " << print(operation);
    else
      ADD_FAILURE() << "unsupported oracle block argument "
                    << mlir::cast<mlir::BlockArgument>(value).getArgNumber();
    return 0;
  }
  mlir::DictionaryAttr occurrence(llvm::StringRef ownerName, unsigned index) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "site", builder.getDictionaryAttr(
                         {builder.getNamedAttr("definition",
                                               mlir::FlatSymbolRefAttr::get(
                                                   &context, ownerName)),
                          builder.getNamedAttr(
                              "ast_path",
                              builder.getArrayAttr({builder.getDictionaryAttr(
                                  {builder.getNamedAttr(
                                       "kind", builder.getStringAttr("index")),
                                   builder.getNamedAttr(
                                       "value", builder.getI64IntegerAttr(
                                                    index))})}))})),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  }
  mlir::DictionaryAttr span(unsigned line) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("lower.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
  }
  SourceTransactionOp addEmptyTransaction(mlir::ModuleOp file,
                                          llvm::StringRef name,
                                          unsigned occurrenceIndex) {
    auto definition = owner(file);
    auto &body = definition.getBody().front();
    auto yes = *body.getOps<BitsConstantOp>().begin();
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState state(definition.getLoc(),
                               SourceTransactionOp::getOperationName());
    state.addOperands(
        {body.getArgument(1), body.getArgument(2), yes.getResult()});
    state.addAttribute("name", builder.getStringAttr(name));
    state.addAttribute("declaration", occurrence("Owner", occurrenceIndex));
    state.addAttribute("occurrence", occurrence("Owner", occurrenceIndex + 1));
    state.addAttribute("ac.guard_location", span(occurrenceIndex));
    state.addRegion();
    state.addRegion();
    auto transaction = mlir::cast<SourceTransactionOp>(builder.create(state));
    for (unsigned region = 0; region != 2; ++region) {
      auto *block = new mlir::Block;
      transaction->getRegion(region).push_back(block);
      auto path =
          block->addArgument(yes.getResult().getType(), transaction.getLoc());
      builder.setInsertionPointToEnd(block);
      yield(region == 0 ? mlir::ValueRange(path) : mlir::ValueRange{},
            transaction.getLoc());
    }
    return transaction;
  }
  SourceBranchOp addNominalBranch(mlir::ModuleOp file,
                                  bool directEnum = false) {
    auto definition = owner(file);
    auto run = named<SourceTransactionOp>(file, "run");
    auto &body = run.getBody().front();
    auto byte = body.getArgument(1).getType();
    auto bit = body.getArgument(0).getType();
    auto tag = EnumType::get(&context, builder.getStringAttr("Tag"));
    auto inner = StructType::get(&context, builder.getStringAttr("Inner"));
    auto packet = StructType::get(&context, builder.getStringAttr("Packet"));
    builder.setInsertionPointToStart(file.getBody());
    mlir::OperationState enumeration(definition.getLoc(),
                                     EnumOp::getOperationName());
    enumeration.addAttribute("sym_name", builder.getStringAttr("Tag"));
    enumeration.addAttribute("width",
                             mlir::parseAttribute("#ac.math_int<8>", &context));
    enumeration.addAttribute("encoding", builder.getStringAttr("explicit"));
    enumeration.addAttribute(
        "members",
        builder.getArrayAttr({builder.getDictionaryAttr(
            {builder.getNamedAttr("name", builder.getStringAttr("ZERO")),
             builder.getNamedAttr(
                 "code",
                 mlir::parseAttribute("#ac.math_int<0>", &context))})}));
    builder.create(enumeration);
    auto declare =
        [&](llvm::StringRef name,
            llvm::ArrayRef<std::pair<llvm::StringRef, mlir::Type>> fields) {
          llvm::SmallVector<mlir::Attribute> rows;
          for (auto [field, type] : fields)
            rows.push_back(builder.getDictionaryAttr(
                {builder.getNamedAttr("name", builder.getStringAttr(field)),
                 builder.getNamedAttr("type", mlir::TypeAttr::get(type))}));
          mlir::OperationState declaration(definition.getLoc(),
                                           StructOp::getOperationName());
          declaration.addAttribute("sym_name", builder.getStringAttr(name));
          declaration.addAttribute("fields", builder.getArrayAttr(rows));
          builder.create(declaration);
        };
    declare("Inner", {{"data", byte}, {"tag", tag}});
    declare("Packet", {{"left", inner}, {"right", inner}});
    auto make = [&](llvm::StringRef operation, mlir::ValueRange operands,
                    mlir::TypeRange types) -> mlir::Operation * {
      mlir::OperationState state(run.getLoc(), operation);
      state.addOperands(operands);
      state.addTypes(types);
      return builder.create(state);
    };
    builder.setInsertionPointToStart(&body);
    auto left = make(EnumFromBitsOp::getOperationName(), body.getArgument(1),
                     {tag, bit});
    auto right = make(EnumFromBitsOp::getOperationName(), body.getArgument(1),
                      {tag, bit});
    mlir::Value a = left->getResult(0), b = right->getResult(0);
    if (!directEnum) {
      auto first = make(StructCreateOp::getOperationName(),
                        {body.getArgument(1), a}, {inner});
      auto second = make(StructCreateOp::getOperationName(),
                         {body.getArgument(1), b}, {inner});
      a = make(StructCreateOp::getOperationName(),
               {first->getResult(0), second->getResult(0)}, {packet})
              ->getResult(0);
      b = make(StructCreateOp::getOperationName(),
               {second->getResult(0), first->getResult(0)}, {packet})
              ->getResult(0);
    }
    mlir::OperationState state(run.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(body.getArgument(0));
    state.addTypes(a.getType());
    state.addAttribute("occurrence", occurrence("Owner", 24));
    state.addAttribute("ac.condition_location", span(24));
    state.addRegion();
    state.addRegion();
    auto split = mlir::cast<SourceBranchOp>(builder.create(state));
    for (auto [region, value] :
         llvm::zip(split->getRegions(), std::array<mlir::Value, 2>{a, b})) {
      auto *arm = new mlir::Block;
      region.push_back(arm);
      builder.setInsertionPointToEnd(arm);
      yield(value, run.getLoc());
    }
    return split;
  }
  // Shape-only variants share the nominal fixture's source provenance. Widen
  // repeated siblings independently of wrapping depth, then optionally put the
  // complete record in a Table. These are IR conformance fixtures, not source
  // authoring examples.
  SourceBranchOp addBranchShape(mlir::ModuleOp file, unsigned siblings,
                               unsigned wrappers, bool table) {
    auto split = addNominalBranch(file);
    auto *packet = mlir::SymbolTable::lookupSymbolIn(file, "Packet");
    auto fields = packet->getAttrOfType<mlir::ArrayAttr>("fields");
    auto inner = mlir::cast<mlir::DictionaryAttr>(fields[0])
                     .getAs<mlir::TypeAttr>("type");
    llvm::SmallVector<mlir::Attribute> wideFields;
    for (unsigned i = 0; i < siblings; ++i)
      wideFields.push_back(builder.getDictionaryAttr(
          {builder.getNamedAttr("name", builder.getStringAttr(
                                            "lane" + std::to_string(i))),
           builder.getNamedAttr("type", inner)}));
    packet->setAttr("fields", builder.getArrayAttr(wideFields));
    std::array<mlir::Value, 2> values;
    for (unsigned arm = 0; arm < 2; ++arm) {
      values[arm] = split->getRegion(arm).front().getTerminator()->getOperand(0);
      auto *create = values[arm].getDefiningOp();
      auto first = create->getOperand(0), second = create->getOperand(1);
      llvm::SmallVector<mlir::Value> operands;
      for (unsigned i = 0; i < siblings; ++i)
        operands.push_back(i % 2 ? second : first);
      create->setOperands(operands);
    }
    auto make = [&](llvm::StringRef name, mlir::Value input, mlir::Type type) {
      builder.setInsertionPoint(split);
      mlir::OperationState state(split.getLoc(), name);
      state.addOperands(input);
      state.addTypes(type);
      return builder.create(state)->getResult(0);
    };
    for (unsigned depth = 0; depth < wrappers; ++depth) {
      auto name = "BranchLevel" + std::to_string(depth);
      builder.setInsertionPointToStart(file.getBody());
      mlir::OperationState declaration(split.getLoc(),
                                       StructOp::getOperationName());
      declaration.addAttribute("sym_name", builder.getStringAttr(name));
      declaration.addAttribute(
          "fields",
          builder.getArrayAttr({builder.getDictionaryAttr(
              {builder.getNamedAttr("name", builder.getStringAttr("child")),
               builder.getNamedAttr(
                   "type", mlir::TypeAttr::get(values[0].getType()))})}));
      builder.create(declaration);
      auto type = StructType::get(&context, builder.getStringAttr(name));
      for (auto &value : values)
        value = make(StructCreateOp::getOperationName(), value, type);
    }
    if (table) {
      auto width = mlir::cast<BitsType>(
                       named<SourceTransactionOp>(file, "run")
                           .getBody().front().getArgument(1).getType())
                       .getWidth();
      auto type = TableType::get(&context, builder.getArrayAttr({width}),
                                 values[0].getType());
      for (auto &value : values) {
        value = make(TableSplatOp::getOperationName(), value, type);
        value.getDefiningOp()->setAttr("shape", type.getShape());
      }
    }
    split.getResult(0).setType(values[0].getType());
    for (unsigned arm = 0; arm < 2; ++arm)
      split->getRegion(arm).front().getTerminator()->setOperand(0, values[arm]);
    return split;
  }
  SourceBranchOp addBranchScenario(mlir::ModuleOp file) {
    auto definition = owner(file);
    auto run = named<SourceTransactionOp>(file, "run");
    auto &body = run.getBody().front();
    auto constants = definition.getBody().front().getOps<BitsConstantOp>();
    auto zero = *std::next(constants.begin(), 3);
    llvm::SmallVector<mlir::Value> captures(run->getOperands());
    captures.push_back(zero.getResult());
    run->setOperands(captures);
    for (auto &region : run->getRegions())
      region.front().addArgument(zero.getResult().getType(), run.getLoc());

    mlir::OperationState state(run.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(body.getArgument(0));
    state.addTypes(body.getArgument(1).getType());
    state.addAttribute("occurrence", occurrence("Owner", 24));
    state.addAttribute("ac.condition_location", span(24));
    state.addRegion();
    state.addRegion();
    builder.setInsertionPointToStart(&body);
    auto split = mlir::cast<SourceBranchOp>(builder.create(state));
    for (unsigned arm = 0; arm != 2; ++arm) {
      auto *block = new mlir::Block;
      split->getRegion(arm).push_back(block);
      builder.setInsertionPointToEnd(block);
      auto value = arm == 0 ? body.getArgument(1) : body.getArguments().back();
      mlir::OperationState write(run.getLoc(),
                                 SourceStateWriteOp::getOperationName());
      write.addOperands({body.getArgument(0), value});
      write.addAttribute("state", builder.getStringAttr("saved"));
      write.addAttribute("selectors", builder.getArrayAttr({}));
      write.addAttribute("occurrence", occurrence("Owner", 25 + arm));
      builder.create(write);
      yield(value, run.getLoc());
    }
    auto firstCall = *body.getOps<SourceMethodCallOp>().begin();
    firstCall->setOperand(1, split.getResult(0));
    for (auto write :
         llvm::make_early_inc_range(body.getOps<SourceStateWriteOp>()))
      if (write.getState() == "saved")
        write.erase();
    return split;
  }
};

mlir::Value extractChain(mlir::OpBuilder &builder, mlir::Value input,
                         mlir::Operation *before, StaticExprAttr low,
                         unsigned count = 1, mlir::Type output = {}) {
  builder.setInsertionPoint(before);
  if (!output)
    output = input.getType();
  for (unsigned i = 0; i < count; ++i) {
    mlir::OperationState operation(before->getLoc(),
                                   BitsExtractOp::getOperationName());
    operation.addOperands(input);
    operation.addTypes(output);
    operation.addAttribute("low", low);
    input = builder.create(operation)->getResult(0);
  }
  return input;
}

TEST_F(TransactionLoweringTest,
       RepeatedNestedCallsMaterializeCandidateSSAAndImmutableSnapshots) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto originalType = owner(*file).getFunctionType();
  auto state = first<SourceStateOp>(owner(*file).getBody());
  auto occurrenceBefore = state.getOccurrence();
  auto originalInit = state.getInitial().getDefiningOp()->getAttr("value");
  auto lexicalBias =
      (*std::next(
           owner(*file).getBody().front().getOps<BitsConstantOp>().begin(), 2))
          .getValue();
  auto runBefore = named<SourceTransactionOp>(*file, "run");
  EXPECT_FALSE(llvm::is_contained(runBefore.getCaptures(), state.getQ()));
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  EXPECT_EQ(file->getOperation()
                ->getAttrOfType<mlir::StringAttr>("ac.stage")
                .getValue(),
            "final");
  EXPECT_EQ(owner(*file).getFunctionType(), originalType);
  EXPECT_EQ(leaf(*file, "count").getOccurrence(), occurrenceBefore);
  // Successful atomic replacement invalidates old handles: compare init's
  // semantic carrier, rather than dereferencing its former operation.
  EXPECT_EQ(
      leaf(*file, "count")->getOperand(4).getDefiningOp()->getAttr("value"),
      originalInit);
  unsigned states = 0, methods = 0, transactions = 0, reads = 0, writes = 0,
           calls = 0;
  file->walk([&](mlir::Operation *op) {
    states += mlir::isa<SourceStateOp>(op);
    methods += mlir::isa<SourceMethodOp>(op);
    transactions += mlir::isa<SourceTransactionOp>(op);
    reads += mlir::isa<SourceStateReadOp>(op);
    writes += mlir::isa<SourceStateWriteOp>(op);
    calls += mlir::isa<SourceMethodCallOp>(op);
  });
  EXPECT_EQ(states + methods + transactions + reads + writes + calls, 0u);
  EXPECT_EQ(
      std::distance(owner(*file).getBody().front().getOps<InstanceOp>().begin(),
                    owner(*file).getBody().front().getOps<InstanceOp>().end()),
      4);
  auto run = named<RuleOp>(*file, "run");
  for (auto rule : owner(*file).getBody().front().getOps<RuleOp>())
    EXPECT_FALSE(rule->hasAttr("ac.guard_location"));
  auto count = leaf(*file, "count"), saved = leaf(*file, "saved");
  auto final = mlir::dyn_cast<BitsBinaryOp>(yielded(count).getDefiningOp());
  auto firstCandidate =
      mlir::dyn_cast<BitsBinaryOp>(yielded(saved).getDefiningOp());
  ASSERT_TRUE(final);
  ASSERT_TRUE(firstCandidate);
  EXPECT_EQ(final->getOperand(0), firstCandidate.getResult());
  auto firstStep = mlir::dyn_cast<BitsBinaryOp>(
      firstCandidate->getOperand(1).getDefiningOp());
  auto secondStep =
      mlir::dyn_cast<BitsBinaryOp>(final->getOperand(1).getDefiningOp());
  ASSERT_TRUE(firstStep);
  ASSERT_TRUE(secondStep);
  EXPECT_EQ(secondStep->getOperand(0), firstCandidate.getResult());
  EXPECT_EQ(captureActual(run, firstCandidate->getOperand(0)),
            count->getResult(0));
  EXPECT_EQ(captureActual(run, firstStep->getOperand(0)),
            owner(*file).getBody().front().getArgument(0));
  auto lexical = captureActual(run, firstStep->getOperand(1));
  ASSERT_TRUE(lexical);
  EXPECT_EQ(lexical.getDefiningOp()->getAttr("value"), lexicalBias);
  EXPECT_EQ(firstStep->getOperand(1), secondStep->getOperand(1));
  auto *moduleYield = owner(*file).getBody().front().getTerminator();
  auto firstOutput = mlir::cast<mlir::OpResult>(moduleYield->getOperand(1));
  EXPECT_EQ(run.getBody().front().getTerminator()->getOperand(
                firstOutput.getResultNumber()),
            firstCandidate.getResult());
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       NativePassLowersBranchDataIntentAndScalarResultFromOneSourceHistory) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto branch = addBranchScenario(*file);
  ASSERT_TRUE(branch);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto before = print(file->getOperation());
  EXPECT_NE(before.find("ac.branch"), std::string::npos);
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;

  unsigned residualBranches = 0;
  file->walk([&](SourceBranchOp) { ++residualBranches; });
  EXPECT_EQ(residualBranches, 0u);
  auto saved = leaf(*file, "saved");
  auto data = mlir::dyn_cast<BitsSelectOp>(yielded(saved, 3).getDefiningOp());
  auto enable = mlir::dyn_cast<BitsBinaryOp>(yielded(saved, 2).getDefiningOp());
  ASSERT_TRUE(data);
  ASSERT_TRUE(enable);
  EXPECT_EQ(enable.getOpcode(), "and");
  auto intent =
      mlir::dyn_cast<BitsSelectOp>(enable->getOperand(1).getDefiningOp());
  ASSERT_TRUE(intent);
  EXPECT_EQ(data.getCondition(), intent.getCondition());
  EXPECT_NE(data->getOperand(1), data->getOperand(2));
  EXPECT_NE(intent->getOperand(1), intent->getOperand(2));

  unsigned selects = 0;
  owner(*file).walk([&](BitsSelectOp) { ++selects; });
  EXPECT_GE(selects, 3u); // joined D, joined E, and escaped scalar result.
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       NominalBranchResultsHaveCanonicalOrderedGraphAndIndependentSourceProof) {
  for (bool directEnum : {false, true}) {
    for (unsigned mutation = 0; mutation != 17; ++mutation) {
      SCOPED_TRACE(directEnum);
      SCOPED_TRACE(mutation);
      auto file = parse();
      ASSERT_TRUE(file);
      auto split = addNominalBranch(*file, directEnum);
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
      auto before = identities(*file);
      SourceTransactionStateAnalysis analysis(file->getOperation());
      ASSERT_TRUE(analysis.isValid()) << diagnostics;
      HardwareSourceCheckLimits limits;
      detail::SourceCheckBudget preparationBudget(limits);
      auto prepared = capture([&] {
        return acir::detail::prepareTransactionLowering(*file, analysis,
                                                        preparationBudget);
      });
      ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
      detail::SourceCheckBudget materializationBudget(limits);
      auto candidate = capture([&] {
        return acir::detail::materializeTransactionLowering(
            *prepared, materializationBudget);
      });
      ASSERT_TRUE(mlir::succeeded(candidate)) << diagnostics;
      auto index =
          llvm::find_if(prepared->transactions,
                        [&](const auto &entry) {
                          return entry.plan->transaction ==
                                 split->getParentOfType<SourceTransactionOp>();
                        }) -
          prepared->transactions.begin();
      ASSERT_LT(static_cast<size_t>(index), candidate->transactions.size());
      auto &transaction = candidate->transactions[index];
      ASSERT_EQ(transaction.branchResults.size(), 1u);
      auto &record = transaction.branchResults[0];
      ASSERT_EQ(record.generated.size(), 1u);
      ASSERT_EQ(record.value, record.generated.back()->getResult(0));
      ASSERT_EQ(record.value.getType(), split.getResult(0).getType());
      for (auto *op : record.generated) {
        EXPECT_EQ(op->getBlock(), &transaction.rule.getBody().front());
        EXPECT_EQ(op->getLoc(), split.getLoc());
      }
      detail::SourceCheckBudget initialProof(limits);
      ASSERT_TRUE(mlir::succeeded(capture([&] {
        return acir::detail::verifyTransactionMaterialization(
            *prepared, *candidate, initialProof);
      }))) << diagnostics;
      auto *last = record.generated.front();
      auto merge = mlir::dyn_cast<ValueMergeOp>(last);
      ASSERT_TRUE(merge);
      ASSERT_EQ(merge.getPaths(),
                builder.getArrayAttr({builder.getArrayAttr({})}));
      ASSERT_EQ(merge->getAttr("operandSegmentSizes"),
                builder.getDenseI32ArrayAttr({1, 1, 1}));
      ASSERT_EQ(merge.getGuards().size(), 1u);
      ASSERT_EQ(merge.getValues().size(), 1u);
      ASSERT_TRUE(merge.getEn().use_empty());
      ASSERT_EQ(merge.getEn().getType(), merge.getGuards()[0].getType());
      ASSERT_NE(merge.getBase(), merge.getValues()[0]);
      if (mutation == 0) {
        unchanged(*file, before);
        continue;
      }
      if (mutation == 1)
        last->setAttr("forged", builder.getUnitAttr());
      if (mutation == 2) {
        auto base = last->getOperand(0);
        last->setOperand(0, last->getOperand(2));
        last->setOperand(2, base);
      }
      if (mutation == 3)
        record.generated.push_back(
            last); // duplicate identity, not a second proof
      if (mutation == 4)
        record.generated.clear();
      if (mutation == 5)
        record.value = merge.getEn(); // wrong result ordinal
      if (mutation == 6)
        record.value =
            split.getResult(0); // original-package SSA is not candidate SSA
      if (mutation == 7) {
        builder.setInsertionPointAfter(last);
        mlir::OperationState dead(last->getLoc(),
                                  BitsBinaryOp::getOperationName());
        dead.addOperands({merge.getEn(), merge.getEn()});
        dead.addTypes(merge.getEn().getType());
        dead.addAttribute("opcode", builder.getStringAttr("and"));
        builder.create(dead); // even a dead en consumer violates the contract
      }
      if (mutation == 8)
        last->erase(); // stale locators must be rejected before dereference
      if (mutation == 9)
        last->setOperand(1, transaction.rule.getBody().front().getArgument(1));
      if (mutation == 10)
        last->setAttr("paths", builder.getArrayAttr({builder.getArrayAttr(
                                   {builder.getStringAttr("left")})}));
      if (mutation == 11)
        last->setLoc(mlir::FileLineColLoc::get(&context, "forged.py", 9, 1));
      if (mutation == 12)
        last->setAttr("operandSegmentSizes",
                      builder.getDenseI32ArrayAttr({1, 0, 2}));
      if (mutation == 13)
        last->getResult(0).setType(merge.getEn().getType());
      if (mutation == 14)
        last->setOperand(2, last->getOperand(0));
      if (mutation == 15) {
        auto *extra = last->clone();
        last->getBlock()->getOperations().insert(last->getIterator(), extra);
        record.generated.push_back(extra);
      }
      if (mutation == 16)
        last->moveBefore(transaction.rule);
      auto candidateBefore = identities(*candidate->module);
      auto candidateText = print(candidate->module->getOperation());
      detail::SourceCheckBudget proofBudget(limits);
      EXPECT_TRUE(mlir::failed(capture([&] {
        return acir::detail::verifyTransactionMaterialization(
            *prepared, *candidate, proofBudget);
      })));
      EXPECT_FALSE(diagnostics.empty());
      unchanged(*file, before);
      unchanged(*candidate->module, candidateBefore);
      EXPECT_EQ(candidateText, print(candidate->module->getOperation()));
    }
  }
}

TEST_F(TransactionLoweringTest,
       SourceReplayBranchShapesRetainIndependentProofAndFailureAtomicity) {
  // Scalar and Enum roots, broad repeated nominal siblings, Table-of-Struct,
  // and a deeper legal chain exercise different reconstruction stack shapes.
  for (unsigned shape = 0; shape < 5; ++shape) {
    SCOPED_TRACE(shape);
    auto file = parse();
    ASSERT_TRUE(file);
    auto split = shape == 0   ? addBranchScenario(*file)
                 : shape == 1 ? addNominalBranch(*file, true)
                              : addBranchShape(*file, shape == 2 ? 16 : 2,
                                               shape == 4 ? 12 : 0, shape == 3);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceTransactionStateAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    HardwareSourceCheckLimits limits;
    detail::SourceCheckBudget preparationBudget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*file, analysis,
                                                      preparationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
    detail::SourceCheckBudget materializationBudget(limits);
    auto candidate = capture([&] {
      return acir::detail::materializeTransactionLowering(
          *prepared, materializationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(candidate)) << diagnostics;
    auto selected =
        llvm::find_if(prepared->transactions, [&](const auto &entry) {
          return entry.plan->transaction ==
                 split->getParentOfType<SourceTransactionOp>();
        });
    ASSERT_NE(selected, prepared->transactions.end());
    auto &transaction = candidate->transactions[std::distance(
        prepared->transactions.begin(), selected)];
    ASSERT_EQ(transaction.branchResults.size(), 1u);
    auto &record = transaction.branchResults[0];
    ASSERT_EQ(record.value.getType(), split.getResult(0).getType());
    auto originalBefore = identities(*file);
    auto originalText = print(file->getOperation());
    detail::SourceCheckBudget initialProof(limits);
    ASSERT_TRUE(mlir::succeeded(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *candidate, initialProof);
    }))) << diagnostics;
    auto selectedLeaf = llvm::find_if(record.generated, [](auto *op) {
      return mlir::isa<BitsSelectOp, ValueMergeOp>(op);
    });
    ASSERT_NE(selectedLeaf, record.generated.end());
    auto *leaf = *selectedLeaf;
    unsigned trueIndex = mlir::isa<ValueMergeOp>(leaf) ? 2 : 1;
    unsigned falseIndex = mlir::isa<ValueMergeOp>(leaf) ? 0 : 2;
    if (shape != 0) {
      ASSERT_EQ(record.generated.size(), 1u);
      ASSERT_TRUE(mlir::isa<ValueMergeOp>(leaf));
    }
    auto trueOperand = leaf->getOperand(trueIndex);
    ASSERT_NE(trueOperand, leaf->getOperand(falseIndex));
    // Each mutation starts from the same successfully authenticated candidate.
    for (unsigned mutation = 0; mutation < 3; ++mutation) {
      SCOPED_TRACE(mutation);
      auto inventory = record.generated;
      if (mutation == 0)
        leaf->setAttr("forged", builder.getUnitAttr());
      else if (mutation == 1)
        leaf->setOperand(trueIndex, leaf->getOperand(falseIndex));
      else
        record.generated.pop_back();
      auto candidateBefore = identities(*candidate->module);
      auto candidateText = print(candidate->module->getOperation());
      detail::SourceCheckBudget proofBudget(limits);
      EXPECT_TRUE(mlir::failed(capture([&] {
        return acir::detail::verifyTransactionMaterialization(
            *prepared, *candidate, proofBudget);
      })));
      EXPECT_FALSE(diagnostics.empty());
      unchanged(*file, originalBefore);
      EXPECT_EQ(originalText, print(file->getOperation()));
      unchanged(*candidate->module, candidateBefore);
      EXPECT_EQ(candidateText, print(candidate->module->getOperation()));
      leaf->removeAttr("forged");
      leaf->setOperand(trueIndex, trueOperand);
      record.generated = inventory;
    }
    detail::SourceCheckBudget restoredProof(limits);
    EXPECT_TRUE(mlir::succeeded(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *candidate, restoredProof);
    }))) << diagnostics;
  }
}

TEST_F(TransactionLoweringTest,
       TableContainingNominalsKeepExpandedBranchProofAndRollback) {
  // Both shapes use the existing rank-one Table-field admission. In particular,
  // a closed outer shape must not hide a Table below its Struct element.
  for (bool outerTable : {false, true}) {
    SCOPED_TRACE(outerTable);
    auto file = parse();
    ASSERT_TRUE(file);
    auto split = addBranchShape(*file, 2, 0, outerTable);
    auto *inner = mlir::SymbolTable::lookupSymbolIn(*file, "Inner");
    auto fields = inner->getAttrOfType<mlir::ArrayAttr>("fields");
    auto dataField = mlir::cast<mlir::DictionaryAttr>(fields[0]);
    auto byte = dataField.getAs<mlir::TypeAttr>("type").getValue();
    auto shape = builder.getArrayAttr({mlir::cast<BitsType>(byte).getWidth()});
    auto table = TableType::get(&context, shape, byte);
    mlir::NamedAttrList changed(dataField);
    changed.set("type", mlir::TypeAttr::get(table));
    inner->setAttr("fields", builder.getArrayAttr(
        {changed.getDictionary(&context), fields[1]}));
    llvm::SmallVector<StructCreateOp> creates;
    file->walk([&](StructCreateOp create) {
      if (create.getResult().getType() ==
          StructType::get(&context, builder.getStringAttr("Inner")))
        creates.push_back(create);
    });
    ASSERT_FALSE(creates.empty());
    for (auto create : creates) {
      builder.setInsertionPoint(create);
      mlir::OperationState splat(create.getLoc(), TableSplatOp::getOperationName());
      splat.addOperands(create->getOperand(0));
      splat.addTypes(table);
      splat.addAttribute("shape", shape);
      create->setOperand(0, builder.create(splat)->getResult(0));
    }
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    auto sourceBefore = identities(*file);
    auto sourceText = print(file->getOperation());
    SourceTransactionStateAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    HardwareSourceCheckLimits limits;
    detail::SourceCheckBudget preparationBudget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*file, analysis,
                                                     preparationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
    detail::SourceCheckBudget materializationBudget(limits);
    auto candidate = capture([&] {
      return acir::detail::materializeTransactionLowering(
          *prepared, materializationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(candidate)) << diagnostics;
    auto selected = llvm::find_if(prepared->transactions, [&](const auto &entry) {
      return entry.plan->transaction ==
             split->getParentOfType<SourceTransactionOp>();
    });
    ASSERT_NE(selected, prepared->transactions.end());
    auto &transaction = candidate->transactions[
        std::distance(prepared->transactions.begin(), selected)];
    ASSERT_EQ(transaction.branchResults.size(), 1u);
    auto &record = transaction.branchResults[0];
    EXPECT_GT(record.generated.size(), 1u);
    EXPECT_TRUE(llvm::none_of(record.generated, [](auto *operation) {
      return mlir::isa<ValueMergeOp>(operation);
    }));
    EXPECT_TRUE(llvm::any_of(record.generated, [](auto *operation) {
      return mlir::isa<BitsSelectOp>(operation);
    }));
    detail::SourceCheckBudget proofBudget(limits);
    ASSERT_TRUE(mlir::succeeded(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *candidate, proofBudget);
    }))) << diagnostics;
    record.generated.front()->setAttr("forged", builder.getUnitAttr());
    auto candidateBefore = identities(*candidate->module);
    auto candidateText = print(candidate->module->getOperation());
    detail::SourceCheckBudget forgedBudget(limits);
    EXPECT_TRUE(mlir::failed(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *candidate, forgedBudget);
    })));
    unchanged(*file, sourceBefore);
    EXPECT_EQ(sourceText, print(file->getOperation()));
    unchanged(*candidate->module, candidateBefore);
    EXPECT_EQ(candidateText, print(candidate->module->getOperation()));
  }
}

TEST_F(TransactionLoweringTest,
       SelectedPartialMapMergeAuthenticatesGeneratedCompanionGet) {
  for (unsigned mutation = 0; mutation < 10; ++mutation) {
    SCOPED_TRACE(mutation);
    const bool uniformZero = mutation == 6 || mutation == 7;
    const bool sampledTable = mutation >= 8;
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto &body = definition.getBody().front();
    auto run = named<SourceTransactionOp>(*file, "run");
    auto bit = body.getArgument(1).getType();
    auto byte = body.getArgument(0).getType();
    auto exemplar = *body.getOps<BitsConstantOp>().begin();
    auto literal = [&](unsigned value) {
      mlir::NamedAttrList tree(exemplar.getValue().getTree());
      tree.set("value", builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("integer")),
          builder.getNamedAttr("value", MathIntAttr::get(&context,
              llvm::APSInt(llvm::APInt(64, value), true)))}));
      return StaticExprAttr::get(&context, tree.getDictionary(&context));
    };
    auto make = [&](llvm::StringRef name, mlir::ValueRange inputs,
                    mlir::TypeRange outputs,
                    llvm::ArrayRef<mlir::NamedAttribute> attrs = {}) {
      mlir::OperationState state(run.getLoc(), name);
      state.addOperands(inputs);
      state.addTypes(outputs);
      state.addAttributes(attrs);
      return builder.create(state);
    };
    auto number = [&](unsigned value, mlir::Type type) {
      return make(BitsConstantOp::getOperationName(), {}, {type},
          {builder.getNamedAttr("value", literal(value))})->getResult(0);
    };
    // Keep the body effect path proven true. The separate runtime guard makes
    // run's returned payload partial through normal compiler-owned admission.
    builder.setInsertionPoint(run);
    mlir::Value sourceGuard = uniformZero ? mlir::Value(number(0, bit))
                                        : mlir::Value(body.getArgument(2));
    run->insertOperands(run->getNumOperands(), sourceGuard);
    for (auto &region : run->getRegions()) {
      auto gate = region.front().addArgument(bit, run.getLoc());
      if (&region == &run.getGuard())
        region.front().getTerminator()->setOperand(0, gate);
    }
    definition->setAttr("ac.result_availability",
                        builder.getStrArrayAttr({"compiler", "compiler",
                                                 "compiler", "compiler"}));
    builder.setInsertionPoint(body.getTerminator());
    auto shape = builder.getArrayAttr({literal(2)});
    auto table = TableType::get(&context, shape, byte);
    auto seven = number(7, byte), nine = number(9, byte);
    mlir::Value base = make(TableCreateOp::getOperationName(), {seven, nine},
                            {table})->getResult(0);
    mlir::OperationState mapState(run.getLoc(), TableMapOp::getOperationName());
    mapState.addOperands({base, run->getResult(0), body.getArgument(2)});
    mapState.addTypes(table);
    mapState.addAttribute("shape", shape);
    mapState.addAttribute("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 2}));
    mapState.addRegion();
    auto map = mlir::cast<TableMapOp>(builder.create(mapState));
    auto *row = new mlir::Block;
    map.getBody().push_back(row);
    for (auto type : {bit, byte, byte, bit})
      row->addArgument(type, run.getLoc());
    builder.setInsertionPointToEnd(row);
    auto zero = number(0, bit);
    auto firstRow = make(BitsCompareOp::getOperationName(),
        {row->getArgument(0), zero}, {bit},
        {builder.getNamedAttr("predicate", builder.getStringAttr("eq"))})->getResult(0);
    auto selected = make(BitsBinaryOp::getOperationName(),
        {firstRow, row->getArgument(3)}, {bit},
        {builder.getNamedAttr("opcode", builder.getStringAttr("and"))})->getResult(0);
    auto value = make(BitsSelectOp::getOperationName(),
        {selected, row->getArgument(2), row->getArgument(1)}, {byte})->getResult(0);
    yield(value, run.getLoc());
    builder.setInsertionPoint(body.getTerminator());
    mlir::Value candidateValue = map.getResult(0);
    mlir::Type mergedType = table;
    mlir::Value mergeGuard = exemplar.getResult();
    if (uniformZero) {
      // A real lowered Rule with known-false guard supplies U=0. Wrap it in
      // nested Tables so confusing proven zero with not-proven cannot fall
      // back to the Table-free leaf route and silently pass this control.
      auto nestedRecord = StructType::get(&context, builder.getStringAttr("UniformZero"));
      {
        mlir::OpBuilder::InsertionGuard insertion(builder);
        builder.setInsertionPointToStart(file->getBody());
        make(StructOp::getOperationName(), {}, {},
            {builder.getNamedAttr("sym_name", builder.getStringAttr("UniformZero")),
             builder.getNamedAttr("fields", builder.getArrayAttr({builder.getDictionaryAttr(
                 {builder.getNamedAttr("name", builder.getStringAttr("t")),
                  builder.getNamedAttr("type", mlir::TypeAttr::get(table))})}))});
      }
      auto inside = make(TableSplatOp::getOperationName(), {run->getResult(0)},
          {table}, {builder.getNamedAttr("shape", shape)})->getResult(0);
      auto record = make(StructCreateOp::getOperationName(), {inside},
                         {nestedRecord})->getResult(0);
      mergedType = TableType::get(&context, shape, nestedRecord);
      candidateValue = make(TableSplatOp::getOperationName(), {record},
          {mergedType}, {builder.getNamedAttr("shape", shape)})->getResult(0);
      base = candidateValue;
      mergeGuard = body.getArgument(2);
    }
    if (sampledTable) {
      // Return the complete total Table from a genuinely guarded transaction.
      // Its scalar fire applies to every row even though its body is total.
      mlir::OperationState state(run.getLoc(), SourceTransactionOp::getOperationName());
      state.addOperands({body.getArgument(1), body.getArgument(2),
                         exemplar.getResult(), body.getArgument(2)});
      state.addTypes(table);
      state.addAttribute("name", builder.getStringAttr("tableProducer"));
      state.addAttribute("declaration", occurrence("Owner", 28));
      state.addAttribute("occurrence", occurrence("Owner", 29));
      state.addAttribute("ac.guard_location", span(28));
      state.addRegion();
      state.addRegion();
      auto producer = mlir::cast<SourceTransactionOp>(builder.create(state));
      for (unsigned region = 0; region < 2; ++region) {
        auto *block = new mlir::Block;
        producer->getRegion(region).push_back(block);
        block->addArgument(bit, run.getLoc());
        auto gate = block->addArgument(bit, run.getLoc());
        builder.setInsertionPointToEnd(block);
        if (region == 0) {
          yield(gate, run.getLoc());
        } else {
          auto total = make(TableCreateOp::getOperationName(),
                            {number(7, byte), number(9, byte)}, {table});
          yield(total->getResults(), run.getLoc());
        }
      }
      builder.setInsertionPoint(body.getTerminator());
      // This base is deliberately not row-uniform. A whole-arm fold or loss
      // of the sampled producer fire must not prove the selected row formula.
      base = make(TableCreateOp::getOperationName(), {run->getResult(0), nine},
                  {table})->getResult(0);
      candidateValue = producer.getResult(0);
      mergeGuard = body.getArgument(2);
    }
    auto merged = make(ValueMergeOp::getOperationName(),
        {base, mergeGuard, candidateValue}, {mergedType, bit},
        {builder.getNamedAttr("paths", builder.getArrayAttr({builder.getArrayAttr({})})),
         builder.getNamedAttr("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 1, 1}))});
    auto readType = mlir::cast<TableType>(mergedType).getElementType();
    auto read = make(TableGetOp::getOperationName(),
        {merged->getResult(0), number(0, bit)}, {readType, bit});
    mlir::Value result = read->getResult(0);
    if (uniformZero) {
      auto inner = make(StructGetOp::getOperationName(), {result}, {table},
          {builder.getNamedAttr("field", builder.getStringAttr("t"))})->getResult(0);
      result = make(TableGetOp::getOperationName(), {inner, number(0, bit)},
                    {byte, bit})->getResult(0);
    }
    if (sampledTable) {
      result = merged->getResult(0);
      llvm::SmallVector<mlir::Type> results(definition.getFunctionType().getResults());
      results[3] = table;
      definition->setAttr("function_type", mlir::TypeAttr::get(builder.getFunctionType(
          definition.getFunctionType().getInputs(), results)));
    }
    body.getTerminator()->setOperand(3, result);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    auto sourceBefore = identities(*file);
    SourceTransactionStateAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid());
    HardwareSourceCheckLimits limits;
    detail::SourceCheckBudget prepareBudget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*file, analysis, prepareBudget);
    });
    ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
    ASSERT_TRUE(prepared->valueAvailability);
    detail::SourceCheckBudget materializeBudget(limits);
    auto candidate = capture([&] {
      return acir::detail::materializeTransactionLowering(*prepared, materializeBudget);
    });
    ASSERT_TRUE(mlir::succeeded(candidate)) << diagnostics;
    TableGetOp companion;
    for (auto *operation : candidate->availabilityOperations)
      if (auto get = mlir::dyn_cast<TableGetOp>(operation);
          get && get.getInput().getDefiningOp<TableMapOp>()) {
        ASSERT_FALSE(companion);
        companion = get;
      }
    if (!uniformZero && !sampledTable) {
      ASSERT_TRUE(companion);
      EXPECT_TRUE(companion.getInRange().use_empty());
    }
    detail::SourceCheckBudget initialProof(limits);
    ASSERT_TRUE(mlir::succeeded(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *candidate, initialProof);
    }))) << diagnostics;
    if (uniformZero) {
      auto candidateOwner = owner(*candidate->module);
      auto layout = getModulePortLayout(candidateOwner);
      ASSERT_TRUE(mlir::succeeded(layout));
      ASSERT_GE(layout->outputValidity[3], 0);
      auto availableValue = candidateOwner.getBody().front().getTerminator()
          ->getOperand(layout->outputValidity[3]);
      auto scope = std::make_shared<EvaluationScope>();
      scope->definition = candidateOwner;
      EXPECT_EQ(evaluate(*candidate->module, availableValue, scope), 0u);
    }
    if (mutation && mutation != 6 && mutation != 8) {
      auto candidateOwner = owner(*candidate->module);
      if (mutation == 9) {
        auto producer = named<RuleOp>(*candidate->module, "tableProducer");
        auto admission = producer->getAttrOfType<mlir::DictionaryAttr>("ac.sampled_admission");
        ASSERT_TRUE(admission);
        auto fireIndex = admission.getAs<mlir::IntegerAttr>("fire_result");
        ASSERT_TRUE(fireIndex);
        auto fire = producer->getResult(fireIndex.getInt());
        builder.setInsertionPoint(producer);
        auto total = number(1, bit);
        unsigned replaced = 0;
        for (auto *operation : candidate->availabilityOperations)
          for (auto &operand : operation->getOpOperands())
            if (operand.get() == fire) {
              operand.set(total);
              ++replaced;
            }
        ASSERT_GT(replaced, 0u);
        EXPECT_TRUE(mlir::failed(capture([&] {
          return detail::verifyHardwareValueAvailability(HardwareAnalysis(*candidate->module));
        })));
        EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                  std::string::npos) << diagnostics;
      } else if (mutation == 7) {
        auto layout = getModulePortLayout(candidateOwner);
        ASSERT_TRUE(mlir::succeeded(layout));
        auto *output = candidateOwner.getBody().front().getTerminator();
        builder.setInsertionPoint(output);
        output->setOperand(layout->outputValidity[3], number(1, bit));
      } else if (mutation == 1 || mutation == 2) {
        builder.setInsertionPoint(companion);
        auto indexType = mutation == 1 ? bit : mlir::Type(BitsType::get(&context, literal(2)));
        companion->setOperand(1, number(mutation, indexType));
      } else if (mutation == 3) {
        companion->setOperand(1, candidateOwner.getBody().front().getArgument(2));
      } else {
        builder.setInsertionPointAfter(companion);
        auto escaped = mutation == 4 ? companion.getInRange() : companion.getValue();
        auto data = make(BitsResizeOp::getOperationName(), {escaped}, {byte},
            {builder.getNamedAttr("mode", builder.getStringAttr("zext"))})->getResult(0);
        candidateOwner.getBody().front().getTerminator()->setOperand(0, data);
      }
      auto candidateBefore = identities(*candidate->module);
      auto candidateText = print(candidate->module->getOperation());
      detail::SourceCheckBudget proof(limits);
      EXPECT_TRUE(mlir::failed(capture([&] {
        return acir::detail::verifyTransactionMaterialization(*prepared, *candidate, proof);
      })));
      unchanged(*candidate->module, candidateBefore);
      EXPECT_EQ(candidateText, print(candidate->module->getOperation()));
    }
    unchanged(*file, sourceBefore);
  }
}

TEST_F(TransactionLoweringTest,
       SourceReplayOverDeepNominalBranchRejectsBeforeReplacement) {
  auto file = parse();
  ASSERT_TRUE(file);
  addBranchShape(*file, 2, 129, false);
  // Metadata admission rejects before localBranchProof's private stack guard.
  // This asserts that earlier boundary and preserves the original package.
  rejects(*file, "source branch result metadata depth exceeded");
  RecordProperty("rejection_diagnostic", diagnostics);
}

TEST_F(TransactionLoweringTest,
       NominalClosureRejectsCyclesUnresolvedAndTableFieldsBeforeReplacement) {
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    addNominalBranch(*file);
    auto declaration = mlir::SymbolTable::lookupSymbolIn(*file, "Inner");
    ASSERT_NE(declaration, nullptr);
    auto fields = declaration->getAttrOfType<mlir::ArrayAttr>("fields");
    auto row = mlir::cast<mlir::DictionaryAttr>(fields[0]);
    mlir::NamedAttrList changed(row);
    mlir::Type invalid;
    if (mutation == 0)
      invalid = StructType::get(&context, builder.getStringAttr("Inner"));
    if (mutation == 1)
      invalid = StructType::get(&context, builder.getStringAttr("Packet"));
    if (mutation == 2)
      invalid = StructType::get(&context, builder.getStringAttr("Absent"));
    if (mutation == 3) {
      auto constant =
          *owner(*file).getBody().front().getOps<BitsConstantOp>().begin();
      invalid =
          TableType::get(&context, builder.getArrayAttr({constant.getValue()}),
                         row.getAs<mlir::TypeAttr>("type").getValue());
    }
    if (mutation == 4) {
      auto enumeration = mlir::SymbolTable::lookupSymbolIn(*file, "Tag");
      enumeration->setAttr("width",
                           mlir::parseAttribute("#ac.math_int<0>", &context));
    } else {
      changed.set("type", mlir::TypeAttr::get(invalid));
      declaration->setAttr(
          "fields",
          builder.getArrayAttr({changed.getDictionary(&context), fields[1]}));
    }
    rejects(*file);
  }
}

TEST_F(TransactionLoweringTest,
       NominalMaterializationTinyBudgetPreservesOriginalAndPreparation) {
  auto file = parse();
  ASSERT_TRUE(file);
  addNominalBranch(*file);
  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  auto before = identities(*file);
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  HardwareSourceCheckLimits tiny;
  tiny.maxWorkUnits = 1;
  detail::SourceCheckBudget tinyBudget(tiny);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared, tinyBudget);
  })));
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
  unchanged(*file, before);
}

TEST_F(TransactionLoweringTest,
       ConditionalWritePreservesOldCandidateAndRawFourStateIntent) {
  for (bool knownFalse : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto watch = named<SourceTransactionOp>(*file, "watch");
    mlir::Value gate = definition.getBody().front().getArgument(2);
    if (knownFalse)
      for (auto constant :
           definition.getBody().front().getOps<BitsConstantOp>()) {
        HardwareAnalysis hardware(*file);
        HardwareBindings bindings;
        bindings.owner = definition;
        auto value =
            hardware.evaluateStatic(constant.getValue(), bindings, constant);
        if (mlir::succeeded(value) && mlir::isa<MathIntAttr>(*value) &&
            mlir::cast<MathIntAttr>(*value).getCanonicalValue() == "0" &&
            constant.getResult().getType() == gate.getType())
          gate = constant.getResult();
      }
    watch->insertOperands(watch->getNumOperands(), gate);
    for (auto &region : watch->getRegions()) {
      auto path = region.front().addArgument(gate.getType(), gate.getLoc());
      if (&region == &watch.getBody())
        first<SourceStateWriteOp>(region)->setOperand(0, path);
    }
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    auto rule = named<RuleOp>(*file, "watch");
    auto seen = leaf(*file, "seen"), count = leaf(*file, "count");
    auto selected = yielded(seen).getDefiningOp<BitsSelectOp>();
    auto effective = yielded(seen, 2).getDefiningOp<BitsBinaryOp>();
    ASSERT_TRUE(selected);
    ASSERT_TRUE(effective);
    auto intent = effective->getOperand(1).getDefiningOp<BitsBinaryOp>();
    ASSERT_TRUE(intent);
    EXPECT_EQ(effective.getOpcode(), "and");
    EXPECT_EQ(intent.getOpcode(), "or");
    EXPECT_EQ(intent->getOperand(1), selected.getCondition());
    EXPECT_EQ(captureActual(rule, selected->getOperand(1)),
              count->getResult(0));
    EXPECT_EQ(captureActual(rule, selected->getOperand(2)), seen->getResult(0));
    ASSERT_TRUE(mlir::succeeded(capture([&] {
      return verifyHardwarePackage(*file);
    }))) << diagnostics;
  }
}

TEST_F(TransactionLoweringTest,
       IndependentReadonlyRuleAndUnwrittenOwnerKeepOldQ) {
  for (bool reverse : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    if (reverse)
      named<SourceTransactionOp>(*file, "watch")
          ->moveBefore(named<SourceTransactionOp>(*file, "run"));
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    auto watch = named<RuleOp>(*file, "watch");
    auto count = leaf(*file, "count"), seen = leaf(*file, "seen"),
         hold = leaf(*file, "hold");
    auto run = named<RuleOp>(*file, "run");
    EXPECT_EQ(captureActual(watch, yielded(seen)), count->getResult(0));
    EXPECT_EQ(hold->getOperand(3), hold->getResult(0));
    auto heldEnable = hold->getOperand(2).getDefiningOp<BitsConstantOp>();
    ASSERT_TRUE(heldEnable);
    auto admission =
        run->getAttrOfType<mlir::DictionaryAttr>("ac.sampled_admission");
    ASSERT_TRUE(admission);
    auto fire = admission.getAs<mlir::IntegerAttr>("fire_result");
    ASSERT_TRUE(fire);
    ASSERT_GE(fire.getInt(), 0);
    ASSERT_LT(static_cast<uint64_t>(fire.getInt()), run->getNumResults());
    auto effective =
        mlir::dyn_cast<BitsBinaryOp>(yielded(count, 2).getDefiningOp());
    ASSERT_TRUE(effective);
    EXPECT_EQ(effective.getOpcode(), "and");
    EXPECT_EQ(effective->getOperand(0),
              run.getBody().front().getTerminator()->getOperand(fire.getInt()));
    EXPECT_TRUE(run->getResult(fire.getInt()).use_empty());
    HardwareAnalysis hardware(*file);
    HardwareBindings bindings;
    bindings.owner = owner(*file);
    auto held = capture([&] {
      return hardware.evaluateStatic(heldEnable.getValue(), bindings,
                                     heldEnable);
    });
    ASSERT_TRUE(mlir::succeeded(held));
    auto isTrue = [](mlir::Attribute value) {
      if (auto boolean = mlir::dyn_cast<mlir::BoolAttr>(value))
        return boolean.getValue();
      return mlir::cast<MathIntAttr>(value).getCanonicalValue() == "1";
    };
    EXPECT_FALSE(isTrue(*held));
    EXPECT_EQ(hold->getOperand(0),
              owner(*file).getBody().front().getArgument(1));
    EXPECT_EQ(hold->getOperand(1),
              owner(*file).getBody().front().getArgument(2));
  }
}

TEST_F(TransactionLoweringTest,
       TwoStructuralAllocationsRemainSeparateInstancesOfOneDefinition) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  mlir::OperationState state(definition.getLoc(), ModuleOp::getOperationName());
  state.addAttributes(definition->getAttrs());
  state.attributes.set("sym_name", builder.getStringAttr("Root"));
  state.addRegion();
  builder.setInsertionPointToEnd(file->getBody());
  auto root = mlir::cast<ModuleOp>(builder.create(state));
  auto *block = new mlir::Block;
  root.getBody().push_back(block);
  for (auto type : definition.getFunctionType().getInputs())
    block->addArgument(type, root.getLoc());
  std::vector<InstanceOp> instances;
  for (unsigned i = 0; i < 2; ++i) {
    builder.setInsertionPointToEnd(block);
    mlir::OperationState instance(root.getLoc(),
                                  InstanceOp::getOperationName());
    instance.addOperands(block->getArguments());
    instance.addTypes(definition.getFunctionType().getResults());
    instance.addAttribute("callee",
                          mlir::FlatSymbolRefAttr::get(&context, "Owner"));
    instance.addAttribute("instance_name",
                          builder.getStringAttr(i ? "right" : "left"));
    instance.addAttribute("parameters", builder.getArrayAttr({}));
    instance.addAttribute("type_arguments", builder.getArrayAttr({}));
    instance.addAttribute("occurrence", occurrence("Root", i));
    instances.push_back(mlir::cast<InstanceOp>(builder.create(instance)));
  }
  builder.setInsertionPointToEnd(block);
  mlir::OperationState yield(root.getLoc(), YieldOp::getOperationName());
  yield.addOperands(instances[0]->getResults());
  builder.create(yield);
  auto system = *file->getOps<SystemOp>().begin();
  mlir::NamedAttrList entry(system.getEntry());
  entry.set("callee", mlir::FlatSymbolRefAttr::get(&context, "Root"));
  system->setAttr("entry", entry.getDictionary(&context));
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  auto loweredRoot =
      mlir::cast<ModuleOp>(mlir::SymbolTable::lookupSymbolIn(*file, "Root"));
  auto children = loweredRoot.getBody().front().getOps<InstanceOp>();
  ASSERT_EQ(std::distance(children.begin(), children.end()), 2);
  auto left = *children.begin(), right = *std::next(children.begin());
  EXPECT_EQ(left.getCallee(), right.getCallee());
  EXPECT_NE(left.getOccurrence(), right.getOccurrence());
  EXPECT_NE(left->getResult(0), right->getResult(0));
  EXPECT_EQ(
      std::distance(owner(*file).getBody().front().getOps<InstanceOp>().begin(),
                    owner(*file).getBody().front().getOps<InstanceOp>().end()),
      4);
}

TEST_F(TransactionLoweringTest,
       NestedPureRegionsKeepLocalArgumentsInsideIsolatedRule) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto bump = named<SourceMethodOp>(*file, "bump");
  auto &body = bump.getBody().front();
  auto next = *std::next(body.getOps<BitsBinaryOp>().begin());
  auto one = (*owner(*file).getBody().front().getOps<BitsConstantOp>().begin())
                 .getValue();
  auto shape = builder.getArrayAttr({one});
  auto table = TableType::get(&context, shape, next.getResult().getType());
  builder.setInsertionPoint(next->getNextNode());
  mlir::OperationState splat(next.getLoc(), TableSplatOp::getOperationName());
  splat.addOperands(next.getResult());
  splat.addTypes(table);
  splat.addAttribute("shape", shape);
  auto *input = builder.create(splat);
  mlir::OperationState map(next.getLoc(), TableMapOp::getOperationName());
  map.addOperands(input->getResults());
  map.addTypes(table);
  map.addAttribute("shape", shape);
  map.addAttribute("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 0}));
  map.addRegion();
  auto *mapped = builder.create(map);
  auto *local = new mlir::Block;
  mapped->getRegion(0).push_back(local);
  local->addArgument(BitsType::get(&context, one), mapped->getLoc());
  auto element =
      local->addArgument(next.getResult().getType(), mapped->getLoc());
  builder.setInsertionPointToEnd(local);
  mlir::OperationState add(mapped->getLoc(), BitsBinaryOp::getOperationName());
  add.addOperands({element, element});
  add.addTypes(element.getType());
  add.addAttribute("opcode", builder.getStringAttr("add"));
  auto *twice = builder.create(add);
  mlir::OperationState yield(mapped->getLoc(), YieldOp::getOperationName());
  yield.addOperands(twice->getResults());
  builder.create(yield);
  // The unused map still requires valid complete region/SSA cloning.
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  unsigned maps = 0;
  named<RuleOp>(*file, "run")->walk([&](TableMapOp op) {
    ++maps;
    auto &block = op->getRegion(0).front();
    auto computation = *block.getOps<BitsBinaryOp>().begin();
    EXPECT_EQ(computation->getOperand(0), block.getArgument(1));
    EXPECT_EQ(computation->getOperand(1), block.getArgument(1));
    EXPECT_EQ(computation->getParentOp(), op.getOperation());
  });
  EXPECT_EQ(maps, 2u);
  EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
}

TEST_F(TransactionLoweringTest,
       PostPreparationNestedBodyEffectsFailCloneAndSourceAuthentication) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto bump = named<SourceMethodOp>(*file, "bump");
  auto &body = bump.getBody().front();
  auto next = *std::next(body.getOps<BitsBinaryOp>().begin());
  auto one = (*owner(*file).getBody().front().getOps<BitsConstantOp>().begin())
                 .getValue();
  auto shape = builder.getArrayAttr({one});
  auto table = TableType::get(&context, shape, next.getResult().getType());
  builder.setInsertionPoint(next->getNextNode());
  mlir::OperationState splat(next.getLoc(), TableSplatOp::getOperationName());
  splat.addOperands(next.getResult());
  splat.addTypes(table);
  splat.addAttribute("shape", shape);
  auto *input = builder.create(splat);
  mlir::OperationState map(next.getLoc(), TableMapOp::getOperationName());
  map.addOperands(input->getResults());
  map.addTypes(table);
  map.addAttribute("shape", shape);
  map.addAttribute("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 0}));
  map.addRegion();
  auto *mapped = builder.create(map);
  auto *local = new mlir::Block;
  mapped->getRegion(0).push_back(local);
  auto ordinal =
      local->addArgument(BitsType::get(&context, one), mapped->getLoc());
  auto element =
      local->addArgument(next.getResult().getType(), mapped->getLoc());
  builder.setInsertionPointToEnd(local);
  mlir::OperationState yield(mapped->getLoc(), YieldOp::getOperationName());
  yield.addOperands(element);
  builder.create(yield);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  detail::SourceCheckBudget initialMaterializationBudget(limits);
  auto materialized = capture([&] {
    return acir::detail::materializeTransactionLowering(
        *prepared, initialMaterializationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;

  builder.setInsertionPoint(local->getTerminator());
  mlir::OperationState observation(mapped->getLoc(),
                                   SourceObserveOp::getOperationName());
  observation.addOperands({ordinal, element});
  observation.addAttribute("kind", builder.getStringAttr("report"));
  observation.addAttribute("spec",
                           builder.getDictionaryAttr({builder.getNamedAttr(
                               "name", builder.getStringAttr("nested"))}));
  observation.addAttribute("occurrence", occurrence("Owner", 30));
  observation.addAttribute("ac.location", bump->getAttr("ac.guard_location"));
  builder.create(observation);
  auto before = identities(*file);
  auto text = print(file->getOperation());

  detail::SourceCheckBudget cloneBudget(limits);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared, cloneBudget);
  })));
  EXPECT_NE(diagnostics.find("sampled effect lowering"), std::string::npos)
      << diagnostics;
  detail::SourceCheckBudget proofBudget(limits);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, proofBudget);
  })));
  EXPECT_NE(diagnostics.find("sampled effect lowering"), std::string::npos)
      << diagnostics;
  unchanged(*file, before);
  EXPECT_EQ(text, print(file->getOperation()));
}

TEST_F(TransactionLoweringTest,
       IdentityWritesFromDistinctOuterRulesConflictBeforeElimination) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto watch = named<SourceTransactionOp>(*file, "watch");
  auto write = first<SourceStateWriteOp>(watch.getBody());
  write->setAttr("state", builder.getStringAttr("count"));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  rejects(*file);
}

TEST_F(TransactionLoweringTest,
       GuardsEffectsDomainsAndEnvelopeRejectWithoutMutation) {
  for (unsigned mutation = 0; mutation < 7; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto run = named<SourceTransactionOp>(*file, "run");
    auto bump = named<SourceMethodOp>(*file, "bump");
    auto no = *std::next(
        definition.getBody().front().getOps<BitsConstantOp>().begin());
    if (mutation == 0)
      bump->setOperand(0, no.getResult());
    if (mutation == 1)
      bump->setOperand(0, definition.getBody().front().getArgument(2));
    if (mutation == 2) {
      auto call = first<SourceMethodCallOp>(run.getBody());
      auto *literal = no->clone();
      run.getBody().front().getOperations().insert(
          run.getBody().front().begin(), literal);
      call->setOperand(0, literal->getResult(0));
    }
    if (mutation == 3)
      first<SourceStateOp>(definition.getBody())->setOperand(1, no.getResult());
    if (mutation == 4) {
      (*file->getOps<ModuleImportOp>().begin())->erase();
      // Logical graph legality does not require a physical storage provider.
      ASSERT_TRUE(mlir::succeeded(capture([&] {
        mlir::PassManager route(&context);
        route.addPass(acir::createRouteTransactionsPass());
        return route.run(*file);
      }))) << diagnostics;
      ASSERT_TRUE(mlir::succeeded(capture([&] {
        return verifyRoutedPackage(*file);
      }))) << diagnostics;
      auto routedIdentities = identities(*file);
      auto routedText = print(file->getOperation());
      EXPECT_TRUE(mlir::failed(capture([&] {
        mlir::PassManager refine(&context);
        refine.addPass(acir::createLowerTransactionsPass());
        return refine.run(*file);
      })));
      EXPECT_NE(diagnostics.find("verified DFFE primitive"), std::string::npos)
          << diagnostics;
      unchanged(*file, routedIdentities);
      EXPECT_EQ(routedText, print(file->getOperation()));
      continue;
    }
    if (mutation == 5)
      (*file->getOps<SystemOp>().begin())->erase();
    if (mutation == 6)
      (*file)->removeAttr("ac.source_units");
    rejects(*file);
  }
  auto staged = parse();
  ASSERT_TRUE(staged);
  staged->getOperation()->setAttr("ac.stage", builder.getStringAttr("source"));
  auto before = identities(*staged);
  EXPECT_TRUE(mlir::failed(lower(*staged))) << diagnostics;
  unchanged(*staged, before);
}

TEST_F(TransactionLoweringTest,
       PlannedBodyEffectsMaterializeAndPassOriginalSourceAuthentication) {
  for (llvm::StringRef kind : {"expect", "observe"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto run = named<SourceTransactionOp>(*file, "run");
    auto &body = run.getBody().front();
    auto path = body.getArgument(0);
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState effect(
        run.getLoc(), kind == "expect" ? SourceExpectOp::getOperationName()
                                       : SourceObserveOp::getOperationName());
    if (kind == "expect") {
      effect.addOperands({path, path});
      effect.addAttribute("kind", builder.getStringAttr("assert"));
      effect.addAttribute("location", run->getAttr("ac.guard_location"));
    } else {
      effect.addOperands({path, body.getArgument(1)});
      effect.addAttribute("kind", builder.getStringAttr("report"));
      effect.addAttribute("spec",
                          builder.getDictionaryAttr({builder.getNamedAttr(
                              "name", builder.getStringAttr("candidate"))}));
      effect.addAttribute("ac.location", run->getAttr("ac.guard_location"));
    }
    effect.addAttribute("occurrence", occurrence("Owner", 30));
    builder.create(effect);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    SourceTransactionStateAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid()) << diagnostics;
    auto *plan = analysis.lookup(run);
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->bodyChecks.size(), kind == "expect" ? 1u : 0u);
    EXPECT_EQ(plan->observations.size(), kind == "observe" ? 1u : 0u);
    auto before = identities(*file);
    auto text = print(file->getOperation());
    HardwareSourceCheckLimits limits;
    detail::SourceCheckBudget preparationBudget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*file, analysis,
                                                      preparationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
    detail::SourceCheckBudget materializationBudget(limits);
    auto materialized = capture([&] {
      return acir::detail::materializeTransactionLowering(
          *prepared, materializationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
    detail::SourceCheckBudget proofBudget(limits);
    EXPECT_TRUE(mlir::succeeded(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *materialized, proofBudget);
    }))) << diagnostics;
    auto preparedIndex =
        llvm::find_if(prepared->transactions, [&](const auto &entry) {
          return entry.plan->transaction == run;
        });
    ASSERT_NE(preparedIndex, prepared->transactions.end());
    auto index = std::distance(prepared->transactions.begin(), preparedIndex);
    EXPECT_EQ(materialized->transactions[index].bodyChecks.size(),
              kind == "expect" ? 1u : 0u);
    EXPECT_EQ(materialized->transactions[index].observations.size(),
              kind == "observe" ? 1u : 0u);
    unchanged(*file, before);
    EXPECT_EQ(text, print(file->getOperation()));
    EXPECT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    unsigned remainingTransactions = 0;
    file->walk([&](SourceTransactionOp) { ++remainingTransactions; });
    EXPECT_EQ(remainingTransactions, 0u);
  }
}

TEST_F(TransactionLoweringTest,
       DirectLoweringSeamsAcceptAuthenticatedBodyEffectPlans) {
  auto effectFile = parse();
  ASSERT_TRUE(effectFile);
  auto effectRun = named<SourceTransactionOp>(*effectFile, "run");
  auto &effectBody = effectRun.getBody().front();
  builder.setInsertionPoint(effectBody.getTerminator());
  mlir::OperationState observe(effectRun.getLoc(),
                               SourceObserveOp::getOperationName());
  observe.addOperands({effectBody.getArgument(0), effectBody.getArgument(1)});
  observe.addAttribute("kind", builder.getStringAttr("report"));
  observe.addAttribute("spec",
                       builder.getDictionaryAttr({builder.getNamedAttr(
                           "name", builder.getStringAttr("candidate"))}));
  observe.addAttribute("occurrence", occurrence("Owner", 30));
  observe.addAttribute("ac.location", effectRun->getAttr("ac.guard_location"));
  builder.create(observe);
  mlir::OperationState check(effectRun.getLoc(),
                             SourceExpectOp::getOperationName());
  check.addOperands({effectBody.getArgument(0), effectBody.getArgument(0)});
  check.addAttribute("kind", builder.getStringAttr("assert"));
  check.addAttribute("occurrence", occurrence("Owner", 31));
  check.addAttribute("location", effectRun->getAttr("ac.guard_location"));
  builder.create(check);
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return mlir::verify(*effectFile);
  }))) << diagnostics;
  SourceTransactionStateAnalysis effectAnalysis(effectFile->getOperation());
  ASSERT_TRUE(effectAnalysis.isValid()) << diagnostics;
  auto *effectPlan = effectAnalysis.lookup(effectRun);
  ASSERT_NE(effectPlan, nullptr);
  ASSERT_EQ(effectPlan->observations.size(), 1u);
  ASSERT_EQ(effectPlan->bodyChecks.size(), 1u);

  HardwareSourceCheckLimits effectLimits;
  detail::SourceCheckBudget effectPrepareBudget(effectLimits);
  auto authenticPrepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*effectFile, effectAnalysis,
                                                    effectPrepareBudget);
  });
  ASSERT_TRUE(mlir::succeeded(authenticPrepared)) << diagnostics;
  detail::SourceCheckBudget effectMaterializeBudget(effectLimits);
  auto authenticMaterialized = capture([&] {
    return acir::detail::materializeTransactionLowering(
        *authenticPrepared, effectMaterializeBudget);
  });
  ASSERT_TRUE(mlir::succeeded(authenticMaterialized)) << diagnostics;
  detail::SourceCheckBudget effectProofBudget(effectLimits);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *authenticPrepared, *authenticMaterialized, effectProofBudget);
  }))) << diagnostics;
  auto effectEntry =
      llvm::find_if(authenticPrepared->transactions, [&](const auto &entry) {
        return entry.plan->transaction == effectRun;
      });
  ASSERT_NE(effectEntry, authenticPrepared->transactions.end());
  auto effectIndex =
      std::distance(authenticPrepared->transactions.begin(), effectEntry);
  EXPECT_EQ(authenticMaterialized->transactions[effectIndex].bodyChecks.size(),
            1u);
  EXPECT_EQ(
      authenticMaterialized->transactions[effectIndex].observations.size(), 1u);
}

TEST_F(TransactionLoweringTest,
       RetiredDiamondAfterRejectsWithoutPublishingHardware) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto a = addEmptyTransaction(*file, "after_a", 100);
  auto b = addEmptyTransaction(*file, "after_b", 102);
  auto c = addEmptyTransaction(*file, "after_c", 104);
  auto d = addEmptyTransaction(*file, "after_d", 106);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  b->setAttr("ac.after", builder.getArrayAttr({a.getOccurrence()}));
  c->setAttr("ac.after", builder.getArrayAttr({a.getOccurrence()}));
  d->setAttr("ac.after",
             builder.getArrayAttr({b.getOccurrence(), c.getOccurrence()}));
  auto before = identities(*file);
  auto text = print(file->getOperation());
  EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
  EXPECT_NE(diagnostics.find("after ordering is retired"), std::string::npos)
      << diagnostics;
  unchanged(*file, before);
  EXPECT_EQ(text, print(file->getOperation()));

  auto analysis = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(file->getOperation());
  });
  ASSERT_FALSE(analysis->isValid());
  EXPECT_NE(diagnostics.find("after ordering is retired"), std::string::npos)
      << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, *analysis,
                                                    preparationBudget);
  });
  EXPECT_TRUE(mlir::failed(prepared));
  unchanged(*file, before);
  EXPECT_EQ(text, print(file->getOperation()));
  // The registered route/lower pipeline must reject the same diamond rather
  // than silently dropping predecessor metadata and publishing new hardware.
  rejects(*file, "after ordering is retired");
}

TEST_F(TransactionLoweringTest,
       OriginalSourceProofRejectsAfterAddedSinceAnEarlierSuccessfulProof) {
  for (bool callable : {false, true}) {
    for (bool empty : {false, true}) {
      SCOPED_TRACE(callable);
      SCOPED_TRACE(empty);
      auto file = parse();
      ASSERT_TRUE(file);
      auto predecessor = addEmptyTransaction(*file, "cache_predecessor", 120);
      auto dependent = addEmptyTransaction(*file, "cache_dependent", 122);
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
      SourceTransactionStateAnalysis analysis(file->getOperation());
      ASSERT_TRUE(analysis.isValid()) << diagnostics;
      HardwareSourceCheckLimits limits;
      detail::SourceCheckBudget preparationBudget(limits);
      auto prepared = capture([&] {
        return acir::detail::prepareTransactionLowering(*file, analysis,
                                                        preparationBudget);
      });
      ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
      detail::SourceCheckBudget materializationBudget(limits);
      auto materialized = capture([&] {
        return acir::detail::materializeTransactionLowering(
            *prepared, materializationBudget);
      });
      ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
      detail::SourceCheckBudget firstProofBudget(limits);
      ASSERT_TRUE(mlir::succeeded(capture([&] {
        return acir::detail::verifyTransactionMaterialization(
            *prepared, *materialized, firstProofBudget);
      }))) << diagnostics;

      mlir::Operation *changed = dependent;
      mlir::Attribute target = predecessor.getOccurrence();
      if (callable) {
        changed = named<SourceMethodOp>(*file, "relay");
        target = named<SourceMethodOp>(*file, "bump").getOccurrence();
      }
      changed->setAttr(callable ? "ac.callable_after" : "ac.after",
                       empty ? builder.getArrayAttr({})
                             : builder.getArrayAttr({target}));
      auto sourceBefore = identities(*file);
      auto sourceText = print(file->getOperation());
      auto candidateBefore = identities(*materialized->module);
      auto candidateText = print(materialized->module->getOperation());
      detail::SourceCheckBudget secondProofBudget(limits);
      EXPECT_TRUE(mlir::failed(capture([&] {
        return acir::detail::verifyTransactionMaterialization(
            *prepared, *materialized, secondProofBudget);
      })));
      EXPECT_NE(diagnostics.find("after ordering is retired"), std::string::npos)
          << diagnostics;
      unchanged(*file, sourceBefore);
      EXPECT_EQ(sourceText, print(file->getOperation()));
      unchanged(*materialized->module, candidateBefore);
      EXPECT_EQ(candidateText, print(materialized->module->getOperation()));
      // A prepared-plan cache and an already authenticated candidate do not
      // grant authority to newly attached ordering metadata, even when empty.
      rejects(*file, "after ordering is retired");
      unchanged(*materialized->module, candidateBefore);
      EXPECT_EQ(candidateText, print(materialized->module->getOperation()));
    }
  }
}

TEST_F(TransactionLoweringTest,
       OriginalSourceProofRevalidatesReceiverInventoryInEachRun) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  detail::SourceCheckBudget materializationBudget(limits);
  auto materialized = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        materializationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
  detail::SourceCheckBudget firstProofBudget(limits);
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, firstProofBudget);
  }))) << diagnostics;

  auto candidateBefore = print(materialized->module->getOperation());
  auto root = module(*file, "Root");
  auto left = instance(root, "left"), right = instance(root, "right");
  right->setAttr("occurrence", left.getOccurrence());
  detail::SourceCheckBudget secondProofBudget(limits);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, secondProofBudget);
  })));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_EQ(print(materialized->module->getOperation()), candidateBefore);
  EXPECT_EQ(right.getOccurrence(), left.getOccurrence());
}

TEST_F(TransactionLoweringTest,
       SourceReplayRejectsDuplicateAndInvalidBoundValueIdentities) {
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    SourceTransactionStateAnalysis analysis(file->getOperation());
    ASSERT_TRUE(analysis.isValid()) << diagnostics;
    HardwareSourceCheckLimits limits;
    detail::SourceCheckBudget preparationBudget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*file, analysis,
                                                      preparationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
    detail::SourceCheckBudget materializationBudget(limits);
    auto materialized = capture([&] {
      return acir::detail::materializeTransactionLowering(
          *prepared, materializationBudget);
    });
    ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
    auto originalBefore = identities(*file);
    auto candidateBefore = identities(*materialized->module);
    auto candidateText = print(materialized->module->getOperation());

    auto selected =
        llvm::find_if(prepared->transactions, [](const auto &entry) {
          return entry.plan->frames.size() > 1 &&
                 entry.plan->instances.size() > 1;
        });
    ASSERT_NE(selected, prepared->transactions.end());
    auto &plan = const_cast<TransactionStatePlan &>(*selected->plan);
    ASSERT_GT(plan.values.size(), 1u);
    llvm::SmallVector<BoundValueId> nonLive;
    for (auto id : llvm::seq<uint64_t>(0, plan.values.size()))
      if (!llvm::is_contained(selected->liveIns, id))
        nonLive.push_back(id);
    ASSERT_GT(nonLive.size(), 1u);
    if (mutation == 0) {
      plan.values[nonLive[1]].value = plan.values[nonLive[0]].value;
      plan.values[nonLive[1]].anchor = plan.values[nonLive[0]].anchor;
      plan.values[nonLive[1]].frame = plan.values[nonLive[0]].frame;
    } else if (mutation == 1) {
      plan.values[nonLive[0]].anchor = plan.instances.size();
    } else if (mutation == 2) {
      plan.values[nonLive[0]].frame = plan.frames.size();
    } else if (mutation == 3) {
      auto lexical = llvm::find_if(plan.values, [&](const auto &ref) {
        mlir::Value value = ref.value;
        return ref.frame == plan.rootFrame &&
               value.getParentBlock()->getParentOp() ==
                   plan.instances[ref.anchor].definition;
      });
      ASSERT_NE(lexical, plan.values.end());
      ASSERT_GT(plan.frames.size(), 1u);
      lexical->frame = plan.rootFrame == 0 ? 1 : 0;
    } else {
      auto local = llvm::find_if(plan.values, [&](const auto &ref) {
        return ref.frame != plan.rootFrame &&
               plan.frames[ref.frame].anchor == ref.anchor &&
               plan.instances.size() > 1;
      });
      ASSERT_NE(local, plan.values.end());
      auto expected = plan.frames[local->frame].anchor;
      local->anchor = expected == 0 ? 1 : 0;
      ASSERT_NE(local->anchor, expected);
    }

    detail::SourceCheckBudget proofBudget(limits);
    EXPECT_TRUE(mlir::failed(capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          *prepared, *materialized, proofBudget);
    })));
    if (mutation == 0 || mutation >= 3)
      EXPECT_NE(diagnostics.find("transaction source replay"),
                std::string::npos)
          << diagnostics;
    unchanged(*file, originalBefore);
    unchanged(*materialized->module, candidateBefore);
    EXPECT_EQ(print(materialized->module->getOperation()), candidateText);
  }
}

TEST_F(TransactionLoweringTest,
       SourceReplayTinyProofBudgetPreservesOriginalAndCandidate) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  detail::SourceCheckBudget materializationBudget(limits);
  auto materialized = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        materializationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
  auto originalBefore = identities(*file);
  auto candidateBefore = identities(*materialized->module);
  auto candidateText = print(materialized->module->getOperation());

  HardwareSourceCheckLimits tiny;
  tiny.maxWorkUnits = 1;
  detail::SourceCheckBudget tinyProof(tiny);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, tinyProof);
  })));
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
  unchanged(*file, originalBefore);
  unchanged(*materialized->module, candidateBefore);
  EXPECT_EQ(print(materialized->module->getOperation()), candidateText);
}

TEST_F(TransactionLoweringTest,
       MaterializedFramesContainOnlyInvocationBindings) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  detail::SourceCheckBudget materializationBudget(limits);
  auto materialized = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        materializationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;

  auto selected = llvm::find_if(prepared->transactions, [](const auto &entry) {
    return entry.plan->frames.size() > 1 && !entry.liveIns.empty();
  });
  ASSERT_NE(selected, prepared->transactions.end());
  auto index = std::distance(prepared->transactions.begin(), selected);
  ASSERT_LT(static_cast<size_t>(index), materialized->transactions.size());
  const auto &plan = *selected->plan;
  const auto &record = materialized->transactions[index];
  ASSERT_EQ(record.frames.size(), plan.frames.size());
  ASSERT_EQ(record.boundValues.size(), plan.values.size());
  for (auto [frame, binding] : llvm::enumerate(plan.frames)) {
    for (auto liveIn : selected->liveIns) {
      const auto &ref = plan.values[liveIn];
      if (ref.anchor == binding.anchor)
        EXPECT_FALSE(record.frames[frame].lookupOrNull(ref.value));
    }
    for (auto arguments :
         {llvm::ArrayRef<TransactionValueBinding>(binding.guardArguments),
          llvm::ArrayRef<TransactionValueBinding>(binding.bodyArguments)})
      for (const auto &argument : arguments) {
        ASSERT_LT(argument.actual, record.boundValues.size());
        ASSERT_TRUE(record.boundValues[argument.actual]);
        EXPECT_EQ(record.frames[frame].lookupOrNull(argument.formal),
                  record.boundValues[argument.actual]);
      }
  }
  detail::SourceCheckBudget proofBudget(limits);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, proofBudget);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       OriginalProofDoesNotRequirePerFrameLexicalSeeds) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  SourceTransactionStateAnalysis analysis(file->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  detail::SourceCheckBudget materializationBudget(limits);
  auto materialized = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        materializationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(materialized)) << diagnostics;
  auto originalBefore = identities(*file);
  auto candidateBefore = identities(*materialized->module);
  auto candidateText = print(materialized->module->getOperation());

  ASSERT_EQ(prepared->transactions.size(), materialized->transactions.size());
  for (auto [preparation, record] :
       llvm::zip(prepared->transactions, materialized->transactions)) {
    const auto &plan = *preparation.plan;
    ASSERT_EQ(record.frames.size(), plan.frames.size());
    for (auto [frame, binding] : llvm::enumerate(plan.frames))
      for (auto liveIn : preparation.liveIns) {
        const auto &ref = plan.values[liveIn];
        if (ref.anchor == binding.anchor &&
            record.frames[frame].contains(ref.value))
          record.frames[frame].erase(ref.value);
      }
  }

  detail::SourceCheckBudget proofBudget(limits);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        *prepared, *materialized, proofBudget);
  }))) << diagnostics;
  unchanged(*file, originalBefore);
  unchanged(*materialized->module, candidateBefore);
  EXPECT_EQ(print(materialized->module->getOperation()), candidateText);
}

TEST_F(TransactionLoweringTest,
       LateDetachedFinalVerificationFailureRollsBackAllOriginalHandles) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto zero = *std::next(
      definition.getBody().front().getOps<BitsConstantOp>().begin(), 3);
  builder.setInsertionPoint(definition.getBody().front().getTerminator());
  mlir::OperationState cycle(definition.getLoc(),
                             BitsBinaryOp::getOperationName());
  cycle.addOperands({zero.getResult(), zero.getResult()});
  cycle.addTypes(zero.getResult().getType());
  cycle.addAttribute("opcode", builder.getStringAttr("add"));
  auto *op = builder.create(cycle);
  op->setOperand(0, op->getResult(0));
  // Native graph admission allows a module graph; final dependency validation
  // must also inspect the unused cycle before committing the detached clone.
  rejects(*file);
}

TEST_F(TransactionLoweringTest,
       NativePrimitiveSchemaIsNotPublicRegistryAuthority) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto imported = *file->getOps<ModuleImportOp>().begin();
  HardwareAnalysis hardware(*file);
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return hardware.verifyImport(imported);
  }))) << diagnostics;
  auto registry = capture([&] {
    return compiler::SourceHeaderRegistry::create(
        &context, {}, [&] { return mlir::emitError(builder.getUnknownLoc()); });
  });
  ASSERT_TRUE(mlir::succeeded(registry)) << diagnostics;
  EXPECT_FALSE(registry->isTrustedBuiltin(imported));
  EXPECT_TRUE(mlir::failed(capture([&] {
    return registry->verifyModuleImport(
        imported, [&] { return mlir::emitError(builder.getUnknownLoc()); });
  })));
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest, WholeTableOwnerUsesTypedStorageCollection) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto exemplar = first<SourceStateOp>(definition.getBody());
  auto one = (*definition.getBody().front().getOps<BitsConstantOp>().begin())
                 .getValue();
  auto shape = builder.getArrayAttr({one});
  auto table = TableType::get(&context, shape, exemplar.getQ().getType());
  builder.setInsertionPoint(definition.getBody().front().getTerminator());
  mlir::OperationState initial(definition.getLoc(),
                               TableSplatOp::getOperationName());
  initial.addOperands(exemplar.getInitial());
  initial.addTypes(table);
  initial.addAttribute("shape", shape);
  auto *constructor = builder.create(initial);
  auto *state = exemplar->clone();
  state->setAttr("instance_name", builder.getStringAttr("aggregate"));
  state->setAttr("occurrence", occurrence("Owner", 31));
  state->setOperand(2, constructor->getResult(0));
  state->getResult(0).setType(table);
  definition.getBody().front().getOperations().insert(
      definition.getBody().front().getTerminator()->getIterator(), state);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
  auto storage = first<CollectionOp>(owner(*file).getBody());
  ASSERT_TRUE(storage);
  EXPECT_EQ(
      storage->getAttrOfType<mlir::StringAttr>("instance_name").getValue(),
      "aggregate");
  EXPECT_EQ(storage.getResult(0).getType(), table);
  ASSERT_EQ(storage->getNumOperands(), 5u);
  for (unsigned index = 0; index < 3; ++index) {
    auto control = storage->getOperand(index).getDefiningOp<TableSplatOp>();
    ASSERT_TRUE(control);
    EXPECT_EQ(control.getShape(), shape);
  }
  EXPECT_EQ(storage->getOperand(3).getType(), table);
  EXPECT_EQ(storage->getOperand(4).getType(), table);
}

TEST_F(TransactionLoweringTest, MissingMethodBodyRejectsWithoutMutation) {
  auto absent = parse();
  ASSERT_TRUE(absent);
  named<SourceMethodOp>(*absent, "bump").getBody().getBlocks().clear();
  rejects(*absent);
}

TEST_F(TransactionLoweringTest,
       ThreeLevelCallsKeepPhysicalOwnersLexicalAnchorsAndOldQSnapshots) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto state = first<SourceStateOp>(provider.getBody());
  auto originalOccurrence = state.getOccurrence();
  auto originalOwner = provider->getAttr("source_owner");
  auto originalClock = provider.getBody().front().getArgument(1).getType();
  auto proof = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(proof->isValid()) << diagnostics;
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
  provider = owner(*file);
  auto root = module(*file, "Root"), relay = module(*file, "Relay");
  EXPECT_EQ(std::distance(file->getOps<ModuleOp>().begin(),
                          file->getOps<ModuleOp>().end()),
            3);
  EXPECT_EQ(
      std::distance(provider.getBody().front().getOps<InstanceOp>().begin(),
                    provider.getBody().front().getOps<InstanceOp>().end()),
      1);
  EXPECT_EQ(std::distance(relay.getBody().front().getOps<InstanceOp>().begin(),
                          relay.getBody().front().getOps<InstanceOp>().end()),
            1);
  EXPECT_EQ(std::distance(root.getBody().front().getOps<InstanceOp>().begin(),
                          root.getBody().front().getOps<InstanceOp>().end()),
            2);
  auto storage = instance(provider, "count");
  EXPECT_EQ(storage.getOccurrence(), originalOccurrence);
  EXPECT_EQ(provider->getAttr("source_owner"), originalOwner);
  EXPECT_EQ(storage->getOperand(0), provider.getBody().front().getArgument(1));
  EXPECT_EQ(storage->getOperand(1), provider.getBody().front().getArgument(2));
  EXPECT_EQ(storage->getOperand(0).getType(), originalClock);
  EXPECT_EQ(instance(root, "left").getCallee(),
            instance(root, "right").getCallee());
  auto scope =
      std::make_shared<EvaluationScope>(EvaluationScope{root, {}, {}, ""});
  // Independent old Q values: left=11, right=19. Provider lexical values are
  // 2*seed and 4*seed; the repeated left call consumes its saved first result.
  for (uint64_t seed : {3u, 17u, 61u}) {
    SCOPED_TRACE(seed);
    auto *outputs = root.getBody().front().getTerminator();
    auto firstCandidate = (11 + 3 * seed) & 255;
    auto lastCandidate = (2 * firstCandidate + 2 * seed) & 255;
    EXPECT_EQ(evaluate(*file, outputs->getOperand(0), scope, seed),
              firstCandidate);
    EXPECT_EQ(evaluate(*file, outputs->getOperand(1), scope, seed),
              lastCandidate);
    EXPECT_EQ(evaluate(*file, outputs->getOperand(2), scope, seed), 11u);
    EXPECT_EQ(evaluate(*file, outputs->getOperand(3), scope, seed),
              (19 + 5 * seed) & 255);
    for (auto name : {"left", "right"}) {
      auto relayScope = std::make_shared<EvaluationScope>(
          EvaluationScope{relay, instance(root, name), scope, name});
      auto providerScope = std::make_shared<EvaluationScope>(
          EvaluationScope{provider, instance(relay, "counter"), relayScope,
                          std::string(name) + ".counter"});
      EXPECT_EQ(evaluate(*file, storage->getOperand(2), providerScope, seed),
                1u);
      EXPECT_EQ(evaluate(*file, storage->getOperand(3), providerScope, seed),
                llvm::StringRef(name) == "left" ? lastCandidate
                                                : (19 + 5 * seed) & 255);
    }
  }
  for (auto definition : {provider, relay}) {
    auto layout = capture([&] { return getModulePortLayout(definition); });
    ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
    EXPECT_EQ(layout->sourceInputCount, 3u);
    EXPECT_EQ(layout->sourceOutputCount, 1u);
    EXPECT_EQ(definition.getInputNames()[0], builder.getStringAttr("seed"));
    EXPECT_EQ(definition.getInputNames()[1], builder.getStringAttr("pyc_clk"));
    EXPECT_EQ(definition.getInputNames()[2], builder.getStringAttr("pyc_rst"));
    EXPECT_EQ(layout->rootInputDefaults.size(),
              definition.getFunctionType().getNumInputs() - 3);
  }
  auto restored = mlir::parseSourceString<mlir::ModuleOp>(
      print(file->getOperation()), &context);
  ASSERT_TRUE(restored);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*restored);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       RouteSuffixesFollowFirstDiscoveryAndDeduplicateSharedDefinitions) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto relay = module(*file, "Relay");
  auto root = module(*file, "Root");
  auto zeta = instance(relay, "counter");
  zeta->setAttr("instance_name", builder.getStringAttr("zeta"));
  zeta->setAttr("occurrence", occurrence("Relay", 90));
  auto alpha = addInstance(relay, provider, "alpha", zeta->getOperands(), 0);
  alpha->moveBefore(zeta);
  // Declaration order, occurrence order and lexical name order all put alpha
  // first. The actual method encounter is zeta, alpha; repeated forward calls
  // and both Root instances must reuse those two definition-relative routes.
  auto forward = named<SourceMethodOp>(relay, "forward");
  auto nested = first<SourceMethodCallOp>(forward.getBody());
  nested->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr("zeta")}));
  builder.setInsertionPoint(forward.getBody().front().getTerminator());
  call(forward.getBody().front().getArgument(1),
       forward.getBody().front().getArgument(0), relay, "Owner", "bump",
       "alpha", {provider.getFunctionType().getResult(0)}, 3);
  first<SourceMethodCallOp>(named<SourceMethodOp>(relay, "peek").getBody())
      ->setAttr("instance_path",
                builder.getArrayAttr({builder.getStringAttr("zeta")}));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, *analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  const auto &relayRoutes = prepared->definitions[
      prepared->definitionIndex.lookup(relay.getOperation())];
  ASSERT_EQ(relayRoutes.downward.size(), 2u);
  ASSERT_EQ(relayRoutes.downward[0].path.size(), 1u);
  ASSERT_EQ(relayRoutes.downward[1].path.size(), 1u);
  EXPECT_EQ(relayRoutes.downward[0].path[0], zeta);
  EXPECT_EQ(relayRoutes.downward[1].path[0], alpha);
  auto state = first<SourceStateOp>(provider.getBody());
  EXPECT_EQ(relayRoutes.downward[0].value, state.getQ());
  EXPECT_EQ(relayRoutes.downward[1].value, state.getQ());
  const auto &providerRoutes = prepared->definitions[
      prepared->definitionIndex.lookup(provider.getOperation())];
  ASSERT_EQ(providerRoutes.downward.size(), 1u);
  EXPECT_TRUE(providerRoutes.downward[0].path.empty());
  EXPECT_EQ(providerRoutes.downward[0].value, state.getQ());
  unsigned downwardCount = 0;
  for (const auto &definition : prepared->definitions)
    downwardCount += definition.downward.size();
  EXPECT_EQ(downwardCount, 3u);

  auto stateOccurrence = state.getOccurrence();
  auto zetaOccurrence = zeta.getOccurrence();
  auto alphaOccurrence = alpha.getOccurrence();
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    mlir::PassManager manager(&context);
    manager.addPass(acir::createRouteTransactionsPass());
    return manager.run(*file);
  }))) << diagnostics;
  relay = module(*file, "Relay");
  provider = owner(*file);
  auto incoming = relay->getAttrOfType<mlir::DictionaryAttr>(
      "ac.transaction_wiring").getAs<mlir::ArrayAttr>("incoming");
  ASSERT_EQ(incoming.size(), 2u);
  for (unsigned ordinal = 0; ordinal < 2; ++ordinal) {
    SCOPED_TRACE(ordinal);
    auto entry = mlir::cast<mlir::DictionaryAttr>(incoming[ordinal]);
    auto endpoint = entry.getAs<mlir::DictionaryAttr>("state");
    EXPECT_EQ(endpoint.getAs<mlir::DictionaryAttr>("declaration"),
              stateOccurrence);
    EXPECT_EQ(endpoint.getAs<mlir::DictionaryAttr>("owner")
                  .getAs<mlir::ArrayAttr>("instance_path"),
              builder.getArrayAttr({ordinal == 0 ? zetaOccurrence
                                                : alphaOccurrence}));
    EXPECT_EQ(entry.getAs<mlir::IntegerAttr>("data").getInt(),
              3 + 2 * ordinal);
    EXPECT_EQ(entry.getAs<mlir::IntegerAttr>("enable").getInt(),
              4 + 2 * ordinal);
    EXPECT_EQ(relay.getInputNames()[3 + 2 * ordinal],
              builder.getStringAttr("__ac_transaction_d_" +
                                    std::to_string(ordinal)));
    EXPECT_EQ(relay.getInputNames()[4 + 2 * ordinal],
              builder.getStringAttr("__ac_transaction_e_" +
                                    std::to_string(ordinal)));
    auto allocation = instance(relay, ordinal == 0 ? "zeta" : "alpha");
    auto closed = allocation->getOperand(3).getDefiningOp<ValueMergeOp>();
    ASSERT_TRUE(closed);
    ASSERT_EQ(closed.getGuards().size(), 1u);
    ASSERT_EQ(closed.getValues().size(), 1u);
    auto guard = closed.getGuards()[0].getDefiningOp<BitsBinaryOp>();
    ASSERT_TRUE(guard);
    EXPECT_EQ(guard.getOpcode(), "and");
    EXPECT_EQ(guard.getLhs(),
              relay.getBody().front().getArgument(4 + 2 * ordinal));
    EXPECT_EQ(closed.getValues()[0],
              relay.getBody().front().getArgument(3 + 2 * ordinal));
    EXPECT_EQ(closed.getBase().getDefiningOp(), allocation.getOperation());
    EXPECT_EQ(allocation->getOperand(4),
              relay.getBody().front().getArgument(4 + 2 * ordinal));
  }
  EXPECT_EQ(relay.getFunctionType().getNumInputs(), 7u);
  EXPECT_EQ(provider.getFunctionType().getNumInputs(), 5u);
  EXPECT_EQ(provider->getAttrOfType<mlir::DictionaryAttr>(
                "ac.transaction_wiring").getAs<mlir::ArrayAttr>("incoming")
                .size(),
            1u);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyRoutedPackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       AncestorAndChildIdentityWritersConflictIncludingUnusedDefinitions) {
  for (bool selectedProvider : {false, true}) {
    SCOPED_TRACE(selectedProvider);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    auto provider = owner(*file);
    observer(provider, "count");
    if (selectedProvider) {
      auto system = *file->getOps<SystemOp>().begin();
      mlir::NamedAttrList entry(system.getEntry());
      entry.set("callee", mlir::FlatSymbolRefAttr::get(&context, "Owner"));
      system->setAttr("entry", entry.getDictionary(&context));
    }
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    rejects(*file);
  }
}

TEST_F(TransactionLoweringTest,
       ChildAutonomousObserverReadsOldQAndWritesADifferentOwner) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto state = first<SourceStateOp>(provider.getBody());
  auto *copy = state->clone();
  copy->setAttr("instance_name", builder.getStringAttr("seen"));
  copy->setAttr("occurrence", occurrence("Owner", 44));
  provider.getBody().front().getOperations().insert(state->getIterator(), copy);
  observer(provider, "seen");
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  provider = owner(*file);
  auto root = module(*file, "Root"), relay = module(*file, "Relay");
  auto scope =
      std::make_shared<EvaluationScope>(EvaluationScope{root, {}, {}, ""});
  auto seen = instance(provider, "seen");
  for (auto name : {"left", "right"}) {
    auto relayScope = std::make_shared<EvaluationScope>(
        EvaluationScope{relay, instance(root, name), scope, name});
    auto providerScope = std::make_shared<EvaluationScope>(
        EvaluationScope{provider, instance(relay, "counter"), relayScope,
                        std::string(name) + ".counter"});
    EXPECT_EQ(evaluate(*file, seen->getOperand(3), providerScope),
              llvm::StringRef(name) == "left" ? 11u : 19u);
  }
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       InstanceWithNoExternalWriterReceivesExplicitKnownZeroInputs) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root"), relay = module(*file, "Relay");
  auto *terminator = root.getBody().front().getTerminator();
  auto idle = addInstance(root, relay, "idle",
                          root.getBody().front().getArguments(), 50);
  idle->moveBefore(terminator);
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  root = module(*file, "Root");
  relay = module(*file, "Relay");
  idle = instance(root, "idle");
  auto layout = capture([&] { return getModulePortLayout(relay); });
  ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
  auto scope =
      std::make_shared<EvaluationScope>(EvaluationScope{root, {}, {}, ""});
  for (auto value : idle->getOperands().drop_front(layout->sourceInputCount)) {
    EXPECT_TRUE(value.getDefiningOp<BitsConstantOp>());
    EXPECT_EQ(evaluate(*file, value, scope), 0u);
  }
  EXPECT_EQ(idle->getNumOperands(), relay.getFunctionType().getNumInputs());
}

TEST_F(TransactionLoweringTest,
       SelectedProviderStillHasOnePhysicalDefinitionAndFullChildArity) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto system = *file->getOps<SystemOp>().begin();
  mlir::NamedAttrList entry(system.getEntry());
  entry.set("callee", mlir::FlatSymbolRefAttr::get(&context, "Owner"));
  system->setAttr("entry", entry.getDictionary(&context));
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  auto provider = owner(*file);
  auto layout = capture([&] { return getModulePortLayout(provider); });
  ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
  EXPECT_EQ(layout->sourceInputCount, 3u);
  EXPECT_EQ(layout->sourceOutputCount, 1u);
  ASSERT_GT(layout->rootInputDefaults.size(), 0u);
  for (auto raw : layout->rootInputDefaults)
    EXPECT_EQ(raw, builder.getI64IntegerAttr(0));
  auto child = instance(module(*file, "Relay"), "counter");
  EXPECT_EQ(child->getNumOperands(), provider.getFunctionType().getNumInputs());
  EXPECT_EQ(child->getNumResults(), provider.getFunctionType().getNumResults());
  auto inputs = llvm::SmallVector<mlir::Value>(child->getOperands());
  inputs.pop_back();
  child->setOperands(inputs);
  EXPECT_TRUE(
      mlir::failed(capture([&] { return verifyHardwarePackage(*file); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(TransactionLoweringTest,
       InternalPortLayoutStrictFieldsTypesRangesAndKnownZeroDefaults) {
  for (unsigned mutation = 0; mutation < 14; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    auto provider = owner(*file);
    auto original =
        provider->getAttrOfType<mlir::DictionaryAttr>("ac.internal_ports");
    ASSERT_TRUE(original);
    mlir::NamedAttrList fields(original);
    auto defaults = llvm::SmallVector<mlir::Attribute>(
        original.getAs<mlir::ArrayAttr>("root_inputs").getValue());
    ASSERT_FALSE(defaults.empty());
    if (mutation == 0)
      fields.set("unknown", builder.getUnitAttr());
    if (mutation == 1)
      fields.erase("source_outputs");
    if (mutation == 2)
      fields.set("source_inputs", builder.getI32IntegerAttr(3));
    if (mutation == 3)
      fields.set("source_outputs", builder.getStringAttr("1"));
    if (mutation == 4)
      fields.set("source_inputs", builder.getI64IntegerAttr(-1));
    if (mutation == 5)
      fields.set("source_outputs", builder.getI64IntegerAttr(-1));
    if (mutation == 6)
      fields.set("source_inputs",
                 builder.getI64IntegerAttr(
                     provider.getFunctionType().getNumInputs() + 1));
    if (mutation == 7)
      fields.set("source_outputs",
                 builder.getI64IntegerAttr(
                     provider.getFunctionType().getNumResults() + 1));
    if (mutation == 8)
      defaults.pop_back();
    if (mutation == 9)
      defaults.push_back(builder.getI64IntegerAttr(0));
    if (mutation == 10)
      defaults[0] = builder.getI64IntegerAttr(1);
    if (mutation == 11)
      defaults[0] = builder.getI32IntegerAttr(0);
    if (mutation == 12)
      defaults[0] = builder.getStringAttr("0");
    fields.set("root_inputs", builder.getArrayAttr(defaults));
    if (mutation == 13) {
      fields.set(
          "source_inputs",
          builder.getI64IntegerAttr(provider.getFunctionType().getNumInputs()));
      fields.set("source_outputs",
                 builder.getI64IntegerAttr(
                     provider.getFunctionType().getNumResults()));
      fields.set("root_inputs", builder.getArrayAttr({}));
    }
    provider->setAttr("ac.internal_ports", fields.getDictionary(&context));
    EXPECT_TRUE(
        mlir::failed(capture([&] { return getModulePortLayout(provider); })));
    EXPECT_FALSE(diagnostics.empty());
  }
  auto file = crossFixture();
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  auto provider = owner(*file);
  provider->setAttr("ac.internal_ports", builder.getArrayAttr({}));
  EXPECT_TRUE(
      mlir::failed(capture([&] { return getModulePortLayout(provider); })));
}

TEST_F(TransactionLoweringTest,
       InternalPortMetadataIsFinalOnlyAndForbiddenOnImportsOrOtherOperations) {
  for (unsigned target = 0; target < 5; ++target) {
    SCOPED_TRACE(target);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    auto provider = owner(*file);
    auto metadata = provider->getAttr("ac.internal_ports");
    ASSERT_TRUE(metadata);
    if (target == 0) {
      file->getOperation()->removeAttr("ac.stage");
      EXPECT_TRUE(
          mlir::failed(capture([&] { return getModulePortLayout(provider); })));
    } else {
      mlir::Operation *operation =
          target == 1 ? (*file->getOps<ModuleImportOp>().begin()).getOperation()
          : target == 2 ? (*file->getOps<SystemOp>().begin()).getOperation()
          : target == 3 ? file->getOperation()
                        : instance(provider, "count").getOperation();
      operation->setAttr("ac.internal_ports", metadata);
      EXPECT_TRUE(
          mlir::failed(capture([&] { return verifyHardwarePackage(*file); })));
    }
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(TransactionLoweringTest,
       CrossInstanceLateCombinationalCyclePreservesEveryOriginalHandle) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  builder.setInsertionPoint(provider.getBody().front().getTerminator());
  auto seed = provider.getBody().front().getArgument(0);
  auto cycle = add(seed, seed, provider.getLoc());
  cycle->setOperand(0, cycle.getResult());
  // The unused cycle is a real graph defect after routing materializes. It
  // cannot disappear through dead-result cleanup or an internal-output cut.
  auto proof = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(proof->isValid()) << diagnostics;
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  rejects(*file);
}

TEST_F(TransactionLoweringTest,
       CollectionSharingExtendedDefinitionRejectsBeforeAnyMutation) {
  for (auto childName : {"Owner", "Relay"}) {
    SCOPED_TRACE(childName);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    addCollection(*file, module(*file, childName));
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    rejects(*file, "Collection");
  }
}

TEST_F(TransactionLoweringTest,
       CollectionWriterCompositionRejectsWithoutAnySignatureExtension) {
  auto file = parse();
  ASSERT_TRUE(file);
  addCollection(*file, owner(*file));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  rejects(*file, "Collection");
}

TEST_F(TransactionLoweringTest,
       UnaffectedCollectionRetainsItsOriginalShapeAndColumnSignature) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto pure = emptyModule(*file, owner(*file), "Pure",
                          {owner(*file).getFunctionType().getResult(0)});
  builder.setInsertionPointToEnd(&pure.getBody().front());
  yield(pure.getBody().front().getArgument(0), pure.getLoc());
  auto collection = addCollection(*file, pure);
  auto originalType = pure.getFunctionType();
  auto shape = collection.getShape();
  auto originalOccurrence = collection.getOccurrence();
  auto inputs = llvm::SmallVector<mlir::Type>(collection->getOperandTypes());
  auto outputs = llvm::SmallVector<mlir::Type>(collection->getResultTypes());
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  pure = module(*file, "Pure");
  EXPECT_EQ(pure.getFunctionType(), originalType);
  EXPECT_FALSE(pure->hasAttr("ac.internal_ports"));
  auto grouped = module(*file, "Grouped");
  ASSERT_EQ(
      std::distance(grouped.getBody().front().getOps<CollectionOp>().begin(),
                    grouped.getBody().front().getOps<CollectionOp>().end()),
      1);
  collection = *grouped.getBody().front().getOps<CollectionOp>().begin();
  EXPECT_EQ(collection.getShape(), shape);
  EXPECT_EQ(collection.getOccurrence(), originalOccurrence);
  EXPECT_EQ(llvm::SmallVector<mlir::Type>(collection->getOperandTypes()),
            inputs);
  EXPECT_EQ(llvm::SmallVector<mlir::Type>(collection->getResultTypes()),
            outputs);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       SourcePrefixMetadataAndDomainOrdinalsSurviveFullPhysicalSuffix) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto bump = named<SourceMethodOp>(provider, "bump");
  auto byte = provider.getFunctionType().getInput(0);
  auto packet = StructType::get(&context, builder.getStringAttr("Packet"));
  builder.setInsertionPointToStart(file->getBody());
  mlir::OperationState declaration(provider.getLoc(),
                                   StructOp::getOperationName());
  declaration.addAttribute("sym_name", builder.getStringAttr("Packet"));
  declaration.addAttribute(
      "fields",
      builder.getArrayAttr({builder.getDictionaryAttr(
          {builder.getNamedAttr("name", builder.getStringAttr("value")),
           builder.getNamedAttr("type", mlir::TypeAttr::get(byte))})}));
  builder.create(declaration);
  auto record = [&](mlir::Value value) {
    mlir::OperationState create(provider.getLoc(),
                                StructCreateOp::getOperationName());
    create.addOperands(value);
    create.addTypes(packet);
    return builder.create(create)->getResult(0);
  };
  builder.setInsertionPoint(bump);
  auto lexical = record(provider.getBody().front().getArgument(0));
  auto captures = llvm::SmallVector<mlir::Value>(bump->getOperands());
  captures.push_back(lexical);
  bump->setOperands(captures);
  for (auto &region : bump->getRegions())
    region.front().addArgument(packet, bump.getLoc());
  auto step = *bump.getBody().front().getOps<BitsBinaryOp>().begin();
  builder.setInsertionPoint(step);
  mlir::OperationState projection(provider.getLoc(),
                                  StructGetOp::getOperationName());
  projection.addOperands(bump.getBody().front().getArguments().back());
  projection.addTypes(byte);
  projection.addAttribute("field", builder.getStringAttr("value"));
  step->setOperand(1, builder.create(projection)->getResult(0));
  // The full source module contract requires a nominal Struct result. Both
  // existing forwarding instances retain that exact original source prefix.
  auto *output = provider.getBody().front().getTerminator();
  builder.setInsertionPoint(output);
  output->setOperand(0, record(output->getOperand(0)));
  for (auto definition : {provider, module(*file, "Relay")})
    definition->setAttr(
        "function_type",
        mlir::TypeAttr::get(builder.getFunctionType(
            definition.getFunctionType().getInputs(), {packet})));
  instance(module(*file, "Relay"), "counter")->getResult(0).setType(packet);
  for (auto name : {"left", "right"})
    instance(module(*file, "Root"), name)->getResult(0).setType(packet);
  auto parameter = mlir::cast<mlir::DictionaryAttr>(
      bump->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
  mlir::NamedAttrList fields(parameter);
  fields.set("name", builder.getStringAttr("seed"));
  provider->setAttr("ac.parameters",
                    builder.getArrayAttr({fields.getDictionary(&context)}));
  provider->setAttr(
      "ac.result_constraints",
      builder.getArrayAttr({builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
           builder.getNamedAttr("type", mlir::TypeAttr::get(packet)),
           builder.getNamedAttr("source_kind",
                                builder.getStringAttr("nominal"))})}));
  provider->setAttr("ac.return_form", builder.getStringAttr("single"));
  auto parameters = provider->getAttr("ac.parameters");
  auto results = provider->getAttr("ac.result_constraints");
  auto domain = provider->getAttr("ac.domain_inputs");
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  provider = owner(*file);
  EXPECT_EQ(provider->getAttr("ac.parameters"), parameters);
  EXPECT_EQ(provider->getAttr("ac.result_constraints"), results);
  EXPECT_EQ(provider->getAttr("ac.domain_inputs"), domain);
  EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  for (auto definition : {provider, module(*file, "Relay")}) {
    auto layout = capture([&] { return getModulePortLayout(definition); });
    ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
    EXPECT_EQ(layout->sourceOutputCount, 1u);
    EXPECT_EQ(definition.getFunctionType().getResult(0), packet);
    unsigned capturedRecords = 0;
    for (auto type : definition.getFunctionType().getResults().drop_front(
             layout->sourceOutputCount))
      capturedRecords += type == packet;
    EXPECT_EQ(capturedRecords, 1u);
  }
  auto root = module(*file, "Root");
  auto rule = named<RuleOp>(root, "run");
  EXPECT_TRUE(llvm::is_contained(rule->getOperandTypes(), packet));
  auto scope =
      std::make_shared<EvaluationScope>(EvaluationScope{root, {}, {}, ""});
  auto *outputs = root.getBody().front().getTerminator();
  EXPECT_EQ(evaluate(*file, outputs->getOperand(0), scope), 20u);
  EXPECT_EQ(evaluate(*file, outputs->getOperand(1), scope), 46u);
  EXPECT_EQ(evaluate(*file, outputs->getOperand(2), scope), 11u);
  EXPECT_EQ(evaluate(*file, outputs->getOperand(3), scope), 34u);
  auto arity = provider.getFunctionType().getNumInputs();
  ASSERT_GT(arity, 3u);
  provider->setAttr(
      "ac.domain_inputs",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("clock", builder.getI64IntegerAttr(arity - 2)),
           builder.getNamedAttr("reset",
                                builder.getI64IntegerAttr(arity - 1))}));
  EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
}

TEST_F(TransactionLoweringTest,
       InternalRootDefaultsAdmitFiniteTableAndRejectZeroWidthSuffixes) {
  for (bool aggregate : {false, true}) {
    SCOPED_TRACE(aggregate);
    auto file = crossFixture();
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
    auto provider = owner(*file);
    auto layout = capture([&] { return getModulePortLayout(provider); });
    ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
    auto oldType = provider.getFunctionType();
    llvm::SmallVector<mlir::Type> inputs(oldType.getInputs());
    auto constants = provider.getBody().front().getOps<BitsConstantOp>();
    auto zero = *std::next(constants.begin());
    auto one = *constants.begin();
    inputs[layout->sourceInputCount] =
        aggregate
            ? mlir::Type(TableType::get(&context,
                                        builder.getArrayAttr({one.getValue()}),
                                        inputs[layout->sourceInputCount]))
            : mlir::Type(BitsType::get(&context, zero.getValue()));
    provider->setAttr("function_type",
                      mlir::TypeAttr::get(builder.getFunctionType(
                          inputs, oldType.getResults())));
    auto updated = capture([&] { return getModulePortLayout(provider); });
    if (aggregate) {
      ASSERT_TRUE(mlir::succeeded(updated)) << diagnostics;
      EXPECT_EQ(updated->sourceInputCount, layout->sourceInputCount);
      EXPECT_EQ(updated->rootInputDefaults, layout->rootInputDefaults);
      // The layout checker admits a finite typed suffix, but a signature-only
      // mutation is still not a valid module: its block arguments disagree.
      EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    } else {
      EXPECT_TRUE(mlir::failed(updated));
      EXPECT_FALSE(diagnostics.empty());
    }
  }
}

TEST_F(TransactionLoweringTest,
       AggregateLexicalCaptureIsRoutedAsOneTypedPayloadFromItsProvider) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto bump = named<SourceMethodOp>(provider, "bump");
  auto one = *provider.getBody().front().getOps<BitsConstantOp>().begin();
  auto shape = builder.getArrayAttr({one.getValue()});
  auto table = TableType::get(
      &context, shape, provider.getBody().front().getArgument(0).getType());
  builder.setInsertionPoint(bump);
  mlir::OperationState splat(provider.getLoc(),
                             TableSplatOp::getOperationName());
  splat.addOperands(provider.getBody().front().getArgument(0));
  splat.addTypes(table);
  splat.addAttribute("shape", shape);
  auto *payload = builder.create(splat);
  auto captures = llvm::SmallVector<mlir::Value>(bump->getOperands());
  captures.push_back(payload->getResult(0));
  bump->setOperands(captures);
  for (auto &region : bump->getRegions())
    region.front().addArgument(table, bump.getLoc());
  auto step = *bump.getBody().front().getOps<BitsBinaryOp>().begin();
  builder.setInsertionPoint(step);
  mlir::OperationState fold(provider.getLoc(), TableFoldOp::getOperationName());
  fold.addOperands(bump.getBody().front().getArguments().back());
  fold.addTypes(provider.getFunctionType().getInput(0));
  fold.addAttribute("kind", builder.getStringAttr("xor"));
  step->setOperand(1, builder.create(fold)->getResult(0));
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  for (auto name : {"Owner", "Relay"}) {
    auto definition = module(*file, name);
    auto layout = capture([&] { return getModulePortLayout(definition); });
    ASSERT_TRUE(mlir::succeeded(layout)) << diagnostics;
    unsigned payloadPorts = 0;
    for (auto type : definition.getFunctionType().getResults().drop_front(
             layout->sourceOutputCount))
      payloadPorts += type == table;
    EXPECT_EQ(payloadPorts, 1u);
  }
  auto rule = named<RuleOp>(module(*file, "Root"), "run");
  EXPECT_TRUE(llvm::is_contained(rule->getOperandTypes(), table));
  auto root = module(*file, "Root");
  auto scope =
      std::make_shared<EvaluationScope>(EvaluationScope{root, {}, {}, ""});
  auto *outputs = root.getBody().front().getTerminator();
  EXPECT_EQ(evaluate(*file, outputs->getOperand(0), scope), 20u);
  EXPECT_EQ(evaluate(*file, outputs->getOperand(1), scope), 46u);
  EXPECT_EQ(evaluate(*file, outputs->getOperand(3), scope), 34u);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       DeepPhysicalRoutingPathBudgetRejectsWithOriginalObjectsIntact) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto child = provider;
  constexpr unsigned depth = 258;
  for (unsigned i = 0; i < depth; ++i) {
    auto layer = emptyModule(*file, provider, "Layer" + std::to_string(i),
                             provider.getFunctionType().getResults());
    auto allocation = addInstance(layer, child, "node",
                                  layer.getBody().front().getArguments(), 0);
    auto forward = named<SourceMethodOp>(child, i ? "forward" : "bump");
    forwardingMethod(layer, forward, child, "node", "forward", 10);
    forwardingMethod(layer, named<SourceMethodOp>(child, "peek"), child,
                     "node", "peek", 20);
    builder.setInsertionPointToEnd(&layer.getBody().front());
    yield(allocation->getResults(), layer.getLoc());
    child = layer;
  }
  auto root = module(*file, "Root");
  instance(root, "left")
      ->setAttr("callee",
                mlir::FlatSymbolRefAttr::get(&context, child.getSymName()));
  for (auto call : named<SourceTransactionOp>(root, "run")
                       .getBody()
                       .front()
                       .getOps<SourceMethodCallOp>()) {
    auto method = call.getCallee().getNestedReferences().back().getValue();
    call->setAttr("callee",
                  mlir::SymbolRefAttr::get(
                      &context, child.getSymName(),
                      {mlir::FlatSymbolRefAttr::get(
                          &context, method == "peek" ? "peek" : "forward")}));
    call->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr("left")}));
  }
  rejects(*file);
  EXPECT_TRUE(diagnostics.find("budget") != std::string::npos ||
              diagnostics.find("depth") != std::string::npos)
      << diagnostics;
}

TEST_F(TransactionLoweringTest,
       RepeatedIdleInstanceRoutePathsExceedTransactionPreparationBudget) {
  auto file = crossFixture();
  ASSERT_TRUE(file);
  auto provider = owner(*file);
  auto child = module(*file, "Relay");
  auto count = first<SourceStateOp>(provider.getBody());
  auto bump = named<SourceMethodOp>(provider, "bump");
  auto write = first<SourceStateWriteOp>(bump.getBody());
  constexpr unsigned routedStates = 24;
  for (unsigned i = 1; i < routedStates; ++i) {
    auto name = "routed" + std::to_string(i);
    auto *stateCopy = count->clone();
    stateCopy->setAttr("instance_name", builder.getStringAttr(name));
    stateCopy->setAttr("occurrence", occurrence("Owner", 100 + i));
    provider.getBody().front().getOperations().insert(count->getIterator(),
                                                      stateCopy);
    auto *writeCopy = write->clone();
    writeCopy->setAttr("state", builder.getStringAttr(name));
    writeCopy->setAttr("occurrence", occurrence("Owner", 200 + i));
    bump.getBody().front().getOperations().insert(write->getIterator(),
                                                  writeCopy);
  }
  // Keep receiver resolution below its depth budget. The sole active call
  // reaches Relay.counter through 30 ordinary wrapper definitions; every idle
  // occurrence shares all sixteen downward state routes.
  constexpr unsigned depth = 30;
  for (unsigned i = 0; i < depth; ++i) {
    auto layer = emptyModule(*file, provider, "Shared" + std::to_string(i),
                             child.getFunctionType().getResults());
    auto allocation = addInstance(layer, child, "node",
                                  layer.getBody().front().getArguments(), 0);
    auto forward = named<SourceMethodOp>(child, "forward");
    forwardingMethod(layer, forward, child, "node", "forward", 10);
    forwardingMethod(layer, named<SourceMethodOp>(child, "peek"), child,
                     "node", "peek", 20);
    builder.setInsertionPointToEnd(&layer.getBody().front());
    yield(allocation->getResults(), layer.getLoc());
    child = layer;
  }

  auto root = module(*file, "Root");
  instance(root, "left")
      ->setAttr("callee",
                mlir::FlatSymbolRefAttr::get(&context, child.getSymName()));
  auto run = named<SourceTransactionOp>(root, "run");
  llvm::SmallVector<SourceMethodCallOp> calls;
  llvm::append_range(calls, run.getBody().front().getOps<SourceMethodCallOp>());
  ASSERT_EQ(calls.size(), 3u);
  auto active = calls.front();
  active->setAttr(
      "callee",
      mlir::SymbolRefAttr::get(
          &context, child.getSymName(),
          {mlir::FlatSymbolRefAttr::get(&context, "forward")}));
  active->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr("left")}));
  run.getBody().front().getTerminator()->setOperands(
      {active.getResult(0), active.getResult(0), active.getResult(0)});
  for (unsigned i = 1; i < calls.size(); ++i)
    calls[i].erase();
  root.getBody().front().getTerminator()->setOperand(
      3, root.getBody().front().getArgument(0));
  named<SourceTransactionOp>(root, "right_run").erase();

  constexpr unsigned idleDefinitions = 8;
  constexpr unsigned instancesPerDefinition = 16;
  for (unsigned i = 0; i < idleDefinitions; ++i) {
    auto parent = emptyModule(*file, provider, "IdleParent" + std::to_string(i),
                              child.getFunctionType().getResults());
    InstanceOp idle;
    for (unsigned j = 0; j < instancesPerDefinition; ++j)
      idle = addInstance(parent, child, "idle" + std::to_string(j),
                         parent.getBody().front().getArguments(), j);
    builder.setInsertionPointToEnd(&parent.getBody().front());
    yield(idle->getResults(), parent.getLoc());
  }
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  ASSERT_NE(analysis->lookup(named<SourceTransactionOp>(root, "run")), nullptr);
  rejects(*file, "prep: route arities");
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       StateExpansionBudgetFailureKeepsOriginalPackage) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto relay = named<SourceMethodOp>(*file, "relay");
  for (unsigned i = 0; i < 22; ++i) {
    auto *copy = relay->clone();
    copy->setAttr("sym_name",
                  builder.getStringAttr("chain" + std::to_string(i)));
    copy->setAttr("occurrence", occurrence("Owner", 100 + i));
    owner(*file).getBody().front().getOperations().insert(relay->getIterator(),
                                                          copy);
    auto method = mlir::cast<SourceMethodOp>(copy);
    auto call = first<SourceMethodCallOp>(method.getBody());
    if (i + 1 != 22) {
      call->setAttr("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Owner",
                        {mlir::FlatSymbolRefAttr::get(
                            &context, "chain" + std::to_string(i + 1))}));
      auto *second = call->clone();
      second->setAttr("occurrence", occurrence("Owner", 300 + 2 * i));
      method.getBody().front().getOperations().insert(call->getIterator(),
                                                      second);
    }
    call->setAttr("occurrence", occurrence("Owner", 301 + 2 * i));
  }
  first<SourceMethodCallOp>(named<SourceTransactionOp>(*file, "run").getBody())
      ->setAttr("callee",
                mlir::SymbolRefAttr::get(
                    &context, "Owner",
                    {mlir::FlatSymbolRefAttr::get(&context, "chain0")}));
  rejects(*file, "budget");
}

TEST_F(
    TransactionLoweringTest,
    ExplicitMaterializationBudgetRejectsAtomicallyAfterSuccessfulPreparation) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto run = named<SourceTransactionOp>(*file, "run");
  auto seed = definition.getBody().front().getArgument(0);
  constexpr unsigned liveInCount = 4;
  builder.setInsertionPoint(run);
  mlir::OperationState fanout(definition.getLoc(), RuleOp::getOperationName());
  fanout.addOperands(seed);
  fanout.addTypes(llvm::SmallVector<mlir::Type>(liveInCount, seed.getType()));
  fanout.addAttribute("name", builder.getStringAttr("live_in_fanout"));
  fanout.addAttribute("occurrence", occurrence("Owner", 40));
  fanout.addRegion();
  auto *pure = builder.create(fanout);
  auto *body = new mlir::Block;
  pure->getRegion(0).push_back(body);
  auto input = body->addArgument(seed.getType(), pure->getLoc());
  builder.setInsertionPointToEnd(body);
  mlir::OperationState fanoutYield(pure->getLoc(), YieldOp::getOperationName());
  fanoutYield.addOperands(llvm::SmallVector<mlir::Value>(liveInCount, input));
  builder.create(fanoutYield);
  auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
  auto unused = named<SourceMethodOp>(*file, "uncalled");
  auto *noop = unused->clone();
  noop->setAttr("sym_name", builder.getStringAttr("noop"));
  noop->setAttr("occurrence", occurrence("Owner", 41));
  noop->setOperand(0, yes.getResult());
  llvm::SmallVector<mlir::Value> captures(noop->getOperands());
  captures.append(pure->result_begin(), pure->result_end());
  noop->setOperands(captures);
  for (auto &region : noop->getRegions())
    for (unsigned i = 0; i < liveInCount; ++i)
      region.front().addArgument(seed.getType(), noop->getLoc());
  auto &noopBody = noop->getRegion(1).front();
  builder.setInsertionPoint(noopBody.getTerminator());
  mlir::Value used = noopBody.getArguments().take_back(liveInCount).front();
  for (auto value : noopBody.getArguments().take_back(liveInCount).drop_front())
    used = add(used, value, noop->getLoc()).getResult();
  definition.getBody().front().getOperations().insert(unused->getIterator(),
                                                      noop);
  constexpr unsigned levels = 4;
  for (unsigned i = 0; i < levels; ++i) {
    auto *copy = noop->clone();
    copy->setAttr("sym_name",
                  builder.getStringAttr("tree" + std::to_string(i)));
    copy->setAttr("occurrence", occurrence("Owner", 50 + i));
    definition.getBody().front().getOperations().insert(unused->getIterator(),
                                                        copy);
    auto method = mlir::cast<SourceMethodOp>(copy);
    builder.setInsertionPoint(method.getBody().front().getTerminator());
    for (unsigned branch = 0; branch < 2; ++branch) {
      mlir::OperationState call(method.getLoc(),
                                SourceMethodCallOp::getOperationName());
      call.addOperands(method.getBody().front().getArgument(0));
      call.addAttribute(
          "callee",
          mlir::SymbolRefAttr::get(
              &context, "Owner",
              {mlir::FlatSymbolRefAttr::get(
                  &context,
                  i + 1 == levels ? "noop" : "tree" + std::to_string(i + 1))}));
      call.addAttribute("instance_path", builder.getArrayAttr({}));
      call.addAttribute("occurrence",
                        occurrence("Owner", 100 + 2 * i + branch));
      builder.create(call);
    }
  }
  builder.setInsertionPoint(run.getBody().front().getTerminator());
  mlir::OperationState call(run.getLoc(),
                            SourceMethodCallOp::getOperationName());
  call.addOperands(run.getBody().front().getArgument(0));
  call.addAttribute("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Owner",
                        {mlir::FlatSymbolRefAttr::get(&context, "tree0")}));
  call.addAttribute("instance_path", builder.getArrayAttr({}));
  call.addAttribute("occurrence", occurrence("Owner", 42));
  builder.create(call);
  auto analysis = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  ASSERT_NE(analysis->lookup(run), nullptr);
  auto before = identities(*file);
  auto text = print(file->getOperation());
  HardwareSourceCheckLimits preparationLimits;
  preparationLimits.maxWorkUnits = 1024 * 1024;
  detail::SourceCheckBudget preparationBudget(preparationLimits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*file, *analysis,
                                                    preparationBudget);
  });
  ASSERT_TRUE(mlir::succeeded(prepared)) << diagnostics;
  HardwareSourceCheckLimits successfulLimits;
  successfulLimits.maxWorkUnits = 1024 * 1024;
  detail::SourceCheckBudget successfulBudget(successfulLimits);
  auto successful = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        successfulBudget);
  });
  ASSERT_TRUE(mlir::succeeded(successful)) << diagnostics;
  llvm::SmallVector<std::tuple<size_t, size_t, size_t>> planSizes;
  for (const auto &transaction : prepared->transactions)
    planSizes.emplace_back(transaction.plan->bodyChecks.size(),
                           transaction.plan->observations.size(),
                           transaction.plan->events.size());
  // The same admitted preparation must fail atomically when the caller gives
  // materialization an explicitly insufficient independent budget.
  HardwareSourceCheckLimits materializationLimits;
  materializationLimits.maxWorkUnits = 1;
  detail::SourceCheckBudget materializationBudget(materializationLimits);
  auto candidate = capture([&] {
    return acir::detail::materializeTransactionLowering(*prepared,
                                                        materializationBudget);
  });
  EXPECT_TRUE(mlir::failed(candidate));
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
  EXPECT_NE(diagnostics.find("transaction clone/materialization"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(diagnostics.find("sampled effect lowering"), std::string::npos)
      << diagnostics;
  for (auto [index, transaction] : llvm::enumerate(prepared->transactions))
    EXPECT_EQ(planSizes[index],
              std::make_tuple(transaction.plan->bodyChecks.size(),
                              transaction.plan->observations.size(),
                              transaction.plan->events.size()));
  unchanged(*file, before);
  EXPECT_EQ(print(file->getOperation()), text);
}

TEST_F(TransactionLoweringTest,
       StateFreeMethodsAndTransactionsAllowMultipleValidStorageImports) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto &block = definition.getBody().front();
  block.getTerminator()->setOperands(
      {block.getArgument(0), block.getArgument(0), block.getArgument(0),
       block.getArgument(0)});
  std::vector<mlir::Operation *> erase;
  for (auto &op : block)
    if (mlir::isa<SourceTransactionOp, SourceMethodOp, SourceStateOp>(&op))
      erase.push_back(&op);
  auto uncalled = named<SourceMethodOp>(*file, "uncalled");
  auto *noop = uncalled->clone();
  noop->setAttr("sym_name", builder.getStringAttr("noop"));
  noop->setAttr("occurrence", occurrence("Owner", 41));
  noop->setOperand(0, (*block.getOps<BitsConstantOp>().begin()).getResult());
  for (auto *op : llvm::reverse(erase))
    op->erase();
  block.getOperations().insert(block.getTerminator()->getIterator(), noop);
  auto imported = *file->getOps<ModuleImportOp>().begin();
  auto *copy = imported->clone();
  copy->setAttr("sym_name", builder.getStringAttr("storage_second"));
  auto bit = block.getArgument(1).getType();
  auto payload = TypeParamType::get(
      &context, mlir::FlatSymbolRefAttr::get(&context, "storage_second"),
      builder.getStringAttr("T"));
  copy->setAttr("function_type",
                mlir::TypeAttr::get(builder.getFunctionType(
                    {bit, bit, bit, payload, payload}, {payload})));
  file->getBody()->getOperations().insert(imported->getIterator(), copy);
  builder.setInsertionPoint(block.getTerminator());
  mlir::OperationState txn(definition.getLoc(),
                           SourceTransactionOp::getOperationName());
  txn.addOperands({block.getArgument(1), block.getArgument(2),
                   (*block.getOps<BitsConstantOp>().begin()).getResult()});
  txn.addAttribute("name", builder.getStringAttr("stateless"));
  txn.addAttribute("declaration", occurrence("Owner", 42));
  txn.addAttribute("occurrence", occurrence("Owner", 43));
  txn.addAttribute("ac.guard_location", noop->getAttr("ac.guard_location"));
  txn.addRegion();
  txn.addRegion();
  auto *transaction = builder.create(txn);
  for (unsigned region = 0; region < 2; ++region) {
    auto *local = new mlir::Block;
    transaction->getRegion(region).push_back(local);
    auto path = local->addArgument(bit, transaction->getLoc());
    builder.setInsertionPointToEnd(local);
    if (region == 1) {
      mlir::OperationState call(transaction->getLoc(),
                                SourceMethodCallOp::getOperationName());
      call.addOperands(path);
      call.addAttribute("callee",
                        mlir::SymbolRefAttr::get(
                            &context, "Owner",
                            {mlir::FlatSymbolRefAttr::get(&context, "noop")}));
      call.addAttribute("instance_path", builder.getArrayAttr({}));
      call.addAttribute("occurrence", occurrence("Owner", 44));
      builder.create(call);
    }
    mlir::OperationState yield(transaction->getLoc(),
                               YieldOp::getOperationName());
    if (region == 0)
      yield.addOperands(path);
    builder.create(yield);
  }
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  EXPECT_EQ(std::distance(file->getOps<ModuleImportOp>().begin(),
                          file->getOps<ModuleImportOp>().end()),
            2);
  EXPECT_TRUE(owner(*file).getBody().front().getOps<InstanceOp>().empty());
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       HardwareOnlyMergedInputDoesNotInheritTransactionWorkBudget) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto &block = definition.getBody().front();
  block.getTerminator()->setOperands(
      {block.getArgument(0), block.getArgument(0), block.getArgument(0),
       block.getArgument(0)});
  std::vector<mlir::Operation *> erase;
  for (auto &op : block)
    if (mlir::isa<SourceTransactionOp, SourceMethodOp, SourceStateOp>(&op))
      erase.push_back(&op);
  for (auto *op : llvm::reverse(erase))
    op->erase();
  // Opaque, semantically inert spelling exceeds the state-only 1Mi budget.
  // Existing native/hardware legality, rather than state analysis, owns this
  // hardware package. This fixture does not create a million physical nodes.
  auto payload = builder.getStringAttr(std::string(1024 * 1024 + 1, 'H'));
  definition->setAttr("test_hardware_payload", payload);
  ASSERT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  EXPECT_EQ(owner(*file)->getAttr("test_hardware_payload"), payload);
  EXPECT_EQ(file->getOperation()
                ->getAttrOfType<mlir::StringAttr>("ac.stage")
                .getValue(),
            "final");
}

class PrimeTransactionAnalysesPass
    : public mlir::PassWrapper<PrimeTransactionAnalysesPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(PrimeTransactionAnalysesPass)
  explicit PrimeTransactionAnalysesPass(bool *valid) : valid(valid) {}
  PrimeTransactionAnalysesPass(const PrimeTransactionAnalysesPass &other)
      : PassWrapper(other), valid(other.valid) {}
  void runOnOperation() override {
    *valid = getAnalysis<SourceTransactionStateAnalysis>().isValid() &&
             getAnalysis<SourceMethodSummariesAnalysis>().isValid();
    markAllAnalysesPreserved();
  }

private:
  bool *valid;
};
class CheckTransactionInvalidationPass
    : public mlir::PassWrapper<CheckTransactionInvalidationPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CheckTransactionInvalidationPass)
  explicit CheckTransactionInvalidationPass(bool *invalidated)
      : invalidated(invalidated) {}
  CheckTransactionInvalidationPass(
      const CheckTransactionInvalidationPass &other)
      : PassWrapper(other), invalidated(other.invalidated) {}
  void runOnOperation() override {
    auto manager = getAnalysisManager();
    *invalidated =
        !manager.getCachedAnalysis<SourceTransactionStateAnalysis>()
             .has_value() &&
        !manager.getCachedAnalysis<SourceMethodSummariesAnalysis>().has_value();
    markAllAnalysesPreserved();
  }

private:
  bool *invalidated;
};
TEST_F(TransactionLoweringTest,
       SuccessfulCommitInvalidatesBorrowedAnalysesAndRoundtripsFinalIR) {
  auto file = parse();
  ASSERT_TRUE(file);
  bool primed = false, invalidated = false;
  auto result = capture([&] {
    mlir::PassManager manager(&context);
    manager.addPass(std::make_unique<PrimeTransactionAnalysesPass>(&primed));
    manager.addPass(acir::createRouteTransactionsPass());
    manager.addPass(acir::createLowerTransactionsPass());
    manager.addPass(
        std::make_unique<CheckTransactionInvalidationPass>(&invalidated));
    return manager.run(*file);
  });
  ASSERT_TRUE(mlir::succeeded(result)) << diagnostics;
  EXPECT_TRUE(primed);
  EXPECT_TRUE(invalidated);
  auto restored = mlir::parseSourceString<mlir::ModuleOp>(
      print(file->getOperation(), true), &context);
  ASSERT_TRUE(restored);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*restored);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       IdentityExtractsForwardLiteralFormalLexicalAndCrossCallPredicateFacts) {
  for (unsigned edges : {1u, 3u}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
    auto no = *std::next(
        definition.getBody().front().getOps<BitsConstantOp>().begin());
    auto identity =
        extractChain(builder, yes.getResult(), no, no.getValue(), edges);
    for (auto method : definition.getBody().front().getOps<SourceMethodOp>())
      if (method.getSymName() != "uncalled")
        method->setOperand(0, identity);
    for (auto transaction :
         definition.getBody().front().getOps<SourceTransactionOp>())
      transaction->setOperand(2, identity);
    auto original = named<SourceMethodOp>(*file, "uncalled");
    auto *copy = original->clone();
    auto predicate = mlir::cast<SourceMethodOp>(copy);
    predicate->setAttr("sym_name", builder.getStringAttr("predicate"));
    predicate->setAttr("occurrence", occurrence("Owner", 40));
    predicate->setOperands(mlir::ValueRange{});
    auto bit = yes.getResult().getType();
    predicate->setAttr("function_type",
                       mlir::TypeAttr::get(builder.getFunctionType({bit}, {})));
    predicate->setAttr("argument_names",
                       builder.getArrayAttr({builder.getStringAttr("allow")}));
    auto parameter = mlir::cast<mlir::DictionaryAttr>(
        named<SourceMethodOp>(*file, "bump")
            ->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
    mlir::NamedAttrList fields(parameter);
    fields.set("name", builder.getStringAttr("allow"));
    fields.set(
        "constraint",
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
             builder.getNamedAttr("type", mlir::TypeAttr::get(bit)),
             builder.getNamedAttr("source_kind",
                                  builder.getStringAttr("fixed_bits"))}));
    fields.set("origin", occurrence("Owner", 40));
    predicate->setAttr("ac.parameters",
                       builder.getArrayAttr({fields.getDictionary(&context)}));
    definition.getBody().front().getOperations().insert(original->getIterator(),
                                                        copy);
    auto guardValue = extractChain(
        builder, predicate.getGuard().front().getArgument(0),
        predicate.getGuard().front().getTerminator(), no.getValue(), edges);
    predicate.getGuard().front().getTerminator()->setOperand(0, guardValue);
    // Captures and caller actuals have separate identities, despite forwarding
    // the same one-bit predicate through the same native operation contract.
    auto run = named<SourceTransactionOp>(*file, "run");
    auto actual = extractChain(builder, run.getBody().front().getArgument(0),
                               run.getBody().front().getTerminator(),
                               no.getValue(), edges);
    builder.setInsertionPoint(run.getBody().front().getTerminator());
    mlir::OperationState call(run.getLoc(),
                              SourceMethodCallOp::getOperationName());
    call.addOperands({run.getBody().front().getArgument(0), actual});
    call.addAttribute(
        "callee", mlir::SymbolRefAttr::get(
                      &context, "Owner",
                      {mlir::FlatSymbolRefAttr::get(&context, "predicate")}));
    call.addAttribute("instance_path", builder.getArrayAttr({}));
    call.addAttribute("occurrence", occurrence("Owner", 41));
    builder.create(call);
    for (auto method : definition.getBody().front().getOps<SourceMethodOp>()) {
      if (method.getSymName() == "uncalled" || method == predicate)
        continue;
      auto *yield = method.getGuard().front().getTerminator();
      yield->setOperand(0, extractChain(builder, yield->getOperand(0), yield,
                                        no.getValue(), edges));
    }
    auto summaries = capture([&] {
      return std::make_unique<SourceMethodSummariesAnalysis>(
          file->getOperation());
    });
    ASSERT_TRUE(summaries->isValid()) << diagnostics;
    EXPECT_TRUE(summaries->isTotal(run));
    auto published =
        capture([&] { return summaries->getPublicationSummary(definition); });
    ASSERT_TRUE(mlir::succeeded(published)) << diagnostics;
    mlir::DictionaryAttr predicateRow;
    for (auto raw : published->getAs<mlir::ArrayAttr>("methods")) {
      auto row = mlir::cast<mlir::DictionaryAttr>(raw);
      if (row.getAs<mlir::StringAttr>("name").getValue() == "predicate")
        predicateRow = row;
    }
    ASSERT_TRUE(predicateRow);
    auto fact = predicateRow.getAs<mlir::DictionaryAttr>("guard")
                    .getAs<mlir::DictionaryAttr>("fact");
    EXPECT_EQ(fact.getAs<mlir::StringAttr>("kind").getValue(), "argument");
    EXPECT_EQ(fact.getAs<mlir::IntegerAttr>("ordinal").getInt(), 0);
    auto state = capture([&] {
      return std::make_unique<SourceTransactionStateAnalysis>(
          file->getOperation());
    });
    ASSERT_TRUE(state->isValid()) << diagnostics;
    auto plan = state->lookup(run);
    ASSERT_NE(plan, nullptr);
    for (auto &guard : plan->guards) {
      ASSERT_TRUE(guard.predicateFact);
      EXPECT_TRUE(guard.predicateFact.getValue());
    }
    ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  }
}

TEST_F(TransactionLoweringTest,
       DynamicGuardDoesNotAcquireLiteralProofAndLowersWithSampling) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
  auto no =
      *std::next(definition.getBody().front().getOps<BitsConstantOp>().begin());
  auto wide = *std::next(
      definition.getBody().front().getOps<BitsConstantOp>().begin(), 2);
  auto *before = definition.getBody().front().getTerminator();
  auto narrow = extractChain(builder, wide.getResult(), before, no.getValue(),
                             1, yes.getResult().getType());
  auto nonzero = extractChain(builder, wide.getResult(), before, yes.getValue(),
                              1, yes.getResult().getType());
  auto multi = extractChain(builder, wide.getResult(), before, no.getValue());
  builder.setInsertionPoint(before);
  mlir::OperationState computation(before->getLoc(),
                                   BitsBinaryOp::getOperationName());
  computation.addOperands({yes.getResult(), no.getResult()});
  computation.addTypes(yes.getResult().getType());
  computation.addAttribute("opcode", builder.getStringAttr("or"));
  mlir::Value expression = builder.create(computation)->getResult(0);
  HardwareAnalysis hardware(*file);
  for (auto value : {narrow, nonzero, multi, expression}) {
    auto proof = capture(
        [&] { return detail::getSourceLiteralPredicate(value, hardware); });
    ASSERT_TRUE(mlir::succeeded(proof)) << diagnostics;
    EXPECT_FALSE(*proof);
  }
  auto method = named<SourceMethodOp>(*file, "uncalled");
  method->setOperand(0, narrow);
  auto watch = named<SourceTransactionOp>(*file, "watch");
  builder.setInsertionPoint(watch.getBody().front().getTerminator());
  mlir::OperationState call(watch.getLoc(),
                            SourceMethodCallOp::getOperationName());
  call.addOperands(watch.getBody().front().getArgument(0));
  call.addAttribute("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Owner",
                        {mlir::FlatSymbolRefAttr::get(&context, "uncalled")}));
  call.addAttribute("instance_path", builder.getArrayAttr({}));
  call.addAttribute("occurrence", occurrence("Owner", 40));
  builder.create(call);
  auto summaries = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(summaries->isValid()) << diagnostics;
  EXPECT_FALSE(summaries->isTotal(watch));
  auto publication =
      capture([&] { return summaries->getPublicationSummary(definition); });
  ASSERT_TRUE(mlir::succeeded(publication)) << diagnostics;
  auto state = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(state->isValid()) << diagnostics;
  auto plan = state->lookup(watch);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->guards.size(), 2u);
  EXPECT_FALSE(plan->guards.back().predicateFact);
  ASSERT_TRUE(mlir::succeeded(lower(*file))) << diagnostics;
  auto lowered = named<RuleOp>(*file, "watch");
  auto admission =
      lowered->getAttrOfType<mlir::DictionaryAttr>("ac.sampled_admission");
  ASSERT_TRUE(admission);
  auto guards = admission.getAs<mlir::ArrayAttr>("controls");
  ASSERT_TRUE(guards);
  EXPECT_EQ(guards.size(), 2u);
  EXPECT_EQ(
      lowered->getAttrOfType<mlir::ArrayAttr>("ac.required_checks").size(), 2u);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return verifyHardwarePackage(*file);
  }))) << diagnostics;
}

TEST_F(TransactionLoweringTest,
       PredicateIdentityDepthAndCyclesFailBoundedlyWithoutPartialStatePlans) {
  for (bool cycle : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
    auto no = *std::next(
        definition.getBody().front().getOps<BitsConstantOp>().begin());
    auto identity = extractChain(builder, yes.getResult(), no, no.getValue(),
                                 cycle ? 1 : 140);
    if (cycle)
      identity.getDefiningOp()->setOperand(0, identity);
    HardwareAnalysis hardware(*file);
    auto literal = capture(
        [&] { return detail::getSourceLiteralPredicate(identity, hardware); });
    EXPECT_TRUE(mlir::failed(literal));
    EXPECT_TRUE(diagnostics.find("depth") != std::string::npos ||
                diagnostics.find("cycle") != std::string::npos ||
                diagnostics.find("cyclic") != std::string::npos)
        << diagnostics;
    named<SourceMethodOp>(*file, "bump")->setOperand(0, identity);
    auto before = identities(*file);
    auto state = capture([&] {
      return std::make_unique<SourceTransactionStateAnalysis>(
          file->getOperation());
    });
    EXPECT_FALSE(state->isValid());
    EXPECT_EQ(state->lookup(named<SourceTransactionOp>(*file, "run")), nullptr);
    EXPECT_EQ(state->lookup(named<SourceTransactionOp>(*file, "watch")),
              nullptr);
    std::string dump;
    llvm::raw_string_ostream stream(dump);
    EXPECT_TRUE(mlir::failed(state->print(stream)));
    EXPECT_TRUE(dump.empty());
    unchanged(*file, before);
  }
}

TEST_F(TransactionLoweringTest,
       IdentityExtractDoesNotMakeAComputedClockTheOriginalPhysicalWire) {
  for (bool literalEndpoint : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto definition = owner(*file);
    auto state = first<SourceStateOp>(definition.getBody());
    auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
    auto no = *std::next(
        definition.getBody().front().getOps<BitsConstantOp>().begin());
    mlir::Value endpoint = state.getClk();
    if (literalEndpoint)
      endpoint = yes.getResult();
    auto clock = extractChain(builder, endpoint, state, no.getValue(), 1);
    state->setOperand(0, clock);
    ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
        << diagnostics;
    if (literalEndpoint) {
      HardwareAnalysis hardware(*file);
      auto predicate = capture(
          [&] { return detail::getSourceLiteralPredicate(clock, hardware); });
      ASSERT_TRUE(mlir::succeeded(predicate)) << diagnostics;
      ASSERT_TRUE(*predicate);
      EXPECT_TRUE(predicate->getValue());
    }
    auto summaries = capture([&] {
      return std::make_unique<SourceMethodSummariesAnalysis>(
          file->getOperation());
    });
    ASSERT_TRUE(summaries->isValid()) << diagnostics;
    auto publication =
        capture([&] { return summaries->getPublicationSummary(definition); });
    EXPECT_TRUE(mlir::failed(publication));
    EXPECT_NE(diagnostics.find("domain endpoint"), std::string::npos)
        << diagnostics;
    EXPECT_NE(diagnostics.find("direct static literal"), std::string::npos)
        << diagnostics;
    auto before = identities(*file);
    auto plan = capture([&] {
      return std::make_unique<SourceTransactionStateAnalysis>(
          file->getOperation());
    });
    EXPECT_FALSE(plan->isValid());
    EXPECT_EQ(plan->lookup(named<SourceTransactionOp>(*file, "run")), nullptr);
    unchanged(*file, before);
  }
}

class SourceLinkStageTest : public TransactionLoweringTest {
protected:
  mlir::DictionaryAttr sourceOwner(llvm::StringRef path = "lower.py") {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("package", builder.getStringAttr("")),
         builder.getNamedAttr("path", builder.getStringAttr(path))});
  }
  mlir::OwningOpRef<mlir::ModuleOp> sourceBody(bool canonical = true) {
    auto text = loweringFixture();
    auto qualify = [&](llvm::StringRef from, llvm::StringRef to) {
      size_t at = 0;
      while ((at = text.find(from.str(), at)) != std::string::npos) {
        text.replace(at, from.size(), to.str());
        at += to.size();
      }
    };
    qualify("@Owner", "@lower.Owner");
    qualify("sym_name = \"Owner\"", "sym_name = \"lower.Owner\"");
    qualify("@storage", "@gfsim.dffe.storage");
    qualify("sym_name = \"storage\"", "sym_name = \"gfsim.dffe.storage\"");
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, false));
    EXPECT_TRUE(file);
    if (!file)
      return {};
    (*file->getOps<SystemOp>().begin())->erase();
    (*file)->removeAttr("ac.source_units");
    if (canonical) {
      auto registry = capture([&] {
        return compiler::SourceHeaderRegistry::create(&context, {}, [&] {
          return mlir::emitError(builder.getUnknownLoc());
        });
      });
      EXPECT_TRUE(mlir::succeeded(registry)) << diagnostics;
      if (mlir::failed(registry))
        return {};
      auto primitive = registry->lookupBuiltin("dffe");
      EXPECT_TRUE(primitive);
      if (!primitive)
        return {};
      auto old = *file->getOps<ModuleImportOp>().begin();
      file->getBody()->getOperations().insert(old->getIterator(),
                                              primitive.declaration->clone());
      old->erase();
    }
    file->getOperation()->setAttr("ac.stage", builder.getStringAttr("source"));
    file->getOperation()->setAttr("ac.unit_kind",
                                  builder.getStringAttr("implementation"));
    file->getOperation()->setAttr("ac.source_owner", sourceOwner());
    file->getOperation()->setAttr("ac.import_bindings",
                                  builder.getArrayAttr({}));
    auto definition = owner(*file);
    definition->setAttr("ac.origin", occurrence("lower.Owner", 0));
    definition->setAttr("ac.declaration_role",
                        builder.getStringAttr("definition"));
    auto imported = *file->getOps<ModuleImportOp>().begin();
    imported->setAttr("ac.declaration_role",
                      builder.getStringAttr("import_snapshot"));
    if (!canonical)
      imported->setAttr("ac.origin", occurrence("gfsim.dffe.storage", 0));
    file->getOperation()->setAttr("ac.interfaces",
                                  builder.getArrayAttr({sourceOwner()}));
    auto span = builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("lower.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
    auto site = builder.getDictionaryAttr(
        {builder.getNamedAttr("ast_path", builder.getArrayAttr({})),
         builder.getNamedAttr("location", span)});
    file->getOperation()->setAttr(
        "ac.exports",
        builder.getArrayAttr({builder.getDictionaryAttr(
            {builder.getNamedAttr("name", builder.getStringAttr("Owner")),
             builder.getNamedAttr("target", mlir::FlatSymbolRefAttr::get(
                                                &context, "lower.Owner")),
             builder.getNamedAttr("site", site)})}));
    return file;
  }
  mlir::OwningOpRef<mlir::ModuleOp> header(mlir::ModuleOp body) {
    mlir::OwningOpRef<mlir::ModuleOp> result(
        mlir::cast<mlir::ModuleOp>(body->clone()));
    auto status = capture([&] {
      mlir::PassManager manager(&context);
      manager.addPass(acir::createExtractSourceInterfacePass());
      return manager.run(*result);
    });
    EXPECT_TRUE(mlir::succeeded(status)) << diagnostics;
    if (mlir::failed(status))
      return {};
    return result;
  }
  auto link(mlir::ModuleOp body, mlir::ModuleOp interface,
            compiler::SourceLinkStages *stages,
            compiler::SourceLinkTarget target =
                compiler::SourceLinkTarget::Hardware) {
    return capture([&] {
      return compiler::linkSourceUnits(
          {{body, interface}}, "lower.Owner",
          [&] { return mlir::emitError(builder.getUnknownLoc()); }, target,
          stages);
    });
  }
};

TEST_F(SourceLinkStageTest,
       SourceBodiesAndPublishedInterfacesRejectForgedFinalPortAuthority) {
  for (bool interface : {false, true}) {
    for (bool imported : {false, true}) {
      SCOPED_TRACE(interface);
      SCOPED_TRACE(imported);
      auto body = sourceBody();
      ASSERT_TRUE(body);
      auto unit = interface ? header(*body) : std::move(body);
      ASSERT_TRUE(unit);
      auto valid = capture([&] {
        auto error = [&] { return mlir::emitError(builder.getUnknownLoc()); };
        return interface ? verifySourceInterfaceStructure(*unit, error)
                         : verifySourceBodyStructure(*unit, error);
      });
      ASSERT_TRUE(mlir::succeeded(valid)) << diagnostics;
      auto *target =
          imported ? (*unit->getOps<ModuleImportOp>().begin()).getOperation()
                   : mlir::SymbolTable::lookupSymbolIn(*unit, "lower.Owner");
      ASSERT_NE(target, nullptr);
      target->setAttr(
          "ac.internal_ports",
          builder.getDictionaryAttr(
              {builder.getNamedAttr("source_inputs",
                                    builder.getI64IntegerAttr(3)),
               builder.getNamedAttr("source_outputs",
                                    builder.getI64IntegerAttr(4)),
               builder.getNamedAttr("root_inputs", builder.getArrayAttr({}))}));
      auto before = identities(*unit);
      auto text = print(unit->getOperation());
      auto result = capture([&] {
        auto error = [&] { return mlir::emitError(builder.getUnknownLoc()); };
        return interface ? verifySourceInterfaceStructure(*unit, error)
                         : verifySourceBodyStructure(*unit, error);
      });
      EXPECT_TRUE(mlir::failed(result));
      EXPECT_NE(diagnostics.find("internal_ports"), std::string::npos)
          << diagnostics;
      unchanged(*unit, before);
      EXPECT_EQ(print(unit->getOperation()), text);
    }
  }
}

TEST_F(SourceLinkStageTest,
       CapturedStagesAreRealBoundariesOfTheSameSuccessfulLink) {
  auto body = sourceBody();
  ASSERT_TRUE(body);
  auto interface = header(*body);
  ASSERT_TRUE(interface);
  auto before = identities(*body);
  compiler::SourceLinkStages stages{"input sentinel", "routed sentinel",
                                    "output sentinel"};
  auto linked = link(*body, *interface, &stages);
  ASSERT_TRUE(mlir::succeeded(linked)) << diagnostics;
  EXPECT_NE(stages.beforeTransactions.find("ac.transaction"),
            std::string::npos);
  EXPECT_EQ(stages.beforeTransactions.find("ac.stage = \"final\""),
            std::string::npos);
  auto routedStage = mlir::parseSourceString<mlir::ModuleOp>(
      stages.routedTransactions, mlir::ParserConfig(&context, false));
  ASSERT_TRUE(routedStage);
  EXPECT_TRUE(mlir::succeeded(verifyRoutedPackage(*routedStage)));
  EXPECT_EQ(
      (*routedStage)->getAttrOfType<mlir::StringAttr>("ac.stage").getValue(),
      "routed");
  auto loweredStage = mlir::parseSourceString<mlir::ModuleOp>(
      stages.afterTransactions, mlir::ParserConfig(&context, false));
  ASSERT_TRUE(loweredStage);
  unsigned sourceTransactions = 0;
  loweredStage->walk([&](SourceTransactionOp) { ++sourceTransactions; });
  EXPECT_EQ(sourceTransactions, 0u);
  EXPECT_NE(stages.afterTransactions.find("ac.stage = \"final\""),
            std::string::npos);
  EXPECT_EQ(stages.afterTransactions, print((*linked)->getOperation(), true));
  auto ordinary = link(*body, *interface, nullptr);
  ASSERT_TRUE(mlir::succeeded(ordinary)) << diagnostics;
  EXPECT_EQ(print((*ordinary)->getOperation(), true), stages.afterTransactions);
  unchanged(*body, before);
  EXPECT_EQ(body->getOperation()
                ->getAttrOfType<mlir::StringAttr>("ac.stage")
                .getValue(),
            "source");
}

TEST_F(SourceLinkStageTest,
       FailedLoweringPreservesStageSentinelsAndSourceInputs) {
  auto body = sourceBody();
  ASSERT_TRUE(body);
  auto write = first<SourceStateWriteOp>(
      named<SourceTransactionOp>(*body, "watch").getBody());
  write->setAttr("state", builder.getStringAttr("count"));
  auto interface = header(*body);
  ASSERT_TRUE(interface);
  auto before = identities(*body), headerBefore = identities(*interface);
  compiler::SourceLinkStages stages{"input sentinel", "routed sentinel",
                                    "output sentinel"};
  auto admitted = capture([&] {
    return compiler::admitSourceLinkUnits({{*body, *interface}}, [&] {
      return mlir::emitError(builder.getUnknownLoc());
    });
  });
  ASSERT_TRUE(mlir::succeeded(admitted)) << diagnostics;
  auto linked = link(*body, *interface, &stages);
  EXPECT_TRUE(mlir::failed(linked)) << diagnostics;
  EXPECT_NE(diagnostics.find("multiple outer writers"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(stages.beforeTransactions, "input sentinel");
  EXPECT_EQ(stages.routedTransactions, "routed sentinel");
  EXPECT_EQ(stages.afterTransactions, "output sentinel");
  unchanged(*body, before);
  unchanged(*interface, headerBefore);
}

TEST_F(
    SourceLinkStageTest,
    PublicLinkRejectsSchemaValidNoncanonicalPrimitiveWithoutPublishingStages) {
  auto body = sourceBody(false);
  ASSERT_TRUE(body);
  mlir::OwningOpRef<mlir::ModuleOp> interface(
      mlir::cast<mlir::ModuleOp>(body->clone()));
  auto extracted = capture([&] {
    mlir::PassManager manager(&context);
    manager.addPass(acir::createExtractSourceInterfacePass());
    return manager.run(*interface);
  });
  // Header extraction itself may reject this primitive's absent authority.
  if (mlir::failed(extracted)) {
    EXPECT_NE(diagnostics.find("primitive"), std::string::npos) << diagnostics;
    EXPECT_EQ(diagnostics.find("symbol does not match"), std::string::npos);
    return;
  }
  auto before = identities(*body), headerBefore = identities(*interface);
  compiler::SourceLinkStages stages{"input sentinel", "routed sentinel",
                                    "output sentinel"};
  EXPECT_TRUE(mlir::failed(link(*body, *interface, &stages)));
  EXPECT_NE(diagnostics.find("primitive"), std::string::npos) << diagnostics;
  EXPECT_EQ(diagnostics.find("symbol does not match"), std::string::npos);
  EXPECT_EQ(stages.beforeTransactions, "input sentinel");
  EXPECT_EQ(stages.routedTransactions, "routed sentinel");
  EXPECT_EQ(stages.afterTransactions, "output sentinel");
  unchanged(*body, before);
  unchanged(*interface, headerBefore);
}

TEST_F(SourceLinkStageTest,
       RoutedTargetPublishesRoutedSnapshotWithoutFinalStage) {
  std::string saved;
  {
    auto body = sourceBody();
    ASSERT_TRUE(body);
    auto interface = header(*body);
    ASSERT_TRUE(interface);
    auto bodyBefore = identities(*body), headerBefore = identities(*interface);
    const auto bodyText = print(body->getOperation());
    const auto headerText = print(interface->getOperation());
    compiler::SourceLinkStages stages{"before sentinel", "routed sentinel",
                                      "final sentinel"};
    auto linked =
        link(*body, *interface, &stages, compiler::SourceLinkTarget::Routed);
    ASSERT_TRUE(mlir::succeeded(linked)) << diagnostics;
    ASSERT_TRUE(mlir::succeeded(verifyRoutedPackage(**linked)));
    EXPECT_EQ((*linked)
                  ->getOperation()
                  ->getAttrOfType<mlir::StringAttr>("ac.stage")
                  .getValue(),
              "routed");
    EXPECT_NE(stages.beforeTransactions.find("ac.transaction"),
              std::string::npos);
    EXPECT_EQ(stages.routedTransactions,
              print((*linked)->getOperation(), true));
    EXPECT_TRUE(stages.afterTransactions.empty());
    const auto routedBefore = identities(**linked);
    for (unsigned repeat = 0; repeat < 2; ++repeat) {
      SCOPED_TRACE(repeat);
      auto rerouted = capture([&] {
        mlir::PassManager manager(&context);
        manager.addPass(acir::createRouteTransactionsPass());
        return manager.run(**linked);
      });
      ASSERT_TRUE(mlir::succeeded(rerouted)) << diagnostics;
      EXPECT_EQ(stages.routedTransactions,
                print((*linked)->getOperation(), true));
      unchanged(**linked, routedBefore);
      EXPECT_TRUE(mlir::succeeded(verifyRoutedPackage(**linked)));
    }
    auto withoutRetention =
        link(*body, *interface, nullptr, compiler::SourceLinkTarget::Routed);
    ASSERT_TRUE(mlir::succeeded(withoutRetention)) << diagnostics;
    EXPECT_EQ(stages.routedTransactions,
              print((*withoutRetention)->getOperation(), true));
    unchanged(*body, bodyBefore);
    unchanged(*interface, headerBefore);
    EXPECT_EQ(print(body->getOperation()), bodyText);
    EXPECT_EQ(print(interface->getOperation()), headerText);
    saved = stages.routedTransactions;
  }
  // Source units and linked operations are gone; no producer-side preparation
  // object or source registry is supplied to the standalone consumer.
  mlir::MLIRContext fresh;
  fresh.loadDialect<ACIRDialect>();
  auto restored = mlir::parseSourceString<mlir::ModuleOp>(saved, &fresh);
  ASSERT_TRUE(restored);
  EXPECT_TRUE(mlir::succeeded(verifyRoutedPackage(*restored)));
  EXPECT_EQ(print(restored->getOperation(), true), saved);
}

TEST_F(SourceLinkStageTest,
       FailedRoutedTargetPreservesAllStagesAndSourceInputs) {
  auto body = sourceBody();
  ASSERT_TRUE(body);
  auto write = first<SourceStateWriteOp>(
      named<SourceTransactionOp>(*body, "watch").getBody());
  ASSERT_TRUE(write);
  write->setAttr("state", builder.getStringAttr("count"));
  auto interface = header(*body);
  ASSERT_TRUE(interface);
  const auto bodyText = print(body->getOperation());
  const auto headerText = print(interface->getOperation());
  auto bodyBefore = identities(*body), headerBefore = identities(*interface);
  compiler::SourceLinkStages stages{"before sentinel", "routed sentinel",
                                    "final sentinel"};
  auto linked =
      link(*body, *interface, &stages, compiler::SourceLinkTarget::Routed);
  EXPECT_TRUE(mlir::failed(linked));
  EXPECT_NE(diagnostics.find("multiple outer writers"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(stages.beforeTransactions, "before sentinel");
  EXPECT_EQ(stages.routedTransactions, "routed sentinel");
  EXPECT_EQ(stages.afterTransactions, "final sentinel");
  unchanged(*body, bodyBefore);
  unchanged(*interface, headerBefore);
  EXPECT_EQ(print(body->getOperation()), bodyText);
  EXPECT_EQ(print(interface->getOperation()), headerText);
}

} // namespace
} // namespace acir::ac
