#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>

namespace acir::ac {
namespace {

// Native intermediate-IR contracts only: no source support or execution claim.
const char *counterAndTwoInstances = R"ir(
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#count_state = {site = {definition = @Counter, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#packet_state = {site = {definition = @Counter, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#counter = {site = {definition = @Counter, ast_path = []}, expansion = []}
#left_instance = {site = {definition = @Root, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#right_instance = {site = {definition = @Root, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#root = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  ac.struct "Packet" fields [{name = "lanes", type = !lanes}]
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %seed: !b8, %packet: !ac.struct<"Packet">):
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
    %initial_lanes = "ac.table.splat"(%initial) {shape = [#w2]} : (!b8) -> !lanes
    %initial_packet = "ac.struct.create"(%initial_lanes) : (!lanes) -> !ac.struct<"Packet">
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #count_state} : (!b1, !b1, !b8) -> !b8
    %packet_q = "ac.state"(%clk, %rst, %initial_packet) {instance_name = "packet", occurrence = #packet_state} : (!b1, !b1, !ac.struct<"Packet">) -> !ac.struct<"Packet">
    "ac.method"(%rst) ({
    ^bb0(%delta: !b8, %path: !b1):
      %current = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      %lane = "ac.state.read"(%path, %path) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #counter} : (!b1, !b1) -> !b8
      %next = "ac.bits.binary"(%old, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      %pair = "ac.table.splat"(%next) {shape = [#w2]} : (!b8) -> !lanes
      %mapped = "ac.table.map"(%pair) ({
      ^bb0(%ordinal: !b1, %entry: !b8):
        %twice = "ac.bits.binary"(%entry, %entry) {opcode = "add"} : (!b8, !b8) -> !b8
        "ac.yield"(%twice) : (!b8) -> ()
      }) {shape = [#w2], operandSegmentSizes = array<i32: 1, 0>} : (!lanes) -> !lanes
      "ac.state.write"(%path, %next) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      "ac.state.write"(%path, %lane, %path) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #counter} : (!b1, !b8, !b1) -> ()
      "ac.yield"(%old) : (!b8) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.method"(%rst) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %result = "ac.method.call"(%path, %delta) {callee = @Counter::@add, instance_path = [], occurrence = #counter} : (!b1, !b8) -> !b8
      "ac.yield"(%result) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8, input_names = ["clk", "rst", "seed", "packet"], output_names = ["count"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %seed: !b8, %packet: !ac.struct<"Packet">):
    %left = "ac.instance"(%clk, %rst, %seed, %packet) {instance_name = "left", callee = @Counter, parameters = [], type_arguments = [], occurrence = #left_instance} : (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8
    %right = "ac.instance"(%clk, %rst, %seed, %packet) {instance_name = "right", callee = @Counter, parameters = [], type_arguments = [], occurrence = #right_instance} : (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8
    %answer = "ac.transaction"(%clk, %rst, %rst, %seed) ({
    ^bb0(%path: !b1, %value: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %value: !b8):
      %a = "ac.method.call"(%path, %value) {callee = @Counter::@relay, instance_path = ["left"], occurrence = #root} : (!b1, !b8) -> !b8
      %again = "ac.method.call"(%path, %value) {callee = @Counter::@add, instance_path = ["left"], occurrence = #root} : (!b1, !b8) -> !b8
      %b = "ac.method.call"(%path, %value) {callee = @Counter::@add, instance_path = ["right"], occurrence = #root} : (!b1, !b8) -> !b8
      "ac.yield"(%b) : (!b8) -> ()
    }) {name = "step", declaration = #root, occurrence = #root, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> !b8
    "ac.yield"(%answer) : (!b8) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8, input_names = ["clk", "rst", "seed", "packet"], output_names = ["answer"]} : () -> ()
  "ac.system"() {entry = {callee = @Root, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";

class TransactionEffectsTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OpBuilder builder{&context};
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ACIRDialect>(); }
  mlir::OwningOpRef<mlir::ModuleOp> parse() {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        counterAndTwoInstances,
        mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    EXPECT_TRUE(file);
    return file;
  }
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
  mlir::Operation *find(mlir::ModuleOp file, llvm::StringRef name,
                        unsigned ordinal = 0) {
    mlir::Operation *result = nullptr;
    file.walk([&](mlir::Operation *op) {
      if (op->getName().getStringRef() == name && !ordinal-- && !result)
        result = op;
    });
    EXPECT_NE(result, nullptr) << name.str();
    return result;
  }
  void reject(mlir::ModuleOp file, llvm::StringRef reason) {
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(file); })));
    EXPECT_NE(diagnostics.find(reason.str()), std::string::npos) << diagnostics;
  }
  mlir::DictionaryAttr changed(mlir::DictionaryAttr dictionary,
                               llvm::StringRef name, mlir::Attribute value) {
    mlir::NamedAttrList fields(dictionary);
    if (value)
      fields.set(name, value);
    else
      fields.erase(name);
    return fields.getDictionary(&context);
  }
  mlir::DictionaryAttr parameter(mlir::Operation *method) {
    return mlir::cast<mlir::DictionaryAttr>(
        method->getAttrOfType<mlir::ArrayAttr>("ac.parameters")[0]);
  }
  void setParameter(mlir::Operation *method, mlir::DictionaryAttr value) {
    method->setAttr("ac.parameters", builder.getArrayAttr({value}));
  }
  void rejectsContract(mlir::ModuleOp file) {
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(file); })));
    EXPECT_FALSE(diagnostics.empty());
  }
  template <typename Effect>
  bool hasEffect(mlir::Operation *op, llvm::StringRef resource) {
    auto effects = mlir::getEffectsRecursively(op);
    EXPECT_TRUE(effects.has_value());
    return effects && llvm::any_of(*effects, [&](const auto &effect) {
             return mlir::isa<Effect>(effect.getEffect()) &&
                    effect.getResource()->getName() == resource;
           });
  }
  void retained(mlir::Operation *op) {
    EXPECT_FALSE(mlir::isMemoryEffectFree(op));
    EXPECT_FALSE(mlir::isSpeculatable(op));
    EXPECT_FALSE(mlir::isOpTriviallyDead(op));
  }
  SourceBranchOp branch(mlir::Block &block, mlir::Block::iterator where,
                        mlir::Value condition,
                        std::optional<mlir::Value> whenTrue = std::nullopt,
                        std::optional<mlir::Value> whenFalse = std::nullopt) {
    EXPECT_EQ(whenTrue.has_value(), whenFalse.has_value());
    auto transaction = mlir::dyn_cast<SourceTransactionOp>(block.getParentOp());
    if (!transaction)
      transaction = block.getParentOp()->getParentOfType<SourceTransactionOp>();
    EXPECT_TRUE(transaction);
    mlir::OperationState state(transaction.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(condition);
    if (whenTrue)
      state.addTypes((*whenTrue).getType());
    state.addAttribute("occurrence", transaction->getAttr("occurrence"));
    state.addAttribute("ac.condition_location",
                       transaction->getAttr("ac.guard_location"));
    state.addRegion();
    state.addRegion();
    builder.setInsertionPoint(&block, where);
    auto result = mlir::cast<SourceBranchOp>(builder.create(state));
    for (auto [region, value] : llvm::zip(
             result->getRegions(),
             std::array<std::optional<mlir::Value>, 2>{whenTrue, whenFalse})) {
      auto *arm = new mlir::Block;
      region.push_back(arm);
      builder.setInsertionPointToEnd(arm);
      mlir::OperationState yield(result.getLoc(), YieldOp::getOperationName());
      if (value)
        yield.addOperands(*value);
      builder.create(yield);
    }
    return result;
  }
};

TEST_F(TransactionEffectsTest,
       NativeTypesResolveLocalAndDistinctInstanceOwners) {
  auto file = parse();
  ASSERT_TRUE(file);
  EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto local = find(*file, "ac.method.call");
  EXPECT_TRUE(local->getAttrOfType<mlir::ArrayAttr>("instance_path").empty());
  auto left = find(*file, "ac.method.call", 1);
  auto again = find(*file, "ac.method.call", 2);
  EXPECT_EQ(left->getAttr("instance_path"), again->getAttr("instance_path"));
  auto right = find(*file, "ac.method.call", 3);
  EXPECT_NE(left->getAttr("instance_path"), right->getAttr("instance_path"));
  EXPECT_EQ(left->getOperand(1).getType(), right->getOperand(1).getType());
  EXPECT_NE(find(*file, "ac.instance")->getAttr("occurrence"),
            find(*file, "ac.instance", 1)->getAttr("occurrence"));
}

TEST_F(TransactionEffectsTest, StateEffectsAreDistinctFromCheckEffects) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto state = find(*file, "ac.state");
  auto read = find(*file, "ac.state.read");
  auto write = find(*file, "ac.state.write");
  EXPECT_TRUE(
      (hasEffect<mlir::MemoryEffects::Allocate>(state, "source.state")));
  EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Read>(state, "source.state")));
  EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Read>(read, "source.state")));
  EXPECT_FALSE((hasEffect<mlir::MemoryEffects::Write>(read, "source.state")));
  EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Write>(read, "source.check")));
  EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Write>(write, "source.state")));
  EXPECT_FALSE((hasEffect<mlir::MemoryEffects::Write>(write, "source.check")));
  for (auto *op : {state, read, write}) {
    EXPECT_FALSE(mlir::isMemoryEffectFree(op));
    EXPECT_FALSE(mlir::isSpeculatable(op));
  }
  retained(read);
  retained(write);
}

TEST_F(TransactionEffectsTest, CallsAndTransactionsRetainConservativeEffects) {
  auto file = parse();
  ASSERT_TRUE(file);
  for (llvm::StringRef name : {"ac.method.call", "ac.transaction"}) {
    auto *op = find(*file, name);
    EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Read>(op, "source.state")));
    EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Write>(op, "source.state")));
    EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Write>(op, "source.check")));
    EXPECT_TRUE(
        (hasEffect<mlir::MemoryEffects::Write>(op, "source.observation")));
    retained(op);
  }
  auto *call = find(*file, "ac.method.call", 1);
  ASSERT_TRUE(call->use_empty());
  retained(call);
  retained(find(*file, "ac.method"));
}

TEST_F(TransactionEffectsTest,
       NativeVerificationDoesNotAuthorizeFinalHardware) {
  auto file = parse();
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_TRUE(
      mlir::failed(capture([&] { return verifyHardwarePackage(*file); })));
  EXPECT_NE(diagnostics.find("unlowered"), std::string::npos) << diagnostics;
}

TEST_F(TransactionEffectsTest, MissingAndForeignStateReferencesFailClosed) {
  for (llvm::StringRef symbol : {"missing", "foreign"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    if (symbol == "foreign") {
      auto *state = find(*file, "ac.state");
      auto &root = find(*file, "ac.module", 1)->getRegion(0).front();
      auto *initial = state->getOperand(2).getDefiningOp()->clone();
      root.getOperations().insert(root.begin(), initial);
      auto *foreign = state->clone();
      foreign->setAttr("instance_name", builder.getStringAttr("foreign"));
      foreign->setAttr("occurrence",
                       find(*file, "ac.instance")->getAttr("occurrence"));
      foreign->setOperand(0, root.getArgument(0));
      foreign->setOperand(1, root.getArgument(1));
      foreign->setOperand(2, initial->getResult(0));
      root.getOperations().insert(std::next(root.begin()), foreign);
    }
    find(*file, "ac.state.read")
        ->setAttr("state", builder.getStringAttr(symbol));
    reject(*file, "state");
  }
}

TEST_F(TransactionEffectsTest, ReceiverMustResolveToTheDeclaredMethodOwner) {
  for (llvm::StringRef path : {"missing", "left", "left/missing"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *call = find(*file, "ac.method.call");
    call->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr(path)}));
    reject(*file, "instance");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  find(*file, "ac.method.call")
      ->setAttr("callee",
                mlir::SymbolRefAttr::get(
                    &context, "Root",
                    {mlir::FlatSymbolRefAttr::get(&context, "step")}));
  reject(*file, "callee");
}

TEST_F(TransactionEffectsTest,
       SelectorProjectionUsesExactNominalAndTableTypes) {
  for (llvm::StringRef selectors :
       {"[{field = \"absent\"}, {index = 0 : i64}]",
        "[{field = \"lanes\"}, {index = 1 : i64}]",
        "[{field = \"lanes\"}, {index = 0 : i32}]",
        "[{field = \"lanes\", index = 0 : i64}]", "[{field = \"lanes\"}]"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *read = find(*file, "ac.state.read", 2);
    read->setAttr("selectors", mlir::parseAttribute(selectors, &context));
    reject(*file, "state");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto *write = find(*file, "ac.state.write");
  write->setOperand(1, write->getOperand(0));
  reject(*file, "type");
}

TEST_F(TransactionEffectsTest, RegionsRespectCaptureAndReturnTypes) {
  for (unsigned region : {0u, 1u}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method");
    method->getRegion(region).front().addArgument(
        method->getOperand(0).getType(), method->getLoc());
    reject(*file, "argument");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto *transaction = find(*file, "ac.transaction");
  auto *yield = transaction->getRegion(0).front().getTerminator();
  yield->setOperand(0, transaction->getRegion(0).front().getArgument(1));
  reject(*file, "guard");
}

TEST_F(TransactionEffectsTest, GuardCannotPerformWritesOrMethodCalls) {
  for (llvm::StringRef effect : {"ac.state.write", "ac.method.call"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method", effect == "ac.method.call" ? 1 : 0);
    auto *op = find(*file, effect);
    auto *copy = op->clone();
    copy->setOperand(0, method->getRegion(0).front().getArgument(1));
    copy->setOperand(1, method->getRegion(0).front().getArgument(0));
    method->getRegion(0).front().getOperations().insert(
        method->getRegion(0).front().getTerminator()->getIterator(), copy);
    reject(*file, "guard");
  }
}

TEST_F(TransactionEffectsTest, DeclarationAndOccurrenceMetadataAreVerified) {
  for (llvm::StringRef name :
       {"ac.state", "ac.state.read", "ac.state.write", "ac.method",
        "ac.method.call", "ac.transaction"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    find(*file, name)->setAttr("occurrence", builder.getDictionaryAttr({}));
    reject(*file, "Occurrence");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  find(*file, "ac.transaction")
      ->setAttr("declaration", builder.getDictionaryAttr({}));
  reject(*file, "Occurrence");
}

TEST_F(TransactionEffectsTest,
       GuardLocationsAreMandatoryClosedSpansOwnedByTheLexicalSource) {
  for (llvm::StringRef operation : {"ac.method", "ac.transaction"}) {
    for (unsigned mutation = 0; mutation != 4; ++mutation) {
      SCOPED_TRACE(operation.str());
      SCOPED_TRACE(mutation);
      auto file = parse();
      ASSERT_TRUE(file);
      auto *callable = find(*file, operation);
      auto location =
          callable->getAttrOfType<mlir::DictionaryAttr>("ac.guard_location");
      ASSERT_TRUE(location);
      if (mutation == 0)
        callable->removeAttr("ac.guard_location");
      if (mutation == 1)
        callable->setAttr("ac.guard_location", builder.getUnitAttr());
      if (mutation == 2)
        callable->setAttr("ac.guard_location",
                          changed(location, "end_column", {}));
      if (mutation == 3)
        callable->setAttr(
            "ac.guard_location",
            changed(location, "path", builder.getStringAttr("foreign.py")));
      rejectsContract(*file);
    }
  }
}

TEST_F(TransactionEffectsTest,
       DuplicateRegistrationCannotHideBehindAnotherName) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto *transaction = find(*file, "ac.transaction");
  auto *copy = transaction->clone();
  copy->setAttr("name", builder.getStringAttr("second_registration"));
  transaction->getBlock()->getOperations().insert(transaction->getIterator(),
                                                  copy);
  reject(*file, "declaration");
}

TEST_F(TransactionEffectsTest, PureFinalRuleRejectsTypedStateAccess) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto *method = find(*file, "ac.method");
  mlir::OperationState rule(method->getLoc(), RuleOp::getOperationName());
  rule.addOperands(method->getOperands());
  rule.addTypes(method->getOperand(0).getType());
  rule.addAttribute("name", builder.getStringAttr("pure"));
  rule.addAttribute("occurrence", method->getAttr("occurrence"));
  rule.addRegion();
  builder.setInsertionPoint(method);
  auto *pure = builder.create(rule);
  auto *block = new mlir::Block;
  pure->getRegion(0).push_back(block);
  auto path =
      block->addArgument(method->getOperand(0).getType(), method->getLoc());
  auto *read = find(*file, "ac.state.read")->clone();
  read->setOperand(0, path);
  block->push_back(read);
  builder.setInsertionPointToEnd(block);
  mlir::OperationState yield(method->getLoc(), YieldOp::getOperationName());
  yield.addOperands(path);
  builder.create(yield);
  reject(*file, "placement");
}

TEST_F(TransactionEffectsTest, InitializersRequireStaticAcyclicConstructors) {
  auto runtime = parse();
  ASSERT_TRUE(runtime);
  auto *state = find(*runtime, "ac.state");
  state->setOperand(2, state->getBlock()->getArgument(2));
  reject(*runtime, "closed static literal/constructor DAG");

  {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *owner = find(*file, "ac.state");
    auto initial = owner->getOperand(2);
    builder.setInsertionPoint(owner);
    mlir::OperationState computation(owner->getLoc(), "ac.bits.binary");
    computation.addOperands({initial, initial});
    computation.addTypes(initial.getType());
    computation.addAttribute("opcode", builder.getStringAttr("add"));
    auto *op = builder.create(computation);
    owner->setOperand(2, op->getResult(0));
    reject(*file, "closed static literal/constructor DAG");
  }
  auto cycle = parse();
  ASSERT_TRUE(cycle);
  auto *packet = find(*cycle, "ac.state", 1);
  auto *constructor = packet->getOperand(2).getDefiningOp();
  builder.setInsertionPoint(packet);
  mlir::OperationState get(packet->getLoc(), "ac.struct.get");
  get.addOperands(constructor->getResult(0));
  get.addTypes(constructor->getOperand(0).getType());
  get.addAttribute("field", builder.getStringAttr("lanes"));
  auto *projection = builder.create(get);
  constructor->setOperand(0, projection->getResult(0));
  reject(*cycle, "cyclic");
}

TEST_F(TransactionEffectsTest, ControlWidthsAndStatePayloadCannotBeInferred) {
  for (llvm::StringRef name : {"ac.state", "ac.state.read", "ac.state.write",
                               "ac.method.call", "ac.transaction"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *op = find(*file, name);
    auto replacement = name == "ac.state" || name == "ac.transaction"
                           ? op->getBlock()->getArgument(2)
                           : op->getBlock()->getArgument(0);
    op->setOperand(0, replacement);
    reject(*file, "width-one Bits");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto *state = find(*file, "ac.state");
  state->setOperand(2, state->getOperand(1));
  reject(*file, "initial value and old-Q payload types must match");
}

TEST_F(TransactionEffectsTest, CallsAndOrdinaryReturnsKeepExactSignature) {
  for (bool arity : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *call = find(*file, "ac.method.call");
    if (arity)
      call->eraseOperand(1);
    else
      call->setOperand(1, call->getOperand(0));
    reject(*file, arity ? "arity" : "payload type");
  }
  for (llvm::StringRef name : {"ac.method", "ac.transaction"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto &body = find(*file, name)->getRegion(1).front();
    auto path = body.getArgument(name == "ac.method" ? 1 : 0);
    body.getTerminator()->setOperand(0, path);
    reject(*file, "body yield types");
  }
}

TEST_F(TransactionEffectsTest,
       EffectsAndUnknownOperationsCannotHideInPayloads) {
  for (llvm::StringRef name : {"test.unknown"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    context.allowUnregisteredDialects();
    auto *method = find(*file, "ac.method");
    auto &body = method->getRegion(1).front();
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState effect(method->getLoc(), name);
    builder.create(effect);
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(TransactionEffectsTest,
       SourceChecksAndObservationsAreOrderedBodyEffects) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto *method = find(*file, "ac.method");
  auto &body = method->getRegion(1).front();
  auto path = body.getArgument(1);
  auto occurrence = method->getAttr("occurrence");
  auto location = method->getAttr("ac.guard_location");
  builder.setInsertionPoint(body.getTerminator());

  mlir::OperationState check(method->getLoc(), "ac.expect");
  check.addOperands({path, path});
  check.addAttribute("kind", builder.getStringAttr("assert"));
  check.addAttribute("location", location);
  check.addAttribute("occurrence", occurrence);
  check.addAttribute("ac.message", builder.getStringAttr(""));
  auto *checkOp = builder.create(check);

  mlir::OperationState observe(method->getLoc(), "ac.observe");
  observe.addOperands({path, body.getArgument(0)});
  observe.addAttribute("kind", builder.getStringAttr("report"));
  observe.addAttribute("spec", builder.getDictionaryAttr({builder.getNamedAttr(
                                   "name", builder.getStringAttr("value"))}));
  observe.addAttribute("occurrence", occurrence);
  observe.addAttribute("ac.location", location);
  auto *observeOp = builder.create(observe);

  EXPECT_TRUE(checkOp->isBeforeInBlock(observeOp));
  EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_TRUE((hasEffect<mlir::MemoryEffects::Write>(checkOp, "source.check")));
  EXPECT_TRUE(
      (hasEffect<mlir::MemoryEffects::Write>(observeOp, "source.observation")));
  retained(checkOp);
  retained(observeOp);
}

TEST_F(TransactionEffectsTest,
       SourceEffectsRejectGuardPlacementFinalIdentityAndWrongKinds) {
  for (llvm::StringRef mutation : {"guard", "check-id", "observation-id",
                                   "check-kind", "observation-kind"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method");
    auto &region = method->getRegion(mutation == "guard" ? 0 : 1).front();
    auto path = region.getArgument(1);
    auto occurrence = method->getAttr("occurrence");
    auto location = method->getAttr("ac.guard_location");
    builder.setInsertionPoint(region.getTerminator());
    bool observation = mutation.starts_with("observation");
    mlir::OperationState effect(method->getLoc(),
                                observation ? "ac.observe" : "ac.expect");
    if (observation) {
      effect.addOperands({path, region.getArgument(0)});
      effect.addAttribute(
          "kind", builder.getStringAttr(
                      mutation == "observation-kind" ? "trace" : "report"));
      effect.addAttribute("spec",
                          builder.getDictionaryAttr({builder.getNamedAttr(
                              "name", builder.getStringAttr("value"))}));
      effect.addAttribute("ac.location", location);
      if (mutation == "observation-id")
        effect.addAttribute("ac.observation_id", occurrence);
    } else {
      effect.addOperands({path, path});
      effect.addAttribute(
          "kind",
          builder.getStringAttr(mutation == "check-kind" ? "range" : "assert"));
      effect.addAttribute("location", location);
      if (mutation == "check-id")
        effect.addAttribute("ac.check_id", occurrence);
    }
    effect.addAttribute("occurrence", occurrence);
    builder.create(effect);
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })))
        << mutation.str();
    EXPECT_FALSE(diagnostics.empty());
  }

  for (llvm::StringRef mutation :
       {"sampling", "path-type", "foreign-location", "missing-occurrence"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method");
    auto &body = method->getRegion(1).front();
    auto path = body.getArgument(1);
    mlir::OperationState effect(method->getLoc(), "ac.expect");
    effect.addOperands(
        {path, mutation == "path-type" ? body.getArgument(0) : path});
    if (mutation == "sampling")
      effect.addOperands(path);
    effect.addAttribute("kind", builder.getStringAttr("assert"));
    auto location =
        method->getAttrOfType<mlir::DictionaryAttr>("ac.guard_location");
    if (mutation == "foreign-location")
      location = changed(location, "path", builder.getStringAttr("other.py"));
    effect.addAttribute("location", location);
    if (mutation != "missing-occurrence")
      effect.addAttribute("occurrence", method->getAttr("occurrence"));
    builder.setInsertionPoint(body.getTerminator());
    builder.create(effect);
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })))
        << mutation.str();
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(TransactionEffectsTest,
       NestedDeclarationsCannotCreateAnotherOwnerScope) {
  for (llvm::StringRef name : {"ac.state", "ac.method", "ac.transaction"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *nested = find(*file, name)->clone();
    auto *method = find(*file, "ac.method");
    auto &body = method->getRegion(1).front();
    if (name == "ac.state") {
      nested->setOperand(0, body.getArgument(1));
      nested->setOperand(1, body.getArgument(1));
      nested->setOperand(2, body.getArgument(0));
    } else if (name == "ac.method") {
      nested->setOperand(0, body.getArgument(1));
    } else {
      nested->setOperand(0, body.getArgument(1));
      nested->setOperand(1, body.getArgument(1));
      nested->setOperand(2, body.getArgument(1));
      nested->setOperand(3, body.getArgument(0));
    }
    body.getOperations().insert(body.getTerminator()->getIterator(), nested);
    reject(*file,
           name == "ac.method" ? "SymbolTable" : "direct module placement");
  }
}

TEST_F(TransactionEffectsTest, PureCollectionRegionsDoNotAdmitHiddenEffects) {
  for (bool stateRead : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto &map = find(*file, "ac.table.map")->getRegion(0).front();
    if (stateRead) {
      auto *read = find(*file, "ac.state.read")->clone();
      read->setOperand(0, map.getArgument(0));
      map.getOperations().insert(map.getTerminator()->getIterator(), read);
      reject(*file, "placement");
    } else {
      context.allowUnregisteredDialects();
      builder.setInsertionPoint(map.getTerminator());
      mlir::OperationState unknown(map.getTerminator()->getLoc(),
                                   "test.unknown");
      builder.create(unknown);
      reject(*file, "region");
    }
  }
}

TEST_F(TransactionEffectsTest,
       StateIdentityAndNamesCannotCollapseDistinctOwners) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto *first = find(*file, "ac.state");
  auto *second = find(*file, "ac.state", 1);
  second->setAttr("occurrence", first->getAttr("occurrence"));
  reject(*file, "occurrence");

  auto collision = parse();
  ASSERT_TRUE(collision);
  auto *state = find(*collision, "ac.state");
  auto &owner = state->getParentOp()->getRegion(0).front();
  auto *child = find(*collision, "ac.instance")->clone();
  child->setAttr("instance_name", state->getAttr("instance_name"));
  child->setAttr("occurrence", state->getAttr("occurrence"));
  child->setOperands(owner.getArguments());
  owner.getOperations().insert(state->getIterator(), child);
  reject(*collision, "instance_name");
}

TEST_F(TransactionEffectsTest,
       ReceiverIdentityCannotCollapseDistinctInstances) {
  auto file = parse();
  ASSERT_TRUE(file);
  find(*file, "ac.instance", 1)
      ->setAttr("occurrence",
                find(*file, "ac.instance")->getAttr("occurrence"));
  reject(*file, "occurrence");
}

TEST_F(TransactionEffectsTest, ReceiverOccurrenceMustNameItsActualParent) {
  auto file = parse();
  ASSERT_TRUE(file);
  find(*file, "ac.instance")
      ->setAttr("occurrence", find(*file, "ac.state")->getAttr("occurrence"));
  reject(*file, "definition");
}

TEST_F(TransactionEffectsTest, SourceIdentityMustNameItsActualModuleOwner) {
  for (llvm::StringRef name : {"ac.state", "ac.method", "ac.transaction"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *op = find(*file, name);
    auto *foreign =
        find(*file, name == "ac.transaction" ? "ac.state" : "ac.transaction");
    op->setAttr("occurrence", foreign->getAttr("occurrence"));
    reject(*file, "definition");
  }
  auto file = parse();
  ASSERT_TRUE(file);
  find(*file, "ac.transaction")
      ->setAttr("declaration", find(*file, "ac.state")->getAttr("occurrence"));
  reject(*file, "definition");
}

TEST_F(TransactionEffectsTest,
       MalformedAttributesDiagnoseBeforeTypedAccessors) {
  for (unsigned mutation = 0; mutation != 12; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto *call = find(*file, "ac.method.call", 1);
    auto *method = find(*file, "ac.method", 1);
    auto *instance = find(*file, "ac.instance");
    switch (mutation) {
    case 0:
      call->setAttr("instance_path",
                    builder.getArrayAttr({builder.getI64IntegerAttr(1)}));
      break;
    case 1:
      call->removeAttr("callee");
      break;
    case 2:
      method->removeAttr("function_type");
      break;
    case 3:
      method->setAttr("function_type",
                      mlir::TypeAttr::get(builder.getI64Type()));
      break;
    case 4:
      instance->removeAttr("callee");
      break;
    case 5:
      instance->removeAttr("instance_name");
      break;
    case 6:
      instance->setAttr("parameters", builder.getStringAttr("invalid"));
      break;
    case 7: {
      auto *state = find(*file, "ac.state");
      state->setAttr("sym_name", builder.getStringAttr("count"));
      break;
    }
    case 8:
      find(*file, "ac.state.read")
          ->setAttr("state", mlir::FlatSymbolRefAttr::get(&context, "count"));
      break;
    case 9: {
      auto *transaction = find(*file, "ac.transaction");
      transaction->setAttr("sym_name", builder.getStringAttr("step"));
      break;
    }
    case 10: {
      auto *state = find(*file, "ac.state");
      state->removeAttr("instance_name");
      state->setAttr("sym_name", builder.getStringAttr("count"));
      break;
    }
    case 11: {
      auto *transaction = find(*file, "ac.transaction");
      transaction->removeAttr("name");
      transaction->setAttr("sym_name", builder.getStringAttr("step"));
      break;
    }
    }
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*file); })));
    EXPECT_FALSE(diagnostics.empty()) << mutation;
  }
}

TEST_F(TransactionEffectsTest, MethodCallAuthorityIsMandatoryAndTyped) {
  for (llvm::StringRef name :
       {"ac.parameters", "ac.result_constraints", "ac.return_form"}) {
    for (bool missing : {false, true}) {
      SCOPED_TRACE(name.str() + (missing ? " missing" : " wrong type"));
      auto file = parse();
      ASSERT_TRUE(file);
      auto *method = find(*file, "ac.method");
      if (missing)
        method->removeAttr(name);
      else
        method->setAttr(name, builder.getI64IntegerAttr(0));
      rejectsContract(*file);
    }
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto *method = find(*file, "ac.method");
  method->removeAttr("ac.parameters");
  method->removeAttr("ac.result_constraints");
  method->removeAttr("ac.return_form");
  rejectsContract(*file);
}

TEST_F(TransactionEffectsTest, MethodParameterRowsHaveAnExactClosedSchema) {
  for (llvm::StringRef name :
       {"name", "binding", "constraint", "default", "origin", "location"}) {
    for (bool missing : {false, true}) {
      SCOPED_TRACE(name.str() + (missing ? " missing" : " wrong type"));
      auto file = parse();
      ASSERT_TRUE(file);
      auto *method = find(*file, "ac.method");
      setParameter(method, changed(parameter(method), name,
                                   missing ? mlir::Attribute()
                                           : builder.getI64IntegerAttr(0)));
      rejectsContract(*file);
    }
  }
  auto file = parse();
  ASSERT_TRUE(file);
  auto *method = find(*file, "ac.method");
  setParameter(method,
               changed(parameter(method), "extra", builder.getUnitAttr()));
  rejectsContract(*file);
}

TEST_F(TransactionEffectsTest,
       MethodParameterIdentityBindingAndDefaultAreChecked) {
  const std::pair<const char *, const char *> mutations[] = {
      {"name", R"attr("other")attr"},
      {"binding", R"attr("keyword_only")attr"},
      {"default",
       R"attr({present = true, value = {kind = "integer", value = #ac.math_int<0>}})attr"},
      {"default", R"attr({present = false, extra = true})attr"},
      {"origin",
       R"attr({site = {definition = @Root, ast_path = []}, expansion = []})attr"},
      {"origin",
       R"attr({site = {definition = @Counter::@add, ast_path = []}, expansion = []})attr"},
      {"location",
       R"attr({path = "foreign.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64})attr"},
      {"location",
       R"attr({path = "transactions.py", line = 0 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64})attr"}};
  for (auto [name, spelling] : mutations) {
    SCOPED_TRACE(spelling);
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method");
    auto value = mlir::parseAttribute(spelling, &context);
    ASSERT_TRUE(value);
    setParameter(method, changed(parameter(method), name, value));
    rejectsContract(*file);
  }
}

TEST_F(TransactionEffectsTest, MethodMetadataArityAndReturnFormAreExact) {
  for (unsigned mutation = 0; mutation != 7; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto *method = find(*file, "ac.method");
    auto row = parameter(method);
    switch (mutation) {
    case 0:
      method->setAttr("ac.parameters", builder.getArrayAttr({}));
      break;
    case 1:
      method->setAttr("ac.parameters", builder.getArrayAttr({row, row}));
      break;
    case 2:
      method->setAttr("ac.parameters",
                      builder.getArrayAttr({builder.getUnitAttr()}));
      break;
    case 3:
      method->setAttr("ac.result_constraints", builder.getArrayAttr({}));
      break;
    case 4: {
      auto results =
          method->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints");
      method->setAttr("ac.result_constraints",
                      builder.getArrayAttr({results[0], results[0]}));
      break;
    }
    case 5:
      method->setAttr("ac.return_form", builder.getStringAttr("none"));
      break;
    case 6:
      method->setAttr("ac.return_form", builder.getStringAttr("tuple"));
      break;
    }
    rejectsContract(*file);
  }
}

TEST_F(TransactionEffectsTest, MethodConstraintsCannotInventLogicalAuthority) {
  const char *mutations[] = {
      R"attr({kind = "logical", type = TYPE, source_kind = "fixed_bits"})attr",
      R"attr({kind = "hardware", type = i8, source_kind = "fixed_bits"})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "nominal"})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "table"})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "unknown"})attr",
      R"attr({kind = "hardware", type = TYPE})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "fixed_bits", domain = #ac.source_domain<{kind = "bool"}>})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "fixed_bits", extra = true})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "boolean", domain = #ac.source_domain<{kind = "bool"}>})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "integer"})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "integer", domain = #ac.source_domain<{kind = "bool"}>})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "integer", domain = #ac.source_domain<{kind = "integer", lower = #ac.math_int<1>, upper = #ac.math_int<256>}>})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "integer", domain = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<128>}>})attr",
      R"attr({kind = "hardware", type = TYPE, source_kind = "integer", domain = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<255>}>})attr"};
  for (bool result : {false, true}) {
    for (const char *mutation : mutations) {
      SCOPED_TRACE(mutation);
      SCOPED_TRACE(result ? "result" : "parameter");
      auto file = parse();
      ASSERT_TRUE(file);
      auto *method = find(*file, "ac.method");
      std::string type;
      llvm::raw_string_ostream stream(type);
      mlir::cast<mlir::FunctionType>(
          mlir::cast<mlir::TypeAttr>(method->getAttr("function_type"))
              .getValue())
          .getInput(0)
          .print(stream);
      std::string spelling(mutation);
      if (auto offset = spelling.find("TYPE"); offset != std::string::npos)
        spelling.replace(offset, 4, type);
      auto value = mlir::parseAttribute(spelling, &context);
      ASSERT_TRUE(value);
      if (result)
        method->setAttr("ac.result_constraints", builder.getArrayAttr({value}));
      else
        setParameter(method, changed(parameter(method), "constraint", value));
      rejectsContract(*file);
    }
  }
}

TEST_F(TransactionEffectsTest, MalformedCalleeMetadataRejectsDuringSymbolUse) {
  for (llvm::StringRef name :
       {"ac.parameters", "ac.result_constraints", "ac.return_form"}) {
    for (bool missing : {false, true}) {
      SCOPED_TRACE(name.str());
      auto file = parse();
      ASSERT_TRUE(file);
      auto *method = find(*file, "ac.method");
      if (missing)
        method->removeAttr(name);
      else
        method->setAttr(name, builder.getUnitAttr());
      mlir::SymbolTableCollection symbols;
      auto call = mlir::cast<SourceMethodCallOp>(find(*file, "ac.method.call"));
      EXPECT_TRUE(mlir::failed(
          capture([&] { return call.verifySymbolUses(symbols); })));
      EXPECT_FALSE(diagnostics.empty());
    }
  }
}

TEST_F(TransactionEffectsTest,
       SourceBranchRequiresClosedTypedArmsAndRetainsDistinctResultSSA) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto transaction =
      mlir::cast<SourceTransactionOp>(find(*file, "ac.transaction"));
  auto &body = transaction.getBody().front();
  auto split = branch(body, body.begin(), body.getArgument(0),
                      body.getArgument(1), body.getArgument(1));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_NE(split.getResult(0), body.getArgument(1));
  EXPECT_EQ(split.getResult(0).getType(), body.getArgument(1).getType());
  EXPECT_TRUE(split.getThenRegion().hasOneBlock());
  EXPECT_TRUE(split.getElseRegion().hasOneBlock());
  EXPECT_TRUE(split.getThenRegion().front().getArguments().empty());
  EXPECT_TRUE(split.getElseRegion().front().getArguments().empty());

  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    auto malformed = parse();
    ASSERT_TRUE(malformed);
    auto tx =
        mlir::cast<SourceTransactionOp>(find(*malformed, "ac.transaction"));
    auto &block = tx.getBody().front();
    auto candidate = branch(block, block.begin(), block.getArgument(0),
                            block.getArgument(1), block.getArgument(1));
    if (mutation == 0)
      candidate->removeAttr("occurrence");
    if (mutation == 1)
      candidate->removeAttr("ac.condition_location");
    if (mutation == 2)
      candidate.getThenRegion().front().addArgument(
          block.getArgument(0).getType(), candidate.getLoc());
    if (mutation == 3)
      candidate.getThenRegion().front().getTerminator()->setOperand(
          0, block.getArgument(0));
    if (mutation == 4) {
      auto *extra = new mlir::Block;
      candidate.getElseRegion().push_back(extra);
    }
    rejectsContract(*malformed);
  }
}

TEST_F(TransactionEffectsTest,
       EmptyPassAndNestedBranchesVerifyButGuardBranchesFailClosed) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto transaction =
      mlir::cast<SourceTransactionOp>(find(*file, "ac.transaction"));
  auto &body = transaction.getBody().front();
  auto outer = branch(body, body.begin(), body.getArgument(0));
  auto &thenBlock = outer.getThenRegion().front();
  auto nested =
      branch(thenBlock, thenBlock.getTerminator()->getIterator(),
             body.getArgument(0), body.getArgument(1), body.getArgument(1));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  EXPECT_TRUE(outer.getResults().empty());
  EXPECT_EQ(nested->getParentOp(), outer.getOperation());

  auto malformed = parse();
  ASSERT_TRUE(malformed);
  auto tx = mlir::cast<SourceTransactionOp>(find(*malformed, "ac.transaction"));
  auto &guard = tx.getGuard().front();
  branch(guard, guard.getTerminator()->getIterator(), guard.getArgument(0));
  rejectsContract(*malformed);
}

TEST_F(TransactionEffectsTest,
       BranchArmsRejectCallsChecksAndObservationsOnEveryPath) {
  for (llvm::StringRef effect : {"ac.method.call", "ac.expect", "ac.observe"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto transaction =
        mlir::cast<SourceTransactionOp>(find(*file, "ac.transaction"));
    auto &body = transaction.getBody().front();
    auto split = branch(body, body.begin(), body.getArgument(0));
    auto &arm = split.getThenRegion().front();
    builder.setInsertionPoint(arm.getTerminator());
    if (effect == "ac.method.call") {
      auto *copy = find(*file, effect)->clone();
      arm.getOperations().insert(arm.getTerminator()->getIterator(), copy);
    } else if (effect == "ac.expect") {
      mlir::OperationState check(split.getLoc(),
                                 SourceExpectOp::getOperationName());
      check.addOperands({body.getArgument(0), body.getArgument(0)});
      check.addAttribute("kind", builder.getStringAttr("assert"));
      check.addAttribute("location", transaction->getAttr("ac.guard_location"));
      check.addAttribute("occurrence", transaction->getAttr("occurrence"));
      builder.create(check);
    } else {
      mlir::OperationState observe(split.getLoc(),
                                   SourceObserveOp::getOperationName());
      observe.addOperands(body.getArgument(0));
      observe.addAttribute("kind", builder.getStringAttr("log"));
      observe.addAttribute(
          "spec",
          builder.getDictionaryAttr(
              {builder.getNamedAttr("level", builder.getStringAttr("info")),
               builder.getNamedAttr("event", builder.getStringAttr("branch")),
               builder.getNamedAttr("items", builder.getArrayAttr({}))}));
      observe.addAttribute("occurrence", transaction->getAttr("occurrence"));
      observe.addAttribute("ac.location",
                           transaction->getAttr("ac.guard_location"));
      builder.create(observe);
    }
    rejectsContract(*file);
  }
}

} // namespace
} // namespace acir::ac
