#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "Dialect/ACIR/SourceTransactionSummary.h"
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
#include <memory>
#include <string>
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
    }) {sym_name = "bump", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #o4, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #o4} : (!b1, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %nested = "ac.method.call"(%path, %delta) {callee = @Owner::@bump, instance_path = [], occurrence = #o9} : (!b1, !b8) -> !b8
      "ac.yield"(%nested) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #o8, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #o8} : (!b1) -> ()
    "ac.method"(%rst) ({
    ^bb0(%admit: !b1):
      "ac.yield"(%admit) : (!b1) -> ()
    }, {
    ^bb0(%admit: !b1):
      "ac.yield"() : () -> ()
    }) {sym_name = "uncalled", function_type = () -> (), argument_names = [], ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", occurrence = #o10} : (!b1) -> ()
    %first, %last = "ac.transaction"(%clk, %rst, %yes, %seed) ({
    ^bb0(%path: !b1, %caller_bias: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %caller_bias: !b8):
      %first = "ac.method.call"(%path, %caller_bias) {callee = @Owner::@bump, instance_path = [], occurrence = #o13} : (!b1, !b8) -> !b8
      %last = "ac.method.call"(%path, %first) {callee = @Owner::@relay, instance_path = [], occurrence = #o14} : (!b1, !b8) -> !b8
      "ac.state.write"(%path, %first) {state = "saved", selectors = [], occurrence = #o15} : (!b1, !b8) -> ()
      "ac.yield"(%first, %last) : (!b8, !b8) -> ()
    }) {name = "run", declaration = #o11, occurrence = #o12} : (!b1, !b1, !b1, !b8) -> (!b8, !b8)
    "ac.transaction"(%clk, %rst, %yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #o18} : (!b1) -> !b8
      "ac.state.write"(%path, %old) {state = "seen", selectors = [], occurrence = #o19} : (!b1, !b8) -> ()
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #o16, occurrence = #o17} : (!b1, !b1, !b1, !b8) -> ()
    "ac.yield"(%q, %first, %last, %hold) : (!b8, !b8, !b8, !b8) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "lower.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1) -> (!b8, !b8, !b8, !b8), input_names = ["seed", "pyc_clk", "pyc_rst"], output_names = ["count", "first", "last", "hold"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
  "ac.system"() {entry = {callee = @Owner, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
  return text;
}

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
    EXPECT_EQ(captureActual(watch, yielded(seen)), count->getResult(0));
    EXPECT_EQ(hold->getOperand(3), hold->getResult(0));
    auto heldEnable = hold->getOperand(2).getDefiningOp<BitsConstantOp>();
    auto writtenEnable = count->getOperand(2).getDefiningOp<BitsConstantOp>();
    ASSERT_TRUE(heldEnable);
    ASSERT_TRUE(writtenEnable);
    HardwareAnalysis hardware(*file);
    HardwareBindings bindings;
    bindings.owner = owner(*file);
    auto held = capture([&] {
      return hardware.evaluateStatic(heldEnable.getValue(), bindings,
                                     heldEnable);
    });
    auto written = capture([&] {
      return hardware.evaluateStatic(writtenEnable.getValue(), bindings,
                                     writtenEnable);
    });
    ASSERT_TRUE(mlir::succeeded(held));
    ASSERT_TRUE(mlir::succeeded(written));
    auto isTrue = [](mlir::Attribute value) {
      if (auto boolean = mlir::dyn_cast<mlir::BoolAttr>(value))
        return boolean.getValue();
      return mlir::cast<MathIntAttr>(value).getCanonicalValue() == "1";
    };
    EXPECT_FALSE(isTrue(*held));
    EXPECT_TRUE(isTrue(*written));
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
    if (mutation == 4)
      (*file->getOps<ModuleImportOp>().begin())->erase();
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

TEST_F(TransactionLoweringTest,
       FixedBitsAdmissionRejectsWholeOwnerAggregateAndMissingBody) {
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
  rejects(*file);
  auto absent = parse();
  ASSERT_TRUE(absent);
  named<SourceMethodOp>(*absent, "bump").getBody().getBlocks().clear();
  rejects(*absent);
}

TEST_F(TransactionLoweringTest,
       CrossInstanceCallsRemainUnsupportedDespiteValidSamePhysicalDomain) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  mlir::OperationState state(definition.getLoc(), ModuleOp::getOperationName());
  state.addAttributes(definition->getAttrs());
  state.attributes.set("sym_name", builder.getStringAttr("Root"));
  state.attributes.set("function_type",
                       mlir::TypeAttr::get(builder.getFunctionType(
                           definition.getFunctionType().getInputs(),
                           {definition.getFunctionType().getResult(0),
                            definition.getFunctionType().getResult(0)})));
  state.attributes.set("output_names",
                       builder.getArrayAttr({builder.getStringAttr("first"),
                                             builder.getStringAttr("last")}));
  state.addRegion();
  builder.setInsertionPointToEnd(file->getBody());
  auto root = mlir::cast<ModuleOp>(builder.create(state));
  auto *block = new mlir::Block;
  root.getBody().push_back(block);
  mlir::IRMapping mapping;
  for (auto argument : definition.getBody().front().getArguments())
    mapping.map(argument,
                block->addArgument(argument.getType(), root.getLoc()));
  auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
  auto *localYes = yes->clone();
  block->push_back(localYes);
  mapping.map(yes.getResult(), localYes->getResult(0));
  builder.setInsertionPointToEnd(block);
  mlir::OperationState instance(root.getLoc(), InstanceOp::getOperationName());
  instance.addOperands(block->getArguments());
  instance.addTypes(definition.getFunctionType().getResults());
  instance.addAttribute("callee",
                        mlir::FlatSymbolRefAttr::get(&context, "Owner"));
  instance.addAttribute("instance_name", builder.getStringAttr("child"));
  instance.addAttribute("parameters", builder.getArrayAttr({}));
  instance.addAttribute("type_arguments", builder.getArrayAttr({}));
  instance.addAttribute("occurrence", occurrence("Root", 0));
  builder.create(instance);
  auto *copy = named<SourceTransactionOp>(*file, "run")->clone(mapping);
  auto run = mlir::cast<SourceTransactionOp>(copy);
  run->setAttr("declaration", occurrence("Root", 1));
  run->setAttr("occurrence", occurrence("Root", 2));
  block->push_back(copy);
  for (auto call : run.getBody().front().getOps<SourceMethodCallOp>()) {
    call->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr("child")}));
    call->setAttr(
        "occurrence",
        occurrence("Root",
                   call->getResult(0) ==
                           run.getBody().front().getTerminator()->getOperand(0)
                       ? 3
                       : 4));
  }
  for (auto write : llvm::make_early_inc_range(
           run.getBody().front().getOps<SourceStateWriteOp>()))
    write->erase();
  builder.setInsertionPointToEnd(block);
  mlir::OperationState yield(root.getLoc(), YieldOp::getOperationName());
  yield.addOperands(run->getResults());
  builder.create(yield);
  auto proof = capture([&] {
    return std::make_unique<SourceTransactionStateAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(proof->isValid()) << diagnostics;
  rejects(*file);
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

TEST_F(TransactionLoweringTest,
       ExpandedLiveInMappingBudgetFailsAfterSuccessfulStateAnalysis) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto definition = owner(*file);
  auto run = named<SourceTransactionOp>(*file, "run");
  auto seed = definition.getBody().front().getArgument(0);
  constexpr unsigned liveInCount = 600;
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
  llvm::SmallVector<mlir::Value> captures(run->getOperands());
  captures.append(pure->result_begin(), pure->result_end());
  run->setOperands(captures);
  for (auto &region : run->getRegions())
    for (unsigned i = 0; i < liveInCount; ++i)
      region.front().addArgument(seed.getType(), run.getLoc());
  auto yes = *definition.getBody().front().getOps<BitsConstantOp>().begin();
  auto unused = named<SourceMethodOp>(*file, "uncalled");
  auto *noop = unused->clone();
  noop->setAttr("sym_name", builder.getStringAttr("noop"));
  noop->setAttr("occurrence", occurrence("Owner", 41));
  noop->setOperand(0, yes.getResult());
  definition.getBody().front().getOperations().insert(unused->getIterator(),
                                                      noop);
  constexpr unsigned levels = 9;
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
  // The input/state plan is admitted. The extra cost is the complete live-in
  // IRMapping copied for each invocation, not the source call-tree budget.
  rejects(*file, "transaction materialization");
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
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
       PredicateProofDoesNotInferFromNarrowingNonzeroLowOrUnrelatedOperations) {
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
  rejects(*file, "proven-true active guards");
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
            compiler::SourceLinkStages *stages) {
    return capture([&] {
      return compiler::linkHardwareUnits(
          {{body, interface}}, "lower.Owner",
          [&] { return mlir::emitError(builder.getUnknownLoc()); }, stages);
    });
  }
};

TEST_F(SourceLinkStageTest,
       CapturedStagesAreRealBoundariesOfTheSameSuccessfulLink) {
  auto body = sourceBody();
  ASSERT_TRUE(body);
  auto interface = header(*body);
  ASSERT_TRUE(interface);
  auto before = identities(*body);
  compiler::SourceLinkStages stages{"input sentinel", "output sentinel"};
  auto linked = link(*body, *interface, &stages);
  ASSERT_TRUE(mlir::succeeded(linked)) << diagnostics;
  EXPECT_NE(stages.beforeTransactions.find("ac.transaction"),
            std::string::npos);
  EXPECT_EQ(stages.beforeTransactions.find("ac.stage = \"final\""),
            std::string::npos);
  EXPECT_EQ(stages.afterTransactions.find("ac.transaction"), std::string::npos);
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
  compiler::SourceLinkStages stages{"input sentinel", "output sentinel"};
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
  compiler::SourceLinkStages stages{"input sentinel", "output sentinel"};
  EXPECT_TRUE(mlir::failed(link(*body, *interface, &stages)));
  EXPECT_NE(diagnostics.find("primitive"), std::string::npos) << diagnostics;
  EXPECT_EQ(diagnostics.find("symbol does not match"), std::string::npos);
  EXPECT_EQ(stages.beforeTransactions, "input sentinel");
  EXPECT_EQ(stages.afterTransactions, "output sentinel");
  unchanged(*body, before);
  unchanged(*interface, headerBefore);
}

} // namespace
} // namespace acir::ac
