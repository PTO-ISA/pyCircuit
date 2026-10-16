#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/AnalysisManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace acir::ac {
namespace {

// Native, intermediate-IR provenance tests. These are neither source admission
// nor simulation or transaction materialization evidence.
std::string stateFixture() {
  std::string text;
  for (unsigned n : {0u, 1u, 8u})
    text += "#w" + std::to_string(n) +
            " = #ac.static_expr<{kind = \"literal\", value = {kind = "
            "\"integer\", value = #ac.math_int<" +
            std::to_string(n) +
            ">}, origin = {site = {definition = @Child, ast_path = []}, "
            "expansion = []}, location = {path = \"state.py\", line = 1 : i64, "
            "column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>\n";
  text += R"ir(
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
#span = {path = "state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
)ir";
  for (auto owner : {"Child", "Root"}) {
    auto prefix = std::string(owner) == "Child" ? "c" : "r";
    for (unsigned i = 0; i < 32; ++i)
      text += "#" + std::string(prefix) + std::to_string(i) +
              " = {site = {definition = @" + owner +
              ", ast_path = [{kind = \"index\", value = " + std::to_string(i) +
              " : i64}]}, expansion = []}\n";
  }
  text += R"ir(
module {
  ac.struct "Packet" fields [{name = "payload", type = !b8}]
  "ac.module"() ({
  ^bb0(%seed: !b8, %flag: !b1, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %no = "ac.bits.constant"() {value = #w0} : () -> !b1
    %initial = "ac.bits.constant"() {value = #w0} : () -> !b8
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #c0} : (!b1, !b1, !b8) -> !b8
    %packet_initial = "ac.struct.create"(%initial) : (!b8) -> !ac.struct<"Packet">
    %packet_q = "ac.state"(%clk, %rst, %packet_initial) {instance_name = "packet", occurrence = #c1} : (!b1, !b1, !ac.struct<"Packet">) -> !ac.struct<"Packet">
    "ac.method"(%yes, %q, %clk, %rst, %flag) ({
    ^bb0(%delta: !b8, %path: !b1, %snapshot: !b8, %clock: !b1, %reset: !b1, %admit: !b1):
      %guard_read = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c3} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1, %snapshot: !b8, %clock: !b1, %reset: !b1, %admit: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c4} : (!b1) -> !b8
      %next = "ac.bits.binary"(%old, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.state.write"(%path, %next) {state = "count", selectors = [], occurrence = #c5} : (!b1, !b8) -> ()
      "ac.yield"(%next) : (!b8) -> ()
    }) {sym_name = "bump", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #c2, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", ac.guard_location = #span, occurrence = #c2} : (!b1, !b8, !b1, !b1, !b1) -> ()
    "ac.method"(%yes) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %nested = "ac.method.call"(%path, %delta) {callee = @Child::@bump, instance_path = [], occurrence = #c7} : (!b1, !b8) -> !b8
      "ac.yield"(%nested) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #c6, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", ac.guard_location = #span, occurrence = #c6} : (!b1) -> ()
    "ac.method"(%yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      %guard_read = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c9} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %unused = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c10} : (!b1) -> !b8
      "ac.yield"(%snapshot) : (!b8) -> ()
    }) {sym_name = "peek", function_type = () -> !b8, argument_names = [], ac.parameters = [], ac.result_constraints = [#byte], ac.return_form = "single", ac.guard_location = #span, occurrence = #c8} : (!b1, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1):
      "ac.yield"() : () -> ()
    }) {sym_name = "noop", function_type = () -> (), argument_names = [], ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", ac.guard_location = #span, occurrence = #c11} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "state.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1, !b1) -> !b8, input_names = ["seed", "flag", "pyc_clk", "pyc_rst"], output_names = ["count"], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}} : () -> ()
  "ac.module"() ({
  ^bb0(%seed: !b8, %flag: !b1, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %no = "ac.bits.constant"() {value = #w0} : () -> !b1
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b8
    %q = "ac.state"(%clk, %rst, %zero) {instance_name = "status", occurrence = #r0} : (!b1, !b1, !b8) -> !b8
    %left = "ac.instance"(%seed, %flag, %clk, %rst) {instance_name = "left", callee = @Child, parameters = [], type_arguments = [], occurrence = #r1} : (!b8, !b1, !b1, !b1) -> !b8
    %right = "ac.instance"(%seed, %flag, %clk, %rst) {instance_name = "right", callee = @Child, parameters = [], type_arguments = [], occurrence = #r2} : (!b8, !b1, !b1, !b1) -> !b8
    %answer = "ac.transaction"(%clk, %rst, %yes, %seed, %q, %clk, %rst) ({
    ^bb0(%path: !b1, %delta: !b8, %snapshot: !b8, %clock: !b1, %reset: !b1):
      %before = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r5} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %delta: !b8, %snapshot: !b8, %clock: !b1, %reset: !b1):
      %before = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r6} : (!b1) -> !b8
      %first = "ac.method.call"(%path, %delta) {callee = @Child::@bump, instance_path = ["left"], occurrence = #r7} : (!b1, !b8) -> !b8
      %second = "ac.method.call"(%path, %first) {callee = @Child::@relay, instance_path = ["left"], occurrence = #r8} : (!b1, !b8) -> !b8
      %third = "ac.method.call"(%path, %first) {callee = @Child::@bump, instance_path = ["right"], occurrence = #r9} : (!b1, !b8) -> !b8
      "ac.state.write"(%path, %before) {state = "status", selectors = [], occurrence = #r10} : (!b1, !b8) -> ()
      %after = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r11} : (!b1) -> !b8
      %unused = "ac.method.call"(%path) {callee = @Child::@peek, instance_path = ["left"], occurrence = #r12} : (!b1) -> !b8
      "ac.method.call"(%path) {callee = @Child::@noop, instance_path = ["left"], occurrence = #r13} : (!b1) -> ()
      "ac.yield"(%first) : (!b8) -> ()
    }) {name = "run", declaration = #r3, occurrence = #r4, ac.guard_location = #span} : (!b1, !b1, !b1, !b8, !b8, !b1, !b1) -> !b8
    "ac.transaction"(%clk, %rst, %yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %old = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r16} : (!b1) -> !b8
      %unused = "ac.method.call"(%path) {callee = @Child::@peek, instance_path = ["left"], occurrence = #r17} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #r14, occurrence = #r15, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> ()
    "ac.yield"(%answer) : (!b8) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "state.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1, !b1) -> !b8, input_names = ["seed", "flag", "pyc_clk", "pyc_rst"], output_names = ["answer"], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}} : () -> ()
}
)ir";
  return text;
}

class TransactionStateSSATest : public ::testing::Test {
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
        stateFixture(), mlir::ParserConfig(&context, false));
    EXPECT_TRUE(file);
    if (file)
      EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
    return file;
  }
  ModuleOp module(mlir::ModuleOp file, llvm::StringRef name) {
    for (auto op : file.getOps<ModuleOp>())
      if (op.getSymName() == name)
        return op;
    ADD_FAILURE() << "missing " << name.str();
    return {};
  }
  SourceTransactionOp transaction(mlir::ModuleOp file,
                                  llvm::StringRef name = "run") {
    for (auto op :
         module(file, "Root").getBody().front().getOps<SourceTransactionOp>())
      if (op.getName() == name)
        return op;
    ADD_FAILURE() << "missing transaction";
    return {};
  }
  SourceMethodOp method(mlir::ModuleOp file, llvm::StringRef name = "bump") {
    for (auto op :
         module(file, "Child").getBody().front().getOps<SourceMethodOp>())
      if (op.getSymName() == name)
        return op;
    ADD_FAILURE() << "missing method";
    return {};
  }
  template <typename Op> Op first(mlir::Region &region) {
    return *region.front().getOps<Op>().begin();
  }
  std::unique_ptr<SourceTransactionStateAnalysis> analyze(mlir::ModuleOp file) {
    return capture([&] {
      return std::make_unique<SourceTransactionStateAnalysis>(
          file.getOperation());
    });
  }
  std::unique_ptr<SourceTransactionStateAnalysis>
  analyzeRawPreflightFailure(mlir::ModuleOp file) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &error) {
          // A raw graph is not yet trusted even as a diagnostic location.
          EXPECT_TRUE(mlir::isa<mlir::UnknownLoc>(error.getLocation()));
          EXPECT_TRUE(error.getNotes().empty());
          llvm::raw_string_ostream stream(diagnostics);
          error.print(stream);
          return mlir::success();
        });
    return std::make_unique<SourceTransactionStateAnalysis>(
        file.getOperation());
  }
  std::string print(mlir::Operation *op) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    op->print(stream);
    return text;
  }
  std::string dump(const SourceTransactionStateAnalysis &analysis) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    EXPECT_TRUE(mlir::succeeded(analysis.print(stream)));
    return text;
  }
  std::vector<mlir::Operation *> identities(mlir::ModuleOp file) {
    std::vector<mlir::Operation *> result;
    file.walk([&](mlir::Operation *op) { result.push_back(op); });
    return result;
  }
  void reject(mlir::ModuleOp file) {
    auto text = print(file.getOperation());
    auto ops = identities(file);
    auto analysis = analyze(file);
    EXPECT_FALSE(analysis->isValid()) << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
    for (auto name : {"run", "watch"})
      EXPECT_EQ(analysis->lookup(transaction(file, name)), nullptr);
    std::string partial;
    llvm::raw_string_ostream stream(partial);
    EXPECT_TRUE(mlir::failed(analysis->print(stream)));
    EXPECT_TRUE(partial.empty());
    EXPECT_EQ(text, print(file.getOperation()));
    EXPECT_EQ(ops, identities(file));
  }
  uint64_t ordinal(const TransactionStatePlan &plan, StateVersionId id) {
    EXPECT_LT(id, plan.versions.size());
    return plan.versions[id].ordinal;
  }
  std::vector<const StateReadBinding *> reads(const TransactionStatePlan &plan,
                                              InvocationFrameId frame) {
    std::vector<const StateReadBinding *> result;
    for (auto &read : plan.reads)
      if (read.frame == frame)
        result.push_back(&read);
    return result;
  }
  const TransactionStateEvent *
  event(const TransactionStatePlan &plan, TransactionStateEventKind kind,
        std::optional<uint64_t> record = std::nullopt) {
    for (const auto &candidate : plan.events) {
      if (candidate.kind != kind)
        continue;
      std::optional<uint64_t> actual;
      switch (kind) {
      case TransactionStateEventKind::Read:
        actual = candidate.read;
        break;
      case TransactionStateEventKind::Write:
        actual = candidate.write;
        break;
      case TransactionStateEventKind::GuardEnter:
      case TransactionStateEventKind::GuardExit:
        actual = candidate.guard;
        break;
      case TransactionStateEventKind::SelectionEnter:
      case TransactionStateEventKind::SelectionExit:
        actual = candidate.selection;
        break;
      case TransactionStateEventKind::CallEnter:
      case TransactionStateEventKind::CallExit:
        actual = candidate.call;
        break;
      case TransactionStateEventKind::BodyCheck:
        actual = candidate.bodyCheck;
        break;
      case TransactionStateEventKind::Observe:
        actual = candidate.observation;
        break;
      case TransactionStateEventKind::BranchEnter:
      case TransactionStateEventKind::ThenEnter:
      case TransactionStateEventKind::ThenExit:
      case TransactionStateEventKind::ElseEnter:
      case TransactionStateEventKind::ElseExit:
      case TransactionStateEventKind::BranchExit:
        actual = candidate.branch;
        break;
      case TransactionStateEventKind::StateJoin:
        actual = candidate.join;
        break;
      case TransactionStateEventKind::BranchResult:
        actual = candidate.branchResult;
        break;
      case TransactionStateEventKind::TransactionEnter:
      case TransactionStateEventKind::TransactionExit:
        break;
      }
      if (!record || actual == record)
        return &candidate;
    }
    ADD_FAILURE() << "missing transaction event";
    return nullptr;
  }
  const BoundValueRef *value(const TransactionStatePlan &plan,
                             mlir::Value original, LexicalInstanceId anchor,
                             InvocationFrameId frame) {
    for (const auto &candidate : plan.values)
      if (candidate.value == original && candidate.anchor == anchor &&
          candidate.frame == frame)
        return &candidate;
    ADD_FAILURE() << "missing bound transaction value";
    return nullptr;
  }
  mlir::DictionaryAttr occurrence(llvm::StringRef owner, unsigned index) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "site", builder.getDictionaryAttr(
                         {builder.getNamedAttr(
                              "definition",
                              mlir::FlatSymbolRefAttr::get(&context, owner)),
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
  mlir::DictionaryAttr sourceOwner(llvm::StringRef path) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("package", builder.getStringAttr("")),
         builder.getNamedAttr("path", builder.getStringAttr(path))});
  }
  mlir::DictionaryAttr span(llvm::StringRef path, unsigned line) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr(path)),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(9))});
  }
  SourceBranchOp branch(mlir::Block &block, mlir::Block::iterator where,
                        mlir::Value condition,
                        std::optional<mlir::Value> whenTrue,
                        std::optional<mlir::Value> whenFalse,
                        unsigned occurrenceIndex,
                        llvm::StringRef owner = "Root") {
    EXPECT_EQ(whenTrue.has_value(), whenFalse.has_value());
    mlir::OperationState state(condition.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(condition);
    if (whenTrue)
      state.addTypes((*whenTrue).getType());
    state.addAttribute("occurrence", occurrence(owner, occurrenceIndex));
    state.addAttribute("ac.condition_location",
                       span("state.py", occurrenceIndex));
    state.addRegion();
    state.addRegion();
    builder.setInsertionPoint(&block, where);
    auto split = mlir::cast<SourceBranchOp>(builder.create(state));
    for (auto [region, value] : llvm::zip(
             split->getRegions(),
             std::array<std::optional<mlir::Value>, 2>{whenTrue, whenFalse})) {
      auto *arm = new mlir::Block;
      region.push_back(arm);
      builder.setInsertionPointToEnd(arm);
      mlir::OperationState yield(split.getLoc(), YieldOp::getOperationName());
      if (value)
        yield.addOperands(*value);
      builder.create(yield);
    }
    return split;
  }
};

TEST_F(TransactionStateSSATest,
       RepeatedNestedCallsThreadVersionsAndSaveOrdinaryResults) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->owners.size(), 3u);
  ASSERT_EQ(plan->writes.size(), 4u);
  ASSERT_EQ(plan->calls.size(), 6u); // relay includes one nested bump.
  ASSERT_EQ(plan->versions.size(), 7u);
  for (unsigned i = 0; i < plan->versions.size(); ++i) {
    auto &version = plan->versions[i];
    EXPECT_EQ(version.transaction, plan->transaction);
    EXPECT_EQ(version.payloadType, plan->owners[version.owner].payloadType);
    if (version.kind == StateVersionKind::OldQ) {
      EXPECT_EQ(version.ordinal, 0u);
      EXPECT_FALSE(version.predecessor);
      EXPECT_FALSE(version.write);
      EXPECT_FALSE(version.join);
      EXPECT_EQ(version.value, plan->owners[version.owner].oldQ);
    } else if (version.kind == StateVersionKind::Write) {
      ASSERT_TRUE(version.predecessor);
      ASSERT_TRUE(version.write);
      EXPECT_FALSE(version.join);
      ASSERT_TRUE(version.value);
      auto &previous = plan->versions[*version.predecessor];
      auto &intent = plan->writes[*version.write];
      EXPECT_EQ(previous.owner, version.owner);
      EXPECT_EQ(previous.ordinal + 1, version.ordinal);
      EXPECT_EQ(intent.version, i);
      EXPECT_EQ(intent.predecessor, *version.predecessor);
      EXPECT_EQ(intent.value, *version.value);
      EXPECT_EQ(intent.occurrence, intent.operation->getAttr("occurrence"));
      EXPECT_TRUE(intent.selectors.empty());
      EXPECT_TRUE(intent.indices.empty());
    } else {
      ADD_FAILURE() << "straight-line fixture unexpectedly produced a join";
    }
  }
  std::vector<const TransactionCallBinding *> bumps;
  for (auto &call : plan->calls)
    if (plan->frames[call.callee]
            .method->template getAttrOfType<mlir::StringAttr>("sym_name")
            .getValue() == "bump")
      bumps.push_back(&call);
  ASSERT_EQ(bumps.size(), 3u);
  auto left = plan->frames[bumps[0]->callee].anchor;
  EXPECT_EQ(left, plan->frames[bumps[1]->callee].anchor);
  EXPECT_NE(left, plan->frames[bumps[2]->callee].anchor);
  for (unsigned i = 0; i < 3; ++i) {
    auto boundReads = reads(*plan, bumps[i]->callee);
    ASSERT_EQ(boundReads.size(), 2u); // guard read and body read.
    EXPECT_EQ(boundReads[0]->owner, boundReads[1]->owner);
    EXPECT_EQ(boundReads[0]->version, boundReads[1]->version);
    EXPECT_EQ(ordinal(*plan, boundReads[0]->version), i == 1 ? 1u : 0u);
    ASSERT_EQ(bumps[i]->entryVersions.size(), 1u);
    ASSERT_EQ(bumps[i]->exitVersions.size(), 1u);
    EXPECT_EQ(bumps[i]->entryVersions[0].version, boundReads[0]->version);
    EXPECT_EQ(ordinal(*plan, bumps[i]->exitVersions[0].version),
              i == 1 ? 2u : 1u);
    auto &guard = plan->guards[plan->frames[bumps[i]->callee].guard];
    ASSERT_EQ(guard.entryVersions.size(), 1u);
    EXPECT_EQ(guard.entryVersions[0].version, boundReads[0]->version);
  }
  auto firstResult = bumps[0]->results[0].result;
  auto firstYield = bumps[0]->results[0].yielded;
  EXPECT_NE(firstResult, firstYield);
  auto next = first<BitsBinaryOp>(method(*file).getBody()).getResult();
  EXPECT_EQ(plan->values[firstYield].value, next);
  EXPECT_EQ(plan->values[firstYield].frame, bumps[0]->callee);
  EXPECT_EQ(plan->values[firstResult].value, bumps[0]->operation->getResult(0));
  EXPECT_EQ(plan->results[0].yielded, firstResult);
  auto relay = llvm::find_if(plan->calls, [&](const auto &call) {
    return plan->frames[call.callee]
               .method->template getAttrOfType<mlir::StringAttr>("sym_name")
               .getValue() == "relay";
  });
  ASSERT_NE(relay, plan->calls.end());
  EXPECT_EQ(relay->arguments[0], firstResult);
  EXPECT_EQ(bumps[2]->arguments[0], firstResult);
  // The nested explicit actual is the relay formal, preserving its SSA scope.
  EXPECT_EQ(plan->values[bumps[1]->arguments[0]].frame, relay->callee);
  EXPECT_EQ(plan->frames[bumps[1]->callee].parent, relay->callee);
  ASSERT_EQ(relay->entryVersions.size(), 1u);
  ASSERT_EQ(relay->exitVersions.size(), 1u);
  EXPECT_EQ(ordinal(*plan, relay->entryVersions[0].version), 1u);
  EXPECT_EQ(ordinal(*plan, relay->exitVersions[0].version), 2u);
}

TEST_F(TransactionStateSSATest,
       BranchStateJoinsPreserveBothArmVersionsAndDistinctSelectedSSA) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto split = branch(body, body.begin(), body.getArgument(0),
                      body.getArgument(1), body.getArgument(2), 20);
  auto originalWrite = first<SourceStateWriteOp>(run.getBody());
  for (auto [arm, value, index] :
       {std::tuple<mlir::Region *, mlir::Value, unsigned>(
            &split.getThenRegion(), body.getArgument(1), 21),
        std::tuple<mlir::Region *, mlir::Value, unsigned>(
            &split.getElseRegion(), body.getArgument(2), 22)}) {
    auto *write = originalWrite->clone();
    write->setOperand(0, body.getArgument(0));
    write->setOperand(1, value);
    write->setAttr("occurrence", occurrence("Root", index));
    arm->front().getOperations().insert(
        arm->front().getTerminator()->getIterator(), write);
  }
  auto firstCall = *body.getOps<SourceMethodCallOp>().begin();
  firstCall->setOperand(1, split.getResult(0));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->branches.size(), 1u);
  ASSERT_EQ(plan->joins.size(), 1u);
  ASSERT_EQ(plan->branchResults.size(), 1u);
  const auto &record = plan->branches[0];
  EXPECT_EQ(record.operation, split);
  EXPECT_EQ(record.frame, plan->rootFrame);
  EXPECT_FALSE(record.entry.path.branch);
  ASSERT_EQ(record.entryVersions.size(), 1u);
  ASSERT_EQ(record.thenVersions.size(), 1u);
  ASSERT_EQ(record.elseVersions.size(), 1u);
  ASSERT_EQ(record.joins.size(), 1u);
  ASSERT_EQ(record.results.size(), 1u);
  const auto &join = plan->joins[record.joins[0]];
  EXPECT_EQ(join.branch, 0u);
  EXPECT_EQ(join.owner, record.entryVersions[0].owner);
  EXPECT_EQ(join.entry, record.entryVersions[0].version);
  EXPECT_EQ(join.whenTrue, record.thenVersions[0].version);
  EXPECT_EQ(join.whenFalse, record.elseVersions[0].version);
  EXPECT_NE(join.whenTrue, join.whenFalse);
  EXPECT_FALSE(record.selection); // State-only B1 keeps source paths without S.
  EXPECT_EQ(plan->versions[join.whenTrue].validity.path.branch, 0u);
  EXPECT_TRUE(plan->versions[join.whenTrue].validity.path.whenTrue);
  EXPECT_EQ(plan->versions[join.whenFalse].validity.path.branch, 0u);
  EXPECT_FALSE(plan->versions[join.whenFalse].validity.path.whenTrue);
  ASSERT_LT(join.result, plan->versions.size());
  const auto &joined = plan->versions[join.result];
  EXPECT_EQ(joined.kind, StateVersionKind::Join);
  EXPECT_EQ(joined.predecessor, join.entry);
  EXPECT_FALSE(joined.write);
  EXPECT_EQ(joined.join, record.joins[0]);
  EXPECT_FALSE(joined.value);
  EXPECT_FALSE(joined.validity.path.branch);
  EXPECT_GT(joined.ordinal, plan->versions[join.whenTrue].ordinal);
  EXPECT_GT(joined.ordinal, plan->versions[join.whenFalse].ordinal);

  const auto &selected = plan->branchResults[record.results[0]];
  EXPECT_EQ(selected.branch, 0u);
  EXPECT_EQ(selected.ordinal, 0u);
  EXPECT_EQ(plan->values[selected.result].value, split.getResult(0));
  EXPECT_EQ(plan->values[selected.whenTrue].value, body.getArgument(1));
  EXPECT_EQ(plan->values[selected.whenFalse].value, body.getArgument(2));
  EXPECT_NE(selected.result, selected.whenTrue);
  EXPECT_NE(selected.result, selected.whenFalse);
  EXPECT_EQ(selected.validity.prefix, record.elseExitPrefix);
  ASSERT_FALSE(plan->calls.empty());
  ASSERT_FALSE(plan->calls.front().arguments.empty());
  EXPECT_EQ(plan->calls.front().operation, firstCall);
  EXPECT_EQ(plan->calls.front().arguments.front(), selected.result);

  std::vector<TransactionStateEventKind> branchEvents;
  for (const auto &entry : plan->events)
    if (entry.branch == 0 || entry.join == record.joins[0] ||
        entry.branchResult == record.results[0])
      branchEvents.push_back(entry.kind);
  const std::vector<TransactionStateEventKind> expected = {
      TransactionStateEventKind::BranchEnter,
      TransactionStateEventKind::ThenEnter,
      TransactionStateEventKind::ThenExit,
      TransactionStateEventKind::ElseEnter,
      TransactionStateEventKind::ElseExit,
      TransactionStateEventKind::StateJoin,
      TransactionStateEventKind::BranchResult,
      TransactionStateEventKind::BranchExit};
  EXPECT_EQ(branchEvents, expected);
}

TEST_F(TransactionStateSSATest,
       NominalBranchResultPreservesWholeValueIdentityAndParentValidity) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto packet = StructType::get(&context, builder.getStringAttr("Packet"));
  auto make = [&](mlir::Value data) {
    mlir::OperationState state(run.getLoc(),
                               StructCreateOp::getOperationName());
    state.addOperands(data);
    state.addTypes(packet);
    return builder.create(state)->getResult(0);
  };
  builder.setInsertionPointToStart(&body);
  auto a = make(body.getArgument(1));
  auto b = make(body.getArgument(2));
  auto split = branch(body, std::next(b.getDefiningOp()->getIterator()),
                      body.getArgument(0), a, b, 20);
  a.getDefiningOp()->moveBefore(split.getThenRegion().front().getTerminator());
  b.getDefiningOp()->moveBefore(split.getElseRegion().front().getTerminator());
  builder.setInsertionPointAfter(split);
  mlir::OperationState projection(run.getLoc(),
                                  StructGetOp::getOperationName());
  projection.addOperands(split.getResult(0));
  projection.addTypes(body.getArgument(1).getType());
  projection.addAttribute("field", builder.getStringAttr("payload"));
  auto field = builder.create(projection)->getResult(0);
  (*body.getOps<SourceMethodCallOp>().begin())->setOperand(1, field);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->branchResults.size(), 1u);
  const auto &result = plan->branchResults.front();
  EXPECT_EQ(plan->values[result.result].value, split.getResult(0));
  EXPECT_EQ(plan->values[result.whenTrue].value, a);
  EXPECT_EQ(plan->values[result.whenFalse].value, b);
  EXPECT_NE(result.result, result.whenTrue);
  EXPECT_NE(result.result, result.whenFalse);
  EXPECT_FALSE(plan->values[result.result].validity.path.branch);
  EXPECT_EQ(plan->values[result.result].frame, plan->rootFrame);
  EXPECT_EQ(plan->values[result.whenTrue].validity.path.branch, 0u);
  EXPECT_TRUE(plan->values[result.whenTrue].validity.path.whenTrue);
  EXPECT_EQ(plan->values[result.whenFalse].validity.path.branch, 0u);
  EXPECT_FALSE(plan->values[result.whenFalse].validity.path.whenTrue);
  EXPECT_EQ(result.payloadType, packet);
  EXPECT_FALSE(plan->branches.front().selection)
      << "pure nominal selection introduces no effect-control knownness";
  split.getElseRegion().front().getTerminator()->setOperand(0, a);
  auto invalid = analyze(*file);
  EXPECT_FALSE(invalid->isValid()) << "an aggregate from the other arm is unavailable";
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(TransactionStateSSATest,
       SingleArmWriteJoinsEntryIntentAndIdentityWriteRemainsVisible) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto firstRead = *body.getOps<SourceStateReadOp>().begin();
  auto split = branch(body, std::next(firstRead->getIterator()),
                      body.getArgument(0), std::nullopt, std::nullopt, 20);
  auto originalWrite = first<SourceStateWriteOp>(run.getBody());
  auto *identity = originalWrite->clone();
  identity->setOperand(0, body.getArgument(0));
  identity->setOperand(1, firstRead.getResult());
  identity->setAttr("occurrence", occurrence("Root", 21));
  split.getThenRegion().front().getOperations().insert(
      split.getThenRegion().front().getTerminator()->getIterator(), identity);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->branches.size(), 1u);
  ASSERT_EQ(plan->joins.size(), 1u);
  const auto &record = plan->branches[0];
  const auto &join = plan->joins[record.joins[0]];
  EXPECT_NE(join.whenTrue, join.entry);
  EXPECT_EQ(join.whenFalse, join.entry);
  auto intent = llvm::find_if(plan->writes, [&](const auto &write) {
    return write.operation.operator->() == identity;
  });
  ASSERT_NE(intent, plan->writes.end());
  EXPECT_NE(intent->version, intent->predecessor);
  EXPECT_EQ(intent->value, *plan->versions[intent->version].getSourceValue());
}

TEST_F(TransactionStateSSATest,
       NestedEmptyBranchesKeepParentageAndDepthFailureIsAtomic) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto outer = branch(body, body.begin(), body.getArgument(0), std::nullopt,
                      std::nullopt, 20);
  auto &thenBlock = outer.getThenRegion().front();
  branch(thenBlock, thenBlock.getTerminator()->getIterator(),
         body.getArgument(0), std::nullopt, std::nullopt, 21);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->branches.size(), 2u);
  EXPECT_FALSE(plan->branches[0].entry.path.branch);
  EXPECT_EQ(plan->branches[1].entry.path.branch, 0u);
  EXPECT_TRUE(plan->branches[1].entry.path.whenTrue);
  EXPECT_FALSE(plan->branches[0].selection);
  EXPECT_FALSE(plan->branches[1].selection);
  EXPECT_TRUE(plan->selections.empty());
  EXPECT_EQ(plan->controls.size(), plan->guards.size());
  EXPECT_TRUE(plan->branches[0].joins.empty());
  EXPECT_TRUE(plan->branches[0].results.empty());
  EXPECT_TRUE(plan->branches[1].joins.empty());
  EXPECT_TRUE(plan->branches[1].results.empty());

  auto tooDeep = parse();
  ASSERT_TRUE(tooDeep);
  auto deepRun = transaction(*tooDeep);
  auto *block = &deepRun.getBody().front();
  for (unsigned depth = 0; depth != 129; ++depth) {
    auto nested = branch(*block, block->getTerminator()->getIterator(),
                         deepRun.getBody().front().getArgument(0), std::nullopt,
                         std::nullopt, 20 + depth);
    block = &nested.getThenRegion().front();
  }
  reject(*tooDeep);
  EXPECT_NE(llvm::StringRef(diagnostics).lower().find("depth"),
            std::string::npos)
      << diagnostics;
}

TEST_F(TransactionStateSSATest,
       RepeatedMethodBranchesUseInvocationLocalJoinsAndOwnerOrdinals) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto bump = method(*file);
  auto &body = bump.getBody().front();
  auto split = branch(body, body.begin(), body.getArgument(5), std::nullopt,
                      std::nullopt, 20, "Child");
  auto originalWrite = first<SourceStateWriteOp>(bump.getBody());
  auto *conditional = originalWrite->clone();
  conditional->setOperand(0, body.getArgument(1));
  conditional->setOperand(1, body.getArgument(0));
  conditional->setAttr("occurrence", occurrence("Child", 21));
  split.getThenRegion().front().getOperations().insert(
      split.getThenRegion().front().getTerminator()->getIterator(),
      conditional);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->branches.size(), 3u);
  ASSERT_EQ(plan->joins.size(), 3u);
  EXPECT_EQ(plan->branches[0].operation, split);
  EXPECT_EQ(plan->branches[1].operation, split);
  EXPECT_EQ(plan->branches[2].operation, split);
  EXPECT_NE(plan->branches[0].frame, plan->branches[1].frame);
  EXPECT_NE(plan->branches[1].frame, plan->branches[2].frame);
  auto left0 = plan->joins[plan->branches[0].joins[0]].owner;
  auto left1 = plan->joins[plan->branches[1].joins[0]].owner;
  auto right = plan->joins[plan->branches[2].joins[0]].owner;
  EXPECT_EQ(left0, left1);
  EXPECT_NE(left0, right);
  auto leftFirst = plan->joins[plan->branches[0].joins[0]].result;
  auto leftSecond = plan->joins[plan->branches[1].joins[0]].result;
  EXPECT_LT(plan->versions[leftFirst].ordinal,
            plan->versions[leftSecond].ordinal);
  for (const auto &record : plan->branches) {
    ASSERT_EQ(record.joins.size(), 1u);
    auto version = plan->joins[record.joins[0]].result;
    EXPECT_EQ(plan->versions[version].kind, StateVersionKind::Join);
    EXPECT_FALSE(plan->versions[version].value);
    EXPECT_TRUE(plan->versions[version].join);
  }
}

TEST_F(TransactionStateSSATest,
       ConstantFalseBranchRetainsCallsChecksAndObservations) {
  for (llvm::StringRef effect : {"call", "check", "observe"}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto root = module(*file, "Root");
    auto constants = root.getBody().front().getOps<BitsConstantOp>();
    auto no = *std::next(constants.begin());
    auto run = transaction(*file);
    llvm::SmallVector<mlir::Value> operands(run->getOperands());
    operands.push_back(no.getResult());
    run->setOperands(operands);
    for (auto &region : run->getRegions())
      region.front().addArgument(no.getResult().getType(), run.getLoc());
    auto &body = run.getBody().front();
    auto split = branch(body, body.begin(), body.getArguments().back(),
                        std::nullopt, std::nullopt, 20);
    auto &arm = split.getThenRegion().front();
    builder.setInsertionPoint(arm.getTerminator());
    if (effect == "call") {
      auto call = *body.getOps<SourceMethodCallOp>().begin();
      arm.getOperations().insert(arm.getTerminator()->getIterator(),
                                 call->clone());
    } else if (effect == "check") {
      mlir::OperationState check(run.getLoc(),
                                 SourceExpectOp::getOperationName());
      check.addOperands({body.getArguments().back(), body.getArgument(0)});
      check.addAttribute("kind", builder.getStringAttr("assert"));
      check.addAttribute("location", span("state.py", 21));
      check.addAttribute("occurrence", occurrence("Root", 21));
      builder.create(check);
    } else {
      mlir::OperationState observe(run.getLoc(),
                                   SourceObserveOp::getOperationName());
      observe.addOperands({body.getArgument(0), body.getArgument(1)});
      observe.addAttribute("kind", builder.getStringAttr("report"));
      observe.addAttribute("spec",
                           builder.getDictionaryAttr({builder.getNamedAttr(
                               "name", builder.getStringAttr("conditional"))}));
      observe.addAttribute("occurrence", occurrence("Root", 21));
      observe.addAttribute("ac.location", span("state.py", 21));
      builder.create(observe);
    }
    auto analysis = analyze(*file);
    ASSERT_TRUE(analysis->isValid()) << diagnostics;
    auto *plan = analysis->lookup(run);
    ASSERT_NE(plan, nullptr);
    ASSERT_EQ(plan->branches.size(), 1u);
    ASSERT_EQ(plan->selections.size(), 1u);
    EXPECT_EQ(plan->controls.size(), plan->guards.size() + 1);
    const auto &record = plan->branches.front();
    ASSERT_TRUE(record.selection);
    ASSERT_TRUE(record.conditionFact);
    EXPECT_FALSE(record.conditionFact.getValue());
    EXPECT_FALSE(record.entry.path.branch);
    const auto &selection = plan->selections[*record.selection];
    EXPECT_EQ(selection.branch, 0u);
    EXPECT_EQ(selection.control, record.entry.prefix);
    ASSERT_TRUE(std::holds_alternative<BranchSelectControl>(
        plan->controls[selection.control].boundary));
    EXPECT_EQ(std::get<BranchSelectControl>(
                  plan->controls[selection.control].boundary).selection,
              *record.selection);
    EXPECT_LE(record.thenExitPrefix, record.elseExitPrefix);
    if (effect == "call") {
      auto call = llvm::find_if(plan->calls, [&](const auto &candidate) {
        return candidate.operation->getParentOp() == split;
      });
      ASSERT_NE(call, plan->calls.end());
      const auto &frame = plan->frames[call->callee];
      EXPECT_EQ(frame.entry.path.branch, 0u);
      EXPECT_TRUE(frame.entry.path.whenTrue);
      EXPECT_EQ(frame.exit.path.branch, 0u);
      EXPECT_TRUE(frame.exit.path.whenTrue);
      EXPECT_LT(frame.entry.prefix, frame.exit.prefix);
      for (const auto &result : call->results) {
        EXPECT_NE(result.result, result.yielded);
        EXPECT_EQ(plan->values[result.result].validity.prefix,
                  frame.exit.prefix);
        EXPECT_EQ(plan->values[result.result].validity.path.branch, 0u);
      }
    } else if (effect == "check") {
      ASSERT_EQ(plan->bodyChecks.size(), 1u);
      EXPECT_EQ(plan->bodyChecks.front().point.path.branch, 0u);
      EXPECT_TRUE(plan->bodyChecks.front().point.path.whenTrue);
    } else {
      ASSERT_EQ(plan->observations.size(), 1u);
      EXPECT_EQ(plan->observations.front().point.path.branch, 0u);
      EXPECT_TRUE(plan->observations.front().point.path.whenTrue);
    }
    AdmissionPrefix previous = 0;
    for (const auto &entry : plan->events) {
      EXPECT_GE(entry.point.prefix, previous);
      previous = entry.point.prefix;
    }
  }
}

TEST_F(TransactionStateSSATest,
       EveryCallClassSelectsAllAncestorBranchesAndRetainsFrameBijection) {
  for (llvm::StringRef callee : {"noop", "peek", "bump"}) {
    SCOPED_TRACE(callee.str());
    auto file = parse();
    ASSERT_TRUE(file);
    auto run = transaction(*file);
    auto &body = run.getBody().front();
    auto outer = branch(body, body.begin(), body.getArgument(0),
                        std::nullopt, std::nullopt, 20);
    auto &outerArm = outer.getThenRegion().front();
    auto inner = branch(outerArm, outerArm.getTerminator()->getIterator(),
                        body.getArgument(0), std::nullopt, std::nullopt, 21);
    auto &innerArm = inner.getElseRegion().front();
    auto exemplar = *body.getOps<SourceMethodCallOp>().begin();
    mlir::OperationState call(run.getLoc(), SourceMethodCallOp::getOperationName());
    call.addAttributes(exemplar->getAttrs());
    call.attributes.set("callee", mlir::SymbolRefAttr::get(
                                     &context, "Child",
                                     {mlir::FlatSymbolRefAttr::get(&context,
                                                                   callee)}));
    call.attributes.set("occurrence", occurrence("Root", 22));
    call.addOperands(body.getArgument(0));
    if (callee == "bump")
      call.addOperands(body.getArgument(1));
    if (callee != "noop")
      call.addTypes(body.getArgument(1).getType());
    builder.setInsertionPoint(innerArm.getTerminator());
    auto authored = mlir::cast<SourceMethodCallOp>(builder.create(call));
    auto analysis = analyze(*file);
    ASSERT_TRUE(analysis->isValid()) << diagnostics;
    auto *plan = analysis->lookup(run);
    ASSERT_NE(plan, nullptr);
    ASSERT_EQ(plan->branches.size(), 2u);
    ASSERT_EQ(plan->selections.size(), 2u);
    EXPECT_EQ(plan->controls.size(), plan->guards.size() + plan->selections.size());
    EXPECT_EQ(plan->frames.size(), plan->guards.size());
    for (InvocationFrameId frame = 0; frame < plan->frames.size(); ++frame) {
      auto guard = plan->frames[frame].guard;
      ASSERT_LT(guard, plan->guards.size());
      EXPECT_EQ(plan->guards[guard].frame, frame);
      auto control = plan->guards[guard].control;
      ASSERT_LT(control, plan->controls.size());
      ASSERT_TRUE(std::holds_alternative<CallableGuardControl>(
          plan->controls[control].boundary));
      EXPECT_EQ(std::get<CallableGuardControl>(plan->controls[control].boundary)
                    .guard, guard);
    }
    EXPECT_TRUE(plan->branches[0].selection);
    EXPECT_TRUE(plan->branches[1].selection);
    EXPECT_EQ(plan->branches[1].entry.path.branch, 0u);
    EXPECT_TRUE(plan->branches[1].entry.path.whenTrue);
    auto expanded = llvm::find_if(plan->calls, [&](const auto &candidate) {
      return candidate.operation == authored;
    });
    ASSERT_NE(expanded, plan->calls.end());
    const auto &frame = plan->frames[expanded->callee];
    EXPECT_EQ(frame.entry.path.branch, 1u);
    EXPECT_FALSE(frame.entry.path.whenTrue);
    EXPECT_EQ(frame.exit.path.branch, 1u);
    EXPECT_FALSE(frame.exit.path.whenTrue);
    EXPECT_LT(frame.entry.prefix, frame.exit.prefix);
    for (const auto &result : expanded->results) {
      EXPECT_NE(result.result, result.yielded);
      EXPECT_EQ(plan->values[result.result].validity.prefix, frame.exit.prefix);
      EXPECT_EQ(plan->values[result.result].validity.path.branch, 1u);
    }
    AdmissionPrefix previous = 0;
    for (const auto &event : plan->events) {
      EXPECT_GE(event.point.prefix, previous);
      EXPECT_LE(event.point.prefix, plan->controls.size());
      previous = event.point.prefix;
      unsigned payloads = unsigned(bool(event.read)) + unsigned(bool(event.write)) +
          unsigned(bool(event.guard)) + unsigned(bool(event.call)) +
          unsigned(bool(event.selection)) + unsigned(bool(event.bodyCheck)) +
          unsigned(bool(event.observation)) + unsigned(bool(event.branch)) +
          unsigned(bool(event.join)) + unsigned(bool(event.branchResult));
      EXPECT_EQ(payloads,
                event.kind == TransactionStateEventKind::TransactionEnter ||
                        event.kind == TransactionStateEventKind::TransactionExit
                    ? 0u : 1u);
    }
    EXPECT_EQ(previous, plan->controls.size());
  }
}

TEST_F(TransactionStateSSATest,
       AdmissionPrefixesTrackNestedCallsReadsWritesAndSavedValues) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->guards.size(), 7u);
  ASSERT_EQ(plan->calls.size(), 6u);

  AdmissionPrefix previous = 0;
  for (const auto &entry : plan->events) {
    EXPECT_GE(entry.point.prefix, previous);
    EXPECT_LE(entry.point.prefix, plan->guards.size());
    previous = entry.point.prefix;
  }
  auto *transactionEnter =
      event(*plan, TransactionStateEventKind::TransactionEnter);
  auto *transactionExit =
      event(*plan, TransactionStateEventKind::TransactionExit);
  ASSERT_NE(transactionEnter, nullptr);
  ASSERT_NE(transactionExit, nullptr);
  EXPECT_EQ(transactionEnter->point.prefix, 0u);
  EXPECT_EQ(transactionExit->point.prefix, plan->guards.size());

  for (GuardBoundaryId id = 0; id < plan->guards.size(); ++id) {
    auto *enter = event(*plan, TransactionStateEventKind::GuardEnter, id);
    auto *exit = event(*plan, TransactionStateEventKind::GuardExit, id);
    ASSERT_NE(enter, nullptr);
    ASSERT_NE(exit, nullptr);
    ASSERT_TRUE(plan->guards[id].predicateFact);
    EXPECT_TRUE(plan->guards[id].predicateFact.getValue());
    EXPECT_EQ(enter->point.prefix, id);
    EXPECT_EQ(exit->point.prefix, id + 1);
  }
  for (CallBindingId id = 0; id < plan->calls.size(); ++id) {
    auto *enter = event(*plan, TransactionStateEventKind::CallEnter, id);
    auto *exit = event(*plan, TransactionStateEventKind::CallExit, id);
    ASSERT_NE(enter, nullptr);
    ASSERT_NE(exit, nullptr);
    EXPECT_EQ(enter->point.prefix, plan->guards[plan->frames[plan->calls[id].callee].guard].control);
    EXPECT_GT(exit->point.prefix, enter->point.prefix);
    for (const auto &result : plan->calls[id].results)
      EXPECT_EQ(exit->point.prefix, plan->values[result.result].validity.prefix);
  }

  for (StateReadId id = 0; id < plan->reads.size(); ++id) {
    const auto &read = plan->reads[id];
    auto *readEvent = event(*plan, TransactionStateEventKind::Read, id);
    ASSERT_NE(readEvent, nullptr);
    ASSERT_LT(read.version, plan->versions.size());
    ASSERT_LT(read.result, plan->values.size());
    EXPECT_EQ(plan->values[read.result].validity.prefix,
              std::max(readEvent->point.prefix,
                       plan->versions[read.version].validity.prefix));
  }
  for (StateWriteId id = 0; id < plan->writes.size(); ++id) {
    const auto &write = plan->writes[id];
    auto *writeEvent = event(*plan, TransactionStateEventKind::Write, id);
    ASSERT_NE(writeEvent, nullptr);
    ASSERT_LT(write.version, plan->versions.size());
    ASSERT_LT(write.value, plan->values.size());
    const auto &version = plan->versions[write.version];
    EXPECT_EQ(
        version.validity.prefix,
        std::max(writeEvent->point.prefix, plan->values[write.value].validity.prefix));
    auto *defining = plan->values[write.value].value.getDefiningOp();
    if (defining && mlir::isa<BitsBinaryOp>(defining))
      EXPECT_EQ(plan->values[write.value].validity.prefix, writeEvent->point.prefix)
          << "pure payload results are defined under the current admission";
  }

  auto relay = llvm::find_if(plan->calls, [&](const auto &call) {
    return plan->frames[call.callee]
               .method->template getAttrOfType<mlir::StringAttr>("sym_name")
               .getValue() == "relay";
  });
  ASSERT_NE(relay, plan->calls.end());
  ASSERT_EQ(relay->results.size(), 1u);
  auto relayId = std::distance(plan->calls.begin(), relay);
  auto *relayExit = event(*plan, TransactionStateEventKind::CallExit, relayId);
  ASSERT_NE(relayExit, nullptr);
  EXPECT_EQ(relayExit->point.prefix, 4u); // relay guard plus its nested bump guard.
  EXPECT_EQ(plan->values[relay->results[0].yielded].validity.prefix,
            relayExit->point.prefix);
  EXPECT_EQ(plan->values[relay->results[0].result].validity.prefix,
            relayExit->point.prefix);

  auto firstResult = plan->calls.front().results.front().result;
  EXPECT_EQ(plan->values[firstResult].validity.prefix, 2u);
  EXPECT_EQ(relay->arguments.front(), firstResult);
  EXPECT_EQ(plan->values[firstResult].validity.prefix, 2u)
      << "later calls must not resample an already bound result";
  ASSERT_EQ(plan->results.size(), 1u);
  EXPECT_EQ(plan->values[plan->results.front().result].validity.prefix,
            plan->guards.size());
}

TEST_F(TransactionStateSSATest,
       PrefixAliasesPinsLexicalCapturesAndOldQKeepTheirActualAuthority) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  auto root = module(*file, "Root");
  auto &rootBlock = root.getBody().front();

  ASSERT_LT(plan->clockValue, plan->values.size());
  ASSERT_LT(plan->resetValue, plan->values.size());
  EXPECT_EQ(plan->values[plan->clockValue].value, rootBlock.getArgument(2));
  EXPECT_EQ(plan->values[plan->resetValue].value, rootBlock.getArgument(3));
  EXPECT_EQ(plan->values[plan->clockValue].anchor,
            plan->frames[plan->rootFrame].anchor);
  EXPECT_EQ(plan->values[plan->resetValue].anchor,
            plan->frames[plan->rootFrame].anchor);
  EXPECT_EQ(plan->values[plan->clockValue].validity.prefix, 0u);
  EXPECT_EQ(plan->values[plan->resetValue].validity.prefix, 0u);

  for (const auto &version : plan->versions)
    if (version.kind == StateVersionKind::OldQ) {
      EXPECT_EQ(version.validity.prefix, 0u);
      ASSERT_TRUE(version.value);
      ASSERT_LT(*version.value, plan->values.size());
      EXPECT_EQ(plan->values[*version.value].validity.prefix, 0u);
    }
  for (const auto &call : plan->calls)
    for (BoundValueId capture : call.captures) {
      ASSERT_LT(capture, plan->values.size());
      EXPECT_EQ(plan->values[capture].validity.prefix, 0u);
    }

  auto relay = llvm::find_if(plan->calls, [&](const auto &call) {
    return plan->frames[call.callee]
               .method->template getAttrOfType<mlir::StringAttr>("sym_name")
               .getValue() == "relay";
  });
  ASSERT_NE(relay, plan->calls.end());
  ASSERT_EQ(relay->arguments.size(), 1u);
  auto actualPrefix = plan->values[relay->arguments.front()].validity.prefix;
  EXPECT_EQ(actualPrefix, 2u);
  const auto &relayFrame = plan->frames[relay->callee];
  for (const auto &binding :
       {relayFrame.guardArguments.front(), relayFrame.bodyArguments.front()}) {
    const auto *formal =
        value(*plan, binding.formal, relayFrame.anchor, relay->callee);
    ASSERT_NE(formal, nullptr);
    EXPECT_EQ(binding.actual, relay->arguments.front());
    EXPECT_EQ(formal->validity.prefix, actualPrefix);
  }

  auto nested = llvm::find_if(plan->calls, [&](const auto &call) {
    return call.caller == relay->callee;
  });
  ASSERT_NE(nested, plan->calls.end());
  ASSERT_EQ(nested->arguments.size(), 1u);
  EXPECT_EQ(plan->values[nested->arguments.front()].validity.prefix,
            actualPrefix);
  const auto &nestedFrame = plan->frames[nested->callee];
  for (const auto &binding : {nestedFrame.guardArguments.front(),
                              nestedFrame.bodyArguments.front()}) {
    const auto *formal =
        value(*plan, binding.formal, nestedFrame.anchor, nested->callee);
    ASSERT_NE(formal, nullptr);
    EXPECT_EQ(formal->validity.prefix, actualPrefix);
  }
}

TEST_F(TransactionStateSSATest,
       ConstantWriteBecomesConditionalAtItsActualWritePoint) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto root = module(*file, "Root");
  auto zero =
      *std::next(root.getBody().front().getOps<BitsConstantOp>().begin(), 2);
  auto write = first<SourceStateWriteOp>(run.getBody());
  llvm::SmallVector<mlir::Value> captures(run->getOperands());
  captures.push_back(zero.getResult());
  run->setOperands(captures);
  for (auto &region : run->getRegions())
    region.front().addArgument(zero.getResult().getType(), run.getLoc());
  auto capturedZero = run.getBody().front().getArguments().back();
  write->setOperand(1, capturedZero);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  auto intent = llvm::find_if(plan->writes, [&](const auto &candidate) {
    return candidate.operation == write;
  });
  ASSERT_NE(intent, plan->writes.end());
  auto id = std::distance(plan->writes.begin(), intent);
  auto *writeEvent = event(*plan, TransactionStateEventKind::Write, id);
  ASSERT_NE(writeEvent, nullptr);
  EXPECT_EQ(writeEvent->point.prefix, 5u);
  EXPECT_EQ(plan->values[intent->value].value, capturedZero);
  EXPECT_EQ(plan->values[intent->value].validity.prefix, 0u);
  const auto &rootFrame = plan->frames[plan->rootFrame];
  auto binding = llvm::find_if(rootFrame.bodyArguments, [&](const auto &entry) {
    return entry.formal == capturedZero;
  });
  ASSERT_NE(binding, rootFrame.bodyArguments.end());
  EXPECT_EQ(plan->values[binding->actual].value, zero.getResult());
  EXPECT_EQ(plan->values[binding->actual].validity.prefix, 0u);
  EXPECT_EQ(plan->versions[intent->version].validity.prefix, writeEvent->point.prefix);
  auto laterRead = llvm::find_if(plan->reads, [&](const auto &candidate) {
    return candidate.frame == plan->rootFrame &&
           candidate.version == intent->version;
  });
  ASSERT_NE(laterRead, plan->reads.end());
  EXPECT_EQ(plan->values[laterRead->result].validity.prefix, writeEvent->point.prefix);
}

TEST_F(TransactionStateSSATest,
       GuardBoundariesRetainCallableSpansAcrossRepeatedCrossSourceCalls) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  child->setAttr("source_owner", sourceOwner("child.py"));
  unsigned line = 10;
  for (auto callable : child.getBody().front().getOps<SourceMethodOp>()) {
    auto location = span("child.py", line++);
    callable->setAttr("ac.guard_location", location);
    llvm::SmallVector<mlir::Attribute> parameters;
    for (auto raw : callable->getAttrOfType<mlir::ArrayAttr>("ac.parameters")) {
      mlir::NamedAttrList fields(mlir::cast<mlir::DictionaryAttr>(raw));
      fields.set("location", location);
      parameters.push_back(fields.getDictionary(&context));
    }
    callable->setAttr("ac.parameters", builder.getArrayAttr(parameters));
  }
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto run = transaction(*file);
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  auto bump = method(*file, "bump");
  auto bumpLocation =
      bump->getAttrOfType<mlir::DictionaryAttr>("ac.guard_location");
  ASSERT_TRUE(bumpLocation);
  EXPECT_EQ(bumpLocation.getAs<mlir::StringAttr>("path").getValue(),
            "child.py");
  EXPECT_EQ(run->getAttrOfType<mlir::DictionaryAttr>("ac.guard_location")
                .getAs<mlir::StringAttr>("path")
                .getValue(),
            "state.py");
  unsigned repeatedBumpBoundaries = 0;
  for (const auto &boundary : plan->guards) {
    ASSERT_NE(boundary.callable, nullptr);
    EXPECT_EQ(boundary.location,
              boundary.callable->getAttrOfType<mlir::DictionaryAttr>(
                  "ac.guard_location"));
    if (boundary.callable == bump.getOperation()) {
      EXPECT_EQ(boundary.location, bumpLocation);
      ++repeatedBumpBoundaries;
    }
  }
  EXPECT_EQ(repeatedBumpBoundaries, 3u);
}

TEST_F(TransactionStateSSATest,
       IdentityWritesUnusedReadsAndLexicalOldQCapturesRemainDistinct) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  auto rootReads = reads(*plan, plan->rootFrame);
  ASSERT_EQ(rootReads.size(), 3u);
  EXPECT_EQ(ordinal(*plan, rootReads[0]->version), 0u);
  EXPECT_EQ(ordinal(*plan, rootReads[1]->version), 0u);
  EXPECT_EQ(ordinal(*plan, rootReads[2]->version), 1u);
  auto intent = llvm::find_if(plan->writes, [&](const auto &write) {
    return write.frame == plan->rootFrame;
  });
  ASSERT_NE(intent, plan->writes.end());
  EXPECT_EQ(intent->value, rootReads[1]->result);
  EXPECT_EQ(intent->predecessor, rootReads[1]->version);
  EXPECT_NE(intent->version, intent->predecessor);
  auto peek = llvm::find_if(plan->calls, [&](const auto &call) {
    return plan->frames[call.callee]
               .method->template getAttrOfType<mlir::StringAttr>("sym_name")
               .getValue() == "peek";
  });
  ASSERT_NE(peek, plan->calls.end());
  EXPECT_TRUE(peek->operation->use_empty());
  auto peekReads = reads(*plan, peek->callee);
  ASSERT_EQ(peekReads.size(), 2u);
  EXPECT_EQ(ordinal(*plan, peekReads[1]->version), 2u);
  ASSERT_EQ(peek->captures.size(), 2u);
  auto &capture = plan->values[peek->captures[1]];
  EXPECT_EQ(capture.value, method(*file, "peek").getOperand(1));
  EXPECT_EQ(capture.frame, plan->rootFrame);
  EXPECT_EQ(capture.anchor, plan->frames[peek->callee].anchor);
  EXPECT_NE(capture.anchor, plan->frames[plan->rootFrame].anchor);
  EXPECT_EQ(plan->values[peek->results[0].yielded].value,
            method(*file, "peek").getBody().front().getArgument(1));
  EXPECT_EQ(plan->frames[peek->callee].bodyArguments[1].actual,
            peek->captures[1]);
  auto noop = llvm::find_if(plan->calls, [&](const auto &call) {
    return plan->frames[call.callee]
               .method->template getAttrOfType<mlir::StringAttr>("sym_name")
               .getValue() == "noop";
  });
  ASSERT_NE(noop, plan->calls.end());
  EXPECT_TRUE(noop->results.empty());
  EXPECT_TRUE(noop->entryVersions.empty());
  EXPECT_TRUE(noop->exitVersions.empty());
  EXPECT_EQ(plan->guards[plan->frames[noop->callee].guard].domain, plan->domain);
}

TEST_F(TransactionStateSSATest,
       EachRuleStartsOldQAndRegistrationOrderIsIrrelevant) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file), watch = transaction(*file, "watch");
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(watch);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->owners.size(), 2u);
  EXPECT_TRUE(plan->writes.empty());
  for (auto &read : plan->reads)
    EXPECT_EQ(ordinal(*plan, read.version), 0u);
  auto before = plan->reads.size();
  auto ownerOccurrences = std::vector<mlir::DictionaryAttr>{};
  for (auto &owner : plan->owners)
    ownerOccurrences.push_back(owner.occurrence);
  watch->moveBefore(run);
  auto reordered = analyze(*file);
  ASSERT_TRUE(reordered->isValid()) << diagnostics;
  plan = reordered->lookup(watch);
  ASSERT_NE(plan, nullptr);
  EXPECT_EQ(plan->reads.size(), before);
  for (unsigned i = 0; i < plan->owners.size(); ++i)
    EXPECT_EQ(plan->owners[i].occurrence, ownerOccurrences[i]);
  for (auto &read : plan->reads)
    EXPECT_EQ(ordinal(*plan, read.version), 0u);
  EXPECT_EQ(reordered->lookup(run)->writes.size(), 4u);
}

TEST_F(TransactionStateSSATest,
       FalseAndUnknownGuardsRetainOrderedStaticObligations) {
  for (bool falseFirst : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto bump = method(*file);
    // Non-total admission may retain ordinary internal call values, but no
    // ordinary result may escape this outer Rule's admission boundary.
    auto root = module(*file, "Root");
    auto run = transaction(*file);
    root.getBody().front().getTerminator()->setOperand(
        0, root.getBody().front().getArgument(0));
    run.getBody().front().getTerminator()->setOperands(mlir::ValueRange{});
    mlir::OperationState voidState(run.getLoc(),
                                   SourceTransactionOp::getOperationName());
    voidState.addOperands(run->getOperands());
    voidState.addAttributes(run->getAttrs());
    voidState.addRegion();
    voidState.addRegion();
    builder.setInsertionPoint(run);
    auto voidRun = mlir::cast<SourceTransactionOp>(builder.create(voidState));
    voidRun.getGuard().takeBody(run.getGuard());
    voidRun.getBody().takeBody(run.getBody());
    run->erase();
    // Unknown comes from a hardware input, false from an existing literal.
    auto falseValue =
        *std::next(child.getBody().front().getOps<BitsConstantOp>().begin());
    mlir::Value falsePredicate = falseValue.getResult();
    mlir::Value unknownPredicate = child.getBody().front().getArgument(1);
    bump->setOperand(0, falseFirst ? falsePredicate : unknownPredicate);
    // Effects retain true paths, independently of admission predicates.
    for (auto &region : bump->getRegions()) {
      auto &block = region.front();
      auto *literal = falseValue->clone();
      literal->setAttr(
          "value", (*child.getBody().front().getOps<BitsConstantOp>().begin())
                       .getValue());
      block.getOperations().insert(block.begin(), literal);
      for (auto &op : block)
        if (mlir::isa<SourceStateReadOp, SourceStateWriteOp>(op))
          op.setOperand(0, literal->getResult(0));
    }
    auto noop = method(*file, "noop");
    noop->setOperand(0, falseFirst ? unknownPredicate : falsePredicate);
    auto analysis = analyze(*file);
    ASSERT_TRUE(analysis->isValid()) << diagnostics;
    auto *plan = analysis->lookup(transaction(*file));
    ASSERT_NE(plan, nullptr);
    EXPECT_EQ(plan->writes.size(), 4u);
    EXPECT_EQ(plan->calls.size(), 6u);
    ASSERT_EQ(plan->guards.size(), 7u);
    for (unsigned i = 1; i < plan->guards.size(); ++i)
      EXPECT_EQ(plan->guards[i].control, i);
    for (GuardBoundaryId id = 0; id < plan->guards.size(); ++id) {
      auto *enter = event(*plan, TransactionStateEventKind::GuardEnter, id);
      auto *exit = event(*plan, TransactionStateEventKind::GuardExit, id);
      ASSERT_NE(enter, nullptr);
      ASSERT_NE(exit, nullptr);
      EXPECT_EQ(enter->point.prefix, id);
      EXPECT_EQ(exit->point.prefix, id + 1);
    }
    auto &firstGuard = plan->guards[1];
    auto &lastGuard = plan->guards.back();
    if (falseFirst) {
      ASSERT_TRUE(firstGuard.predicateFact);
      EXPECT_FALSE(firstGuard.predicateFact.getValue());
      EXPECT_FALSE(lastGuard.predicateFact);
    } else {
      EXPECT_FALSE(firstGuard.predicateFact);
      ASSERT_TRUE(lastGuard.predicateFact);
      EXPECT_FALSE(lastGuard.predicateFact.getValue());
    }
    EXPECT_EQ(plan->events.front().kind,
              TransactionStateEventKind::TransactionEnter);
    EXPECT_EQ(plan->events.back().kind,
              TransactionStateEventKind::TransactionExit);
    for (auto &call : plan->calls) {
      bool entered = false, guarded = false, exited = false;
      for (auto &event : plan->events) {
        if (event.call && &plan->calls[*event.call] == &call) {
          EXPECT_EQ(event.frame, call.caller);
          if (event.kind == TransactionStateEventKind::CallEnter)
            entered = true;
          if (event.kind == TransactionStateEventKind::CallExit) {
            EXPECT_TRUE(entered);
            EXPECT_TRUE(guarded);
            exited = true;
          }
        }
        if (event.guard && *event.guard == plan->frames[call.callee].guard) {
          EXPECT_TRUE(entered);
          EXPECT_FALSE(exited);
          EXPECT_EQ(event.frame, call.callee);
          guarded = true;
        }
      }
      EXPECT_TRUE(exited);
    }
  }
}

TEST_F(TransactionStateSSATest,
       WholeNominalAggregateHasOneOwnerAndUnmodifiedPayloadSSA) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto bump = method(*file);
  auto &body = bump.getBody().front();
  auto packet = *std::next(
      module(*file, "Child").getBody().front().getOps<SourceStateOp>().begin());
  auto countRead = first<SourceStateReadOp>(bump.getBody());
  auto *read = countRead->clone();
  read->setAttr("state", builder.getStringAttr("packet"));
  read->setAttr("occurrence", occurrence("Child", 20));
  read->getResult(0).setType(packet.getQ().getType());
  body.getOperations().insert(body.getTerminator()->getIterator(), read);
  auto *write = first<SourceStateWriteOp>(bump.getBody())->clone();
  write->setAttr("state", builder.getStringAttr("packet"));
  write->setAttr("occurrence", occurrence("Child", 21));
  write->setOperand(1, read->getResult(0));
  body.getOperations().insert(body.getTerminator()->getIterator(), write);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  unsigned aggregateOwners = 0, aggregateWrites = 0;
  for (auto &owner : plan->owners) {
    if (owner.declaration == packet) {
      ++aggregateOwners;
      EXPECT_EQ(owner.payloadType, packet.getQ().getType());
      EXPECT_TRUE(mlir::isa<StructType>(owner.payloadType));
    }
  }
  for (auto &intent : plan->writes) {
    if (intent.operation == write) {
      ++aggregateWrites;
      auto &version = plan->versions[intent.version];
      EXPECT_EQ(version.payloadType, packet.getQ().getType());
      ASSERT_TRUE(version.value);
      EXPECT_EQ(*version.value, intent.value);
      EXPECT_EQ(plan->values[intent.value].value, read->getResult(0));
      EXPECT_TRUE(intent.selectors.empty());
      EXPECT_TRUE(intent.indices.empty());
    }
  }
  EXPECT_EQ(aggregateOwners, 2u);
  EXPECT_EQ(aggregateWrites, 3u);
}

TEST_F(TransactionStateSSATest,
       ActualInstanceInputsNormalizeBothClockAndReset) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  EXPECT_EQ(plan->domain.clock.value, root.getBody().front().getArgument(2));
  EXPECT_EQ(plan->domain.reset.value, root.getBody().front().getArgument(3));
  EXPECT_EQ(plan->domain.clock.anchor, plan->frames[plan->rootFrame].anchor);
  for (auto &owner : plan->owners)
    EXPECT_EQ(owner.domain, plan->domain);
  for (auto &guard : plan->guards)
    EXPECT_EQ(guard.domain, plan->domain);
  for (auto &frame : plan->frames) {
    for (auto binding : frame.guardArguments)
      EXPECT_LT(binding.actual, plan->values.size());
    for (auto binding : frame.bodyArguments)
      EXPECT_LT(binding.actual, plan->values.size());
  }
  // A reset mismatch remains a mismatch even when the clock agrees.
  auto instance = *root.getBody().front().getOps<InstanceOp>().begin();
  instance->setOperand(3, root.getBody().front().getArgument(1));
  reject(*file);
}

TEST_F(TransactionStateSSATest,
       EqualLiteralValuesAtDifferentSSASitesDoNotProveDomains) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto yes = *root.getBody().front().getOps<BitsConstantOp>().begin();
  for (auto op : root.getBody().front().getOps<SourceStateOp>())
    op->setOperand(0, yes.getResult());
  for (auto op : root.getBody().front().getOps<InstanceOp>())
    op->setOperand(2, yes.getResult());
  for (auto op : root.getBody().front().getOps<SourceTransactionOp>())
    op->setOperand(0, yes.getResult());
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *other = yes->clone();
  root.getBody().front().getOperations().insert(yes->getIterator(), other);
  (*root.getBody().front().getOps<InstanceOp>().begin())
      ->setOperand(2, other->getResult(0));
  reject(*file);
}

TEST_F(TransactionStateSSATest,
       ReceiverDomainIsRequiredEvenForGuardOnlyVoidCalls) {
  auto file = parse();
  ASSERT_TRUE(file);
  // Remove all stateful calls, leaving the void, stateless Method.
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto *yield = body.getTerminator();
  yield->setOperand(0, body.getArgument(1));
  std::vector<mlir::Operation *> remove;
  for (auto &op : body)
    if (auto call = mlir::dyn_cast<SourceMethodCallOp>(&op))
      if (call.getCallee().getNestedReferences().back().getValue() != "noop")
        remove.push_back(&op);
  for (auto *op : llvm::reverse(remove))
    op->erase();
  // watch retains peek, which is also read-only; no missing-domain bypass.
  module(*file, "Child")->removeAttr("ac.domain_inputs");
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  reject(*file);
}

TEST_F(TransactionStateSSATest,
       ConditionalEffectPathsFailWithoutAnyPublishedPlan) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto run = transaction(*file);
    auto &body = run.getBody().front();
    auto root = module(*file, "Root");
    auto no =
        *std::next(root.getBody().front().getOps<BitsConstantOp>().begin());
    auto *effect = mutation == 2
                       ? first<SourceMethodCallOp>(run.getBody()).getOperation()
                       : first<SourceStateReadOp>(run.getBody()).getOperation();
    if (mutation == 1) {
      // Existing capture slot is b1 and can carry an unproved input.
      run->setOperand(2, root.getBody().front().getArgument(1));
      effect->setOperand(0, body.getArgument(0));
    } else {
      auto *literal = no->clone();
      body.getOperations().insert(body.begin(), literal);
      effect->setOperand(0, literal->getResult(0));
    }
    reject(*file);
  }
}

TEST_F(TransactionStateSSATest, PartialSelectorAndMissingOwnerFailClosed) {
  for (bool selector : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto bump = method(*file);
    auto read = first<SourceStateReadOp>(bump.getBody());
    if (selector) {
      // A legal field projection is outside the whole-owner analysis slice.
      read->setAttr("state", builder.getStringAttr("packet"));
      read->setAttr(
          "selectors",
          builder.getArrayAttr({builder.getDictionaryAttr({builder.getNamedAttr(
              "field", builder.getStringAttr("payload"))})}));
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
    } else {
      read->setAttr("state", builder.getStringAttr("absent"));
    }
    reject(*file);
  }
}

TEST_F(TransactionStateSSATest,
       MalformedMetadataAndCallableRegionsRejectAtomically) {
  for (unsigned mutation = 0; mutation < 6; ++mutation) {
    SCOPED_TRACE(mutation);
    auto file = parse();
    ASSERT_TRUE(file);
    auto bump = method(*file);
    switch (mutation) {
    case 0:
      bump->setAttr("occurrence", builder.getDictionaryAttr({}));
      break;
    case 1:
      bump.getBody().getBlocks().clear();
      break;
    case 2:
      bump.getGuard().getBlocks().clear();
      break;
    case 3:
      bump.getBody().front().addArgument(bump.getOperand(0).getType(),
                                         bump.getLoc());
      break;
    case 4:
      bump->setAttr("ac.result_constraints", builder.getArrayAttr({}));
      break;
    case 5:
      first<SourceMethodCallOp>(transaction(*file).getBody())
          ->setAttr("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Child",
                        {mlir::FlatSymbolRefAttr::get(&context, "absent")}));
      break;
    }
    reject(*file);
  }
}

TEST_F(TransactionStateSSATest,
       SuccessPreservesIRIdentityAndDumpIsDeterministic) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto text = print(file->getOperation());
  auto ops = identities(*file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto firstDump = dump(*analysis);
  EXPECT_FALSE(firstDump.empty());
  EXPECT_EQ(text, print(file->getOperation()));
  EXPECT_EQ(ops, identities(*file));
  auto another = analyze(*file);
  ASSERT_TRUE(another->isValid()) << diagnostics;
  EXPECT_EQ(firstDump, dump(*another));
  auto independent = parse();
  ASSERT_TRUE(independent);
  auto rebuilt = analyze(*independent);
  ASSERT_TRUE(rebuilt->isValid()) << diagnostics;
  EXPECT_EQ(firstDump, dump(*rebuilt));
  EXPECT_EQ(analysis->lookup(transaction(*independent)), nullptr);
  // Address spellings must not escape through the stable analysis dump.
  EXPECT_EQ(firstDump.find("0x"), std::string::npos);
}

TEST_F(TransactionStateSSATest, AnalysisManagerInvalidatesAfterIRMutation) {
  auto file = parse();
  ASSERT_TRUE(file);
  mlir::ModuleAnalysisManager manager(file->getOperation(), nullptr);
  mlir::AnalysisManager am = manager;
  auto &analysis = capture([&]() -> SourceTransactionStateAnalysis & {
    return am.getAnalysis<SourceTransactionStateAnalysis>();
  });
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  EXPECT_TRUE(
      am.getCachedAnalysis<SourceTransactionStateAnalysis>().has_value());
  auto run = transaction(*file);
  first<SourceStateReadOp>(run.getBody())
      ->setAttr("state", builder.getStringAttr("missing"));
  am.invalidate(mlir::detail::PreservedAnalyses());
  EXPECT_FALSE(
      am.getCachedAnalysis<SourceTransactionStateAnalysis>().has_value());
  auto &rebuilt = capture([&]() -> SourceTransactionStateAnalysis & {
    return am.getAnalysis<SourceTransactionStateAnalysis>();
  });
  EXPECT_FALSE(rebuilt.isValid());
  EXPECT_EQ(rebuilt.lookup(run), nullptr);
}

TEST_F(TransactionStateSSATest,
       AnalysisManagerRebuildsBodyEffectOperandsAndMetadataAfterMutation) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto &body = run.getBody().front();
  auto read = first<SourceStateReadOp>(run.getBody());
  builder.setInsertionPoint(body.getTerminator());
  mlir::OperationState state(run.getLoc(), SourceObserveOp::getOperationName());
  state.addOperands({body.getArgument(0), body.getArgument(1)});
  state.addAttribute("kind", builder.getStringAttr("report"));
  auto beforeSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("name", builder.getStringAttr("before"))});
  state.addAttribute("spec", beforeSpec);
  state.addAttribute("occurrence", occurrence("Root", 30));
  state.addAttribute("ac.location", span("state.py", 30));
  auto observation = mlir::cast<SourceObserveOp>(builder.create(state));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  mlir::ModuleAnalysisManager manager(file->getOperation(), nullptr);
  mlir::AnalysisManager am = manager;
  auto &initial = capture([&]() -> SourceTransactionStateAnalysis & {
    return am.getAnalysis<SourceTransactionStateAnalysis>();
  });
  ASSERT_TRUE(initial.isValid()) << diagnostics;
  auto *initialPlan = initial.lookup(run);
  ASSERT_NE(initialPlan, nullptr);
  ASSERT_EQ(initialPlan->observations.size(), 1u);
  EXPECT_EQ(initialPlan->values[initialPlan->observations[0].values[0]].value,
            body.getArgument(1));
  EXPECT_EQ(initialPlan->observations[0].spec, beforeSpec);

  auto afterSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("name", builder.getStringAttr("after"))});
  observation->setOperand(1, read.getResult());
  observation->setAttr("spec", afterSpec);
  am.invalidate(mlir::detail::PreservedAnalyses());
  EXPECT_FALSE(
      am.getCachedAnalysis<SourceTransactionStateAnalysis>().has_value());
  auto &rebuilt = capture([&]() -> SourceTransactionStateAnalysis & {
    return am.getAnalysis<SourceTransactionStateAnalysis>();
  });
  ASSERT_TRUE(rebuilt.isValid()) << diagnostics;
  auto *rebuiltPlan = rebuilt.lookup(run);
  ASSERT_NE(rebuiltPlan, nullptr);
  ASSERT_EQ(rebuiltPlan->observations.size(), 1u);
  EXPECT_EQ(rebuiltPlan->values[rebuiltPlan->observations[0].values[0]].value,
            read.getResult());
  EXPECT_EQ(rebuiltPlan->observations[0].spec, afterSpec);
}

TEST_F(TransactionStateSSATest,
       ImportedSummaryCannotSubstituteForExecutableMethodBody) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto published =
      capture([&] { return summaries.getPublicationSummary(child); });
  ASSERT_TRUE(mlir::succeeded(published)) << diagnostics;
  mlir::OperationState state(child.getLoc(),
                             ModuleImportOp::getOperationName());
  state.addAttributes(child->getAttrs());
  state.addAttribute("semantic_kind", builder.getStringAttr("transactions"));
  state.addAttribute("transaction_summary", *published);
  builder.setInsertionPoint(child);
  builder.create(state);
  child->erase();
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  reject(*file);
}

TEST_F(TransactionStateSSATest,
       BodyEffectsBindStatementTimeSSAAndExpandedInvocationFrames) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto run = transaction(*file);
  auto bump = method(*file);
  auto &rootBody = run.getBody().front();
  auto rootPath = rootBody.getArgument(0);
  auto rootReads = llvm::to_vector(rootBody.getOps<SourceStateReadOp>());
  auto rootCalls = llvm::to_vector(rootBody.getOps<SourceMethodCallOp>());
  ASSERT_EQ(rootReads.size(), 2u);
  ASSERT_GE(rootCalls.size(), 3u);

  auto valueItem = [&](unsigned ordinal) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("value")),
         builder.getNamedAttr("ordinal", builder.getI32IntegerAttr(ordinal))});
  };
  auto literalItem = [&](llvm::StringRef text) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
         builder.getNamedAttr("text", builder.getStringAttr(text))});
  };
  auto addObserve = [&](mlir::Block &block, mlir::Block::iterator where,
                        mlir::Value path, mlir::ValueRange values,
                        mlir::DictionaryAttr spec, llvm::StringRef kind,
                        llvm::StringRef owner, unsigned index) {
    builder.setInsertionPoint(&block, where);
    mlir::OperationState state(run.getLoc(),
                               SourceObserveOp::getOperationName());
    state.addOperands(path);
    state.addOperands(values);
    state.addAttribute("kind", builder.getStringAttr(kind));
    state.addAttribute("spec", spec);
    state.addAttribute("occurrence", occurrence(owner, index));
    state.addAttribute("ac.location", span("state.py", index));
    return mlir::cast<SourceObserveOp>(builder.create(state));
  };
  auto addCheck = [&](mlir::Block &block, mlir::Block::iterator where,
                      mlir::Value condition, mlir::Value path,
                      llvm::StringRef owner, unsigned index,
                      std::optional<llvm::StringRef> message) {
    builder.setInsertionPoint(&block, where);
    mlir::OperationState state(run.getLoc(),
                               SourceExpectOp::getOperationName());
    state.addOperands({condition, path});
    state.addAttribute("kind", builder.getStringAttr("assert"));
    state.addAttribute("location", span("state.py", index));
    state.addAttribute("occurrence", occurrence(owner, index));
    if (message)
      state.addAttribute("ac.message", builder.getStringAttr(*message));
    return mlir::cast<SourceExpectOp>(builder.create(state));
  };

  auto mixedSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("level", builder.getStringAttr("info")),
       builder.getNamedAttr("event", builder.getStringAttr("saved")),
       builder.getNamedAttr(
           "items", builder.getArrayAttr({valueItem(0), literalItem("then"),
                                          valueItem(1), valueItem(2)}))});
  auto pre = addObserve(rootBody, rootCalls[2]->getIterator(), rootPath,
                        mlir::ValueRange{rootReads[0].getResult(),
                                         rootCalls[0].getResult(0),
                                         rootReads[0].getResult()},
                        mixedSpec, "log", "Root", 20);
  auto directCheck = addCheck(rootBody, rootCalls[2]->getIterator(), rootPath,
                              rootPath, "Root", 21, llvm::StringRef(""));
  auto root = module(*file, "Root");
  auto rootConstants = root.getBody().front().getOps<BitsConstantOp>();
  auto rootFalse = *std::next(rootConstants.begin());
  auto *falseCondition = rootFalse->clone();
  rootBody.getOperations().insert(rootCalls[2]->getIterator(), falseCondition);
  auto falseCheck = addCheck(rootBody, rootCalls[2]->getIterator(),
                             falseCondition->getResult(0), rootPath, "Root", 24,
                             std::nullopt);
  auto postSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("name", builder.getStringAttr("post"))});
  auto post = addObserve(rootBody, std::next(rootReads[1]->getIterator()),
                         rootPath, mlir::ValueRange(rootReads[1].getResult()),
                         postSpec, "report", "Root", 22);
  auto literalSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("level", builder.getStringAttr("debug")),
       builder.getNamedAttr("event", builder.getStringAttr("literal")),
       builder.getNamedAttr("items",
                            builder.getArrayAttr({literalItem("only")}))});
  auto literal =
      addObserve(rootBody, rootBody.getTerminator()->getIterator(), rootPath,
                 mlir::ValueRange{}, literalSpec, "log", "Root", 23);

  auto &methodBody = bump.getBody().front();
  auto methodRead = first<SourceStateReadOp>(bump.getBody());
  auto methodPath = methodBody.getArgument(1);
  auto methodSpec = builder.getDictionaryAttr(
      {builder.getNamedAttr("name", builder.getStringAttr("method-old"))});
  auto expandedObserve =
      addObserve(methodBody, methodBody.getTerminator()->getIterator(),
                 methodPath, mlir::ValueRange(methodRead.getResult()),
                 methodSpec, "report", "Child", 20);
  auto expandedCheck = addCheck(
      methodBody, methodBody.getTerminator()->getIterator(),
      methodBody.getArgument(5), methodPath, "Child", 21, std::nullopt);

  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto before = print(file->getOperation());
  auto operations = identities(*file);
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(run);
  ASSERT_NE(plan, nullptr);
  EXPECT_EQ(plan->bodyChecks.size(), 5u);
  EXPECT_EQ(plan->observations.size(), 6u);
  EXPECT_EQ(plan->versions.size(), 7u);
  EXPECT_EQ(before, print(file->getOperation()));
  EXPECT_EQ(operations, identities(*file));

  auto recordFor = [&](SourceObserveOp operation,
                       std::optional<InvocationFrameId> frame = std::nullopt) {
    const TransactionObservation *found = nullptr;
    for (const auto &record : plan->observations)
      if (record.operation == operation && (!frame || record.frame == *frame)) {
        EXPECT_EQ(found, nullptr);
        found = &record;
      }
    return found;
  };
  auto preRecord = recordFor(pre);
  auto postRecord = recordFor(post);
  auto literalRecord = recordFor(literal);
  ASSERT_NE(preRecord, nullptr);
  ASSERT_NE(postRecord, nullptr);
  ASSERT_NE(literalRecord, nullptr);
  ASSERT_EQ(preRecord->values.size(), 3u);
  EXPECT_EQ(plan->values[preRecord->values[0]].value, rootReads[0].getResult());
  EXPECT_EQ(plan->values[preRecord->values[1]].value,
            rootCalls[0].getResult(0));
  EXPECT_EQ(preRecord->values[0], preRecord->values[2]);
  EXPECT_EQ(plan->values[postRecord->values[0]].value,
            rootReads[1].getResult());
  EXPECT_TRUE(literalRecord->values.empty());
  const TransactionBodyCheck *directCheckRecord = nullptr;
  for (const auto &record : plan->bodyChecks)
    if (record.operation == directCheck)
      directCheckRecord = &record;
  ASSERT_NE(directCheckRecord, nullptr);
  ASSERT_TRUE(directCheckRecord->message);
  EXPECT_TRUE(directCheckRecord->message.getValue().empty());
  EXPECT_EQ(directCheckRecord->condition, directCheckRecord->authoredPath);
  const TransactionBodyCheck *falseCheckRecord = nullptr;
  for (const auto &record : plan->bodyChecks)
    if (record.operation == falseCheck)
      falseCheckRecord = &record;
  ASSERT_NE(falseCheckRecord, nullptr);
  EXPECT_NE(falseCheckRecord->condition, falseCheckRecord->authoredPath);
  auto conditionValue = plan->values[falseCheckRecord->condition].value;
  auto conditionLiteral = conditionValue.getDefiningOp<BitsConstantOp>();
  ASSERT_TRUE(conditionLiteral);
  EXPECT_EQ(conditionLiteral.getValue(), rootFalse.getValue());

  const StateReadBinding *preRead = nullptr, *postRead = nullptr;
  for (const auto &read : plan->reads) {
    if (read.operation == rootReads[0])
      preRead = &read;
    if (read.operation == rootReads[1])
      postRead = &read;
  }
  ASSERT_NE(preRead, nullptr);
  ASSERT_NE(postRead, nullptr);
  EXPECT_EQ(ordinal(*plan, preRead->version), 0u);
  EXPECT_EQ(ordinal(*plan, postRead->version), 1u);
  auto *preEvent = event(*plan, TransactionStateEventKind::Observe,
                         preRecord - plan->observations.data());
  ASSERT_NE(preEvent, nullptr);
  EXPECT_EQ(preRecord->point.prefix, preEvent->point.prefix);
  EXPECT_LT(plan->values[preRecord->values[0]].validity.prefix,
            preEvent->point.prefix)
      << "saved operands retain their original admission prefix";

  llvm::SmallVector<const TransactionObservation *> expanded;
  for (const auto &record : plan->observations)
    if (record.operation == expandedObserve)
      expanded.push_back(&record);
  ASSERT_EQ(expanded.size(), 3u);
  EXPECT_NE(expanded[0]->frame, expanded[1]->frame);
  EXPECT_NE(expanded[1]->frame, expanded[2]->frame);
  EXPECT_EQ(plan->frames[expanded[0]->frame].anchor,
            plan->frames[expanded[1]->frame].anchor);
  EXPECT_NE(plan->frames[expanded[1]->frame].anchor,
            plan->frames[expanded[2]->frame].anchor);
  for (const auto *record : expanded) {
    EXPECT_EQ(record->kind.getValue(), "report");
    EXPECT_EQ(record->operation, expandedObserve);
    EXPECT_EQ(record->occurrence, expandedObserve->getAttr("occurrence"));
  }
  unsigned expandedChecks = 0;
  for (const auto &record : plan->bodyChecks)
    if (record.operation == expandedCheck) {
      ++expandedChecks;
      EXPECT_FALSE(record.message);
    }
  EXPECT_EQ(expandedChecks, 3u);
  BodyCheckId nextCheck = 0;
  ObservationId nextObservation = 0;
  for (const auto &entry : plan->events) {
    if (entry.kind == TransactionStateEventKind::BodyCheck) {
      ASSERT_TRUE(entry.bodyCheck);
      EXPECT_EQ(*entry.bodyCheck, nextCheck++);
      EXPECT_EQ(entry.frame, plan->bodyChecks[*entry.bodyCheck].frame);
      EXPECT_FALSE(entry.observation);
      EXPECT_FALSE(entry.read);
      EXPECT_FALSE(entry.write);
      EXPECT_FALSE(entry.guard);
      EXPECT_FALSE(entry.call);
    } else if (entry.kind == TransactionStateEventKind::Observe) {
      ASSERT_TRUE(entry.observation);
      EXPECT_EQ(*entry.observation, nextObservation++);
      EXPECT_EQ(entry.frame, plan->observations[*entry.observation].frame);
      EXPECT_FALSE(entry.bodyCheck);
      EXPECT_FALSE(entry.read);
      EXPECT_FALSE(entry.write);
      EXPECT_FALSE(entry.guard);
      EXPECT_FALSE(entry.call);
    } else {
      EXPECT_FALSE(entry.bodyCheck);
      EXPECT_FALSE(entry.observation);
    }
  }
  EXPECT_EQ(nextCheck, plan->bodyChecks.size());
  EXPECT_EQ(nextObservation, plan->observations.size());

  auto eventIndex = [&](TransactionStateEventKind kind,
                        uint64_t record) -> size_t {
    for (auto [index, entry] : llvm::enumerate(plan->events)) {
      if (entry.kind == kind &&
          ((kind == TransactionStateEventKind::BodyCheck && entry.bodyCheck &&
            *entry.bodyCheck == record) ||
           (kind == TransactionStateEventKind::Observe && entry.observation &&
            *entry.observation == record)))
        return index;
    }
    ADD_FAILURE() << "missing projected body-effect event";
    return size_t(0);
  };
  for (const auto *observation : expanded) {
    auto observationId = observation - plan->observations.data();
    auto observationEvent =
        eventIndex(TransactionStateEventKind::Observe, observationId);
    const TransactionBodyCheck *matchingCheck = nullptr;
    for (const auto &check : plan->bodyChecks)
      if (check.operation == expandedCheck && check.frame == observation->frame)
        matchingCheck = &check;
    ASSERT_NE(matchingCheck, nullptr);
    auto checkEvent = eventIndex(TransactionStateEventKind::BodyCheck,
                                 matchingCheck - plan->bodyChecks.data());
    EXPECT_LT(observationEvent, checkEvent);
    bool sawRead = false, sawCallExit = false;
    for (auto [index, entry] : llvm::enumerate(plan->events)) {
      if (entry.frame == observation->frame &&
          entry.kind == TransactionStateEventKind::Read &&
          index < observationEvent)
        sawRead = true;
      if (entry.kind == TransactionStateEventKind::CallExit && entry.call &&
          plan->calls[*entry.call].callee == observation->frame) {
        EXPECT_GT(index, checkEvent);
        sawCallExit = true;
      }
    }
    EXPECT_TRUE(sawRead);
    EXPECT_TRUE(sawCallExit);
  }
  auto text = dump(*analysis);
  EXPECT_NE(text.find("body-check"), std::string::npos);
  EXPECT_NE(text.find("observe"), std::string::npos);
  EXPECT_EQ(text.find("0x"), std::string::npos);
}

TEST_F(TransactionStateSSATest,
       LaterFalseGuardInVoidTransactionDoesNotPruneEarlierObservation) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto watch = transaction(*file, "watch");
  auto &body = watch.getBody().front();
  auto read = first<SourceStateReadOp>(watch.getBody());
  auto call = first<SourceMethodCallOp>(watch.getBody());
  builder.setInsertionPoint(call);
  mlir::OperationState observation(watch.getLoc(),
                                   SourceObserveOp::getOperationName());
  observation.addOperands({body.getArgument(0), read.getResult()});
  observation.addAttribute("kind", builder.getStringAttr("report"));
  observation.addAttribute(
      "spec", builder.getDictionaryAttr({builder.getNamedAttr(
                  "name", builder.getStringAttr("before-false-guard"))}));
  observation.addAttribute("occurrence", occurrence("Root", 30));
  observation.addAttribute("ac.location", span("state.py", 30));
  auto sourceObservation =
      mlir::cast<SourceObserveOp>(builder.create(observation));
  auto child = module(*file, "Child");
  auto constants = child.getBody().front().getOps<BitsConstantOp>();
  auto no = *std::next(constants.begin());
  auto peek = method(*file, "peek");
  auto *falsePeek = peek->clone();
  falsePeek->setAttr("sym_name", builder.getStringAttr("peek_false"));
  falsePeek->setAttr("occurrence", occurrence("Child", 30));
  for (auto &region : falsePeek->getRegions())
    for (auto read :
         llvm::make_early_inc_range(region.front().getOps<SourceStateReadOp>()))
      read->erase();
  auto &falseGuard = falsePeek->getRegion(0).front();
  auto *falsePredicate = no->clone();
  falseGuard.getOperations().insert(falseGuard.begin(), falsePredicate);
  falseGuard.getTerminator()->setOperand(0, falsePredicate->getResult(0));
  child.getBody().front().getOperations().insert(peek->getIterator(),
                                                 falsePeek);
  call->setAttr("callee",
                mlir::SymbolRefAttr::get(
                    &context, "Child",
                    {mlir::FlatSymbolRefAttr::get(&context, "peek_false")}));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;

  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(watch);
  ASSERT_NE(plan, nullptr);
  ASSERT_EQ(plan->observations.size(), 1u);
  EXPECT_EQ(plan->observations[0].operation, sourceObservation);
  ASSERT_EQ(plan->observations[0].values.size(), 1u);
  EXPECT_EQ(plan->values[plan->observations[0].values[0]].value,
            read.getResult());
  EXPECT_TRUE(plan->bodyChecks.empty());
  EXPECT_TRUE(plan->writes.empty());
  EXPECT_EQ(plan->versions.size(), plan->owners.size());
  ASSERT_EQ(plan->guards.size(), 2u);
  ASSERT_TRUE(plan->guards.back().predicateFact);
  EXPECT_FALSE(plan->guards.back().predicateFact.getValue());
  size_t observationEvent = plan->events.size(), falseGuardEnter = 0;
  for (auto [index, entry] : llvm::enumerate(plan->events)) {
    if (entry.kind == TransactionStateEventKind::Observe)
      observationEvent = index;
    if (entry.kind == TransactionStateEventKind::GuardEnter && entry.guard &&
        *entry.guard == 1)
      falseGuardEnter = index;
  }
  ASSERT_LT(observationEvent, plan->events.size());
  EXPECT_LT(observationEvent, falseGuardEnter);
}

TEST_F(TransactionStateSSATest,
       FalseAuthoredBodyEffectPathsRejectEveryPlanAtomically) {
  for (llvm::StringRef pathKind : {"false", "unknown"}) {
    for (llvm::StringRef kind : {"expect", "observe"}) {
      SCOPED_TRACE(pathKind.str());
      SCOPED_TRACE(kind.str());
      auto file = parse();
      ASSERT_TRUE(file);
      bool useLaterTransaction = pathKind == "false" && kind == "observe";
      auto target = transaction(*file, useLaterTransaction ? "watch" : "run");
      auto &body = target.getBody().front();
      auto root = module(*file, "Root");
      auto constants = root.getBody().front().getOps<BitsConstantOp>();
      auto falseValue = *std::next(constants.begin());
      auto *no = falseValue->clone();
      body.getOperations().insert(body.getTerminator()->getIterator(), no);
      mlir::Value authoredPath = pathKind == "false"
                                     ? mlir::Value(no->getResult(0))
                                     : mlir::Value(body.getArgument(3));
      mlir::OperationState effect(target.getLoc(),
                                  kind == "expect"
                                      ? SourceExpectOp::getOperationName()
                                      : SourceObserveOp::getOperationName());
      if (kind == "expect") {
        effect.addOperands({body.getArgument(0), authoredPath});
        effect.addAttribute("kind", builder.getStringAttr("assert"));
        effect.addAttribute("location", span("state.py", 30));
      } else {
        effect.addOperands({authoredPath, body.getArgument(1)});
        effect.addAttribute("kind", builder.getStringAttr("report"));
        effect.addAttribute("spec",
                            builder.getDictionaryAttr({builder.getNamedAttr(
                                "name", builder.getStringAttr("candidate"))}));
        effect.addAttribute("ac.location", span("state.py", 30));
      }
      effect.addAttribute("occurrence", occurrence("Root", 30));
      builder.setInsertionPoint(body.getTerminator());
      builder.create(effect);
      ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
      reject(*file);
      EXPECT_NE(diagnostics.find("path"), std::string::npos) << diagnostics;
    }
  }
}

TEST_F(TransactionStateSSATest,
       StaticGrandchildCallsComposeOccurrencePathsAndInputDomains) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto exemplar = *root.getBody().front().getOps<InstanceOp>().begin();
  mlir::OperationState wrapperState(root.getLoc(),
                                    ModuleOp::getOperationName());
  wrapperState.addAttributes(root->getAttrs());
  wrapperState.attributes.set("sym_name", builder.getStringAttr("Wrapper"));
  wrapperState.addRegion();
  builder.setInsertionPoint(root);
  auto wrapper = mlir::cast<ModuleOp>(builder.create(wrapperState));
  auto *block = new mlir::Block;
  wrapper.getBody().push_back(block);
  for (auto type : root.getFunctionType().getInputs())
    block->addArgument(type, wrapper.getLoc());
  auto *yes =
      (*root.getBody().front().getOps<BitsConstantOp>().begin())->clone();
  block->push_back(yes);
  auto *inner = exemplar->clone();
  inner->setOperands(block->getArguments());
  inner->setAttr("instance_name", builder.getStringAttr("inner"));
  inner->setAttr("occurrence", occurrence("Wrapper", 0));
  block->push_back(inner);
  auto *forward = method(*file, "relay")->clone();
  forward->setOperands(yes->getResults());
  forward->setAttr("sym_name", builder.getStringAttr("forward"));
  forward->setAttr("occurrence", occurrence("Wrapper", 1));
  // Rebind source declaration metadata to the wrapper's lexical declaration.
  auto parameters = forward->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  mlir::NamedAttrList parameter(
      mlir::cast<mlir::DictionaryAttr>(parameters[0]));
  parameter.set("origin", occurrence("Wrapper", 1));
  forward->setAttr("ac.parameters",
                   builder.getArrayAttr({parameter.getDictionary(&context)}));
  block->push_back(forward);
  auto nested =
      first<SourceMethodCallOp>(mlir::cast<SourceMethodOp>(forward).getBody());
  nested->setAttr("instance_path",
                  builder.getArrayAttr({builder.getStringAttr("inner")}));
  nested->setAttr("occurrence", occurrence("Wrapper", 2));
  builder.setInsertionPointToEnd(block);
  mlir::OperationState yield(wrapper.getLoc(), YieldOp::getOperationName());
  yield.addOperands(inner->getResults());
  builder.create(yield);
  auto *outer = exemplar->clone();
  outer->setAttr("callee", mlir::FlatSymbolRefAttr::get(&context, "Wrapper"));
  outer->setAttr("instance_name", builder.getStringAttr("wrapped"));
  outer->setAttr("occurrence", occurrence("Root", 22));
  root.getBody().front().getOperations().insert(
      transaction(*file)->getIterator(), outer);
  auto call = first<SourceMethodCallOp>(transaction(*file).getBody());
  call->setAttr("callee",
                mlir::SymbolRefAttr::get(
                    &context, "Wrapper",
                    {mlir::FlatSymbolRefAttr::get(&context, "forward")}));
  call->setAttr("instance_path",
                builder.getArrayAttr({builder.getStringAttr("wrapped")}));
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *plan = analysis->lookup(transaction(*file));
  ASSERT_NE(plan, nullptr);
  auto owner = llvm::find_if(plan->owners, [](const auto &owner) {
    return owner.instancePath.size() == 2;
  });
  ASSERT_NE(owner, plan->owners.end());
  EXPECT_EQ(owner->instancePath[0], outer->getAttr("occurrence"));
  EXPECT_EQ(owner->instancePath[1], inner->getAttr("occurrence"));
  EXPECT_EQ(owner->domain, plan->domain);
  auto anchor = owner->anchor;
  ASSERT_LT(anchor, plan->instances.size());
  ASSERT_TRUE(plan->instances[anchor].parent);
  EXPECT_EQ(plan->instances[*plan->instances[anchor].parent].allocation, outer);
  EXPECT_EQ(plan->instances[anchor].allocation, inner);
  EXPECT_EQ(plan->instances[anchor].bindings.owner,
            module(*file, "Child").getOperation());
}

TEST_F(TransactionStateSSATest,
       CapturesAndOrdinaryInstanceResultsKeepLexicalAndOrdinalIdentity) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto root = module(*file, "Root");
  auto instance = *root.getBody().front().getOps<InstanceOp>().begin();
  auto watch = transaction(*file, "watch");
  watch->setOperand(3, instance->getResult(0));
  auto analysis = analyze(*file);
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  auto *watchPlan = analysis->lookup(watch);
  ASSERT_NE(watchPlan, nullptr);
  auto &rootFrame = watchPlan->frames[watchPlan->rootFrame];
  ASSERT_EQ(rootFrame.bodyArguments.size(), 2u);
  auto captured = rootFrame.bodyArguments[1].actual;
  EXPECT_EQ(watchPlan->values[captured].value, instance->getResult(0));
  EXPECT_EQ(watchPlan->values[captured].anchor, rootFrame.anchor);
  for (auto &owner : watchPlan->owners)
    EXPECT_NE(watchPlan->values[owner.oldQ].value, instance->getResult(0));
  auto *runPlan = analysis->lookup(transaction(*file));
  ASSERT_NE(runPlan, nullptr);
  for (auto &call : runPlan->calls) {
    auto &frame = runPlan->frames[call.callee];
    auto methodOp = frame.method;
    ASSERT_EQ(frame.guardArguments.size(),
              call.arguments.size() + call.captures.size());
    ASSERT_EQ(frame.bodyArguments.size(), frame.guardArguments.size());
    for (unsigned i = 0; i < frame.guardArguments.size(); ++i) {
      EXPECT_EQ(frame.guardArguments[i].formal,
                methodOp.getGuard().front().getArgument(i));
      EXPECT_EQ(frame.bodyArguments[i].formal,
                methodOp.getBody().front().getArgument(i));
      auto actual = i < call.arguments.size()
                        ? call.arguments[i]
                        : call.captures[i - call.arguments.size()];
      EXPECT_EQ(frame.guardArguments[i].actual, actual);
      EXPECT_EQ(frame.bodyArguments[i].actual, actual);
    }
  }
}

TEST_F(TransactionStateSSATest,
       DeepAttributeAndTypeGraphsRejectBeforeNativeVerification) {
  // The owning operation graph stays small. Only immutable subelements become
  // deep, so an operation-only preflight cannot protect the verifier here.
  // Never invoke standalone native verification on the mutated fixtures.
  for (unsigned graph = 0; graph < 4; ++graph) {
    SCOPED_TRACE(graph);
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto literal = *child.getBody().front().getOps<BitsConstantOp>().begin();
    auto leaf = literal.getValue();
    constexpr unsigned depth = 140; // Beyond the analysis's depth-128 policy.
    if (graph == 0) {
      mlir::Type nested = literal.getResult().getType();
      for (unsigned i = 0; i < depth; ++i)
        // Use a public native constructor: Table's public builder normalizes
        // element nesting, while Tuple preserves the type subelement chain.
        nested = mlir::TupleType::get(&context, {nested});
      child->setAttr("test_deep_type", mlir::TypeAttr::get(nested));
    } else if (graph == 3) {
      mlir::Attribute nested = builder.getUnitAttr();
      for (unsigned i = 0; i < depth; ++i)
        nested = builder.getArrayAttr({nested});
      child->setAttr("test_deep_attribute", nested);
    } else {
      auto nested = leaf;
      for (unsigned i = 0; i < depth; ++i)
        nested = StaticExprAttr::get(
            &context,
            builder.getDictionaryAttr(
                {builder.getNamedAttr("kind", builder.getStringAttr("binary")),
                 builder.getNamedAttr("operator", builder.getStringAttr("add")),
                 builder.getNamedAttr("lhs", nested),
                 builder.getNamedAttr("rhs", leaf),
                 builder.getNamedAttr("origin", leaf.getTree().get("origin")),
                 builder.getNamedAttr("location",
                                      leaf.getTree().get("location"))}));
      if (graph == 1)
        // Width nesting is a type -> StaticExpr -> dictionary edge sequence.
        literal->getResult(0).setType(BitsType::get(&context, nested));
      else
        literal->setAttr("value", nested);
    }
    EXPECT_LT(identities(*file).size(), 100u);
    reject(*file);
    EXPECT_TRUE(diagnostics.find("depth") != std::string::npos ||
                diagnostics.find("budget") != std::string::npos)
        << diagnostics;
  }
}

TEST_F(TransactionStateSSATest,
       ShallowSharedTypeDiamondChargesExpandedWorkBeforeVerification) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  auto literal = *child.getBody().front().getOps<BitsConstantOp>().begin();
  mlir::Type diamond = literal.getResult().getType();
  // Only 24 new unique nodes, but over 2^24 expanded child occurrences.
  // The depth stays well below 128: deduplicated visitation alone is unsafe.
  for (unsigned i = 0; i < 24; ++i)
    diamond = mlir::TupleType::get(&context, {diamond, diamond});
  auto metadata = mlir::TypeAttr::get(diamond);
  child->setAttr("test_shared_type", metadata);
  auto operations = identities(*file);
  std::vector<std::vector<mlir::Value>> operands;
  std::vector<mlir::DictionaryAttr> attributes;
  for (auto *operation : operations) {
    operands.emplace_back(operation->operand_begin(), operation->operand_end());
    attributes.push_back(operation->getAttrDictionary());
  }
  // Keep normal diagnostic behavior enabled: preflight must reject without
  // serializing the expanded hostile operation in a diagnostic note.
  context.printOpOnDiagnostic(true);
  auto analysis = analyzeRawPreflightFailure(*file);
  EXPECT_FALSE(analysis->isValid());
  EXPECT_LT(diagnostics.size(), 4096u);
  EXPECT_TRUE(diagnostics.find("budget") != std::string::npos ||
              diagnostics.find("work") != std::string::npos ||
              diagnostics.find("expansion") != std::string::npos)
      << diagnostics;
  EXPECT_EQ(analysis->lookup(transaction(*file)), nullptr);
  EXPECT_EQ(analysis->lookup(transaction(*file, "watch")), nullptr);
  std::string partial;
  llvm::raw_string_ostream stream(partial);
  EXPECT_TRUE(mlir::failed(analysis->print(stream)));
  EXPECT_TRUE(partial.empty());
  EXPECT_EQ(operations, identities(*file));
  EXPECT_EQ(child->getAttr("test_shared_type"), metadata);
  for (unsigned i = 0; i < operations.size(); ++i) {
    EXPECT_EQ(operations[i]->getAttrDictionary(), attributes[i]);
    EXPECT_EQ(std::vector<mlir::Value>(operations[i]->operand_begin(),
                                       operations[i]->operand_end()),
              operands[i]);
  }
}

TEST_F(TransactionStateSSATest,
       DeepRawLocationRejectsWithBoundedDiagnosticBeforeNativeVerification) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  mlir::Location location = builder.getUnknownLoc();
  auto label = builder.getStringAttr(std::string(128, 'L'));
  for (unsigned i = 0; i < 140; ++i)
    location = mlir::NameLoc::get(label, location);
  child->setLoc(location);
  auto operations = identities(*file);
  std::vector<mlir::Location> locations;
  std::vector<mlir::DictionaryAttr> attributes;
  for (auto *operation : operations) {
    locations.push_back(operation->getLoc());
    attributes.push_back(operation->getAttrDictionary());
  }
  context.printOpOnDiagnostic(true);
  // Never print or separately verify the raw location graph. The diagnostic
  // must use a bounded safe location before native validation traverses it.
  auto analysis = analyzeRawPreflightFailure(*file);
  EXPECT_FALSE(analysis->isValid());
  EXPECT_LT(diagnostics.size(), 4096u);
  EXPECT_TRUE(diagnostics.find("depth") != std::string::npos ||
              diagnostics.find("budget") != std::string::npos)
      << diagnostics;
  EXPECT_EQ(analysis->lookup(transaction(*file)), nullptr);
  EXPECT_EQ(analysis->lookup(transaction(*file, "watch")), nullptr);
  std::string partial;
  llvm::raw_string_ostream stream(partial);
  EXPECT_TRUE(mlir::failed(analysis->print(stream)));
  EXPECT_TRUE(partial.empty());
  EXPECT_EQ(operations, identities(*file));
  EXPECT_EQ(child->getLoc(), location);
  for (unsigned i = 0; i < operations.size(); ++i) {
    EXPECT_EQ(operations[i]->getLoc(), locations[i]);
    EXPECT_EQ(operations[i]->getAttrDictionary(), attributes[i]);
  }
}

TEST_F(TransactionStateSSATest,
       ResolverProviderPaymentFailureKeepsPrimaryBudgetDiagnosticAndAtomicity) {
  auto file = parse();
  ASSERT_TRUE(file);
  auto child = module(*file, "Child");
  // A single shallow, semantically inert provider attribute fits the raw-graph
  // bound but makes repeated provider-header work expensive. The real calls
  // resolve through this provider; no missing or malformed target substitutes
  // for the budget failure. This checks externally observable failure, not an
  // internal resolver scan count or timing oracle.
  auto payload = builder.getStringAttr(std::string(256 * 1024, 'P'));
  child->setAttr("test_resolver_payload", payload);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
      << diagnostics;
  auto summaries = capture([&] {
    return std::make_unique<SourceMethodSummariesAnalysis>(
        file->getOperation());
  });
  ASSERT_TRUE(summaries->isValid()) << diagnostics;
  auto operations = identities(*file);
  std::vector<mlir::DictionaryAttr> attributes;
  std::vector<std::vector<mlir::Value>> operands;
  for (auto *operation : operations) {
    attributes.push_back(operation->getAttrDictionary());
    operands.emplace_back(operation->operand_begin(), operation->operand_end());
  }
  context.printOpOnDiagnostic(true);
  diagnostics.clear();
  unsigned diagnosticCount = 0;
  std::unique_ptr<SourceTransactionStateAnalysis> analysis;
  {
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &error) {
          if (diagnosticCount++ == 0) {
            EXPECT_TRUE(mlir::isa<mlir::UnknownLoc>(error.getLocation()));
            EXPECT_TRUE(error.getNotes().empty());
            EXPECT_NE(error.str().find(
                          "transaction state analysis work budget exceeded"),
                      std::string::npos);
          }
          // A nullable resolver may emit its existing call-local diagnostic
          // after the primary budget error. Preserve that public behavior.
          llvm::raw_string_ostream stream(diagnostics);
          error.print(stream);
          for (auto &note : error.getNotes())
            note.print(stream);
          return mlir::success();
        });
    analysis =
        std::make_unique<SourceTransactionStateAnalysis>(file->getOperation());
  }
  EXPECT_GT(diagnosticCount, 0u);
  EXPECT_FALSE(analysis->isValid());
  EXPECT_LT(diagnostics.size(), 4096u);
  EXPECT_EQ(analysis->lookup(transaction(*file)), nullptr);
  EXPECT_EQ(analysis->lookup(transaction(*file, "watch")), nullptr);
  std::string partial;
  llvm::raw_string_ostream stream(partial);
  EXPECT_TRUE(mlir::failed(analysis->print(stream)));
  EXPECT_TRUE(partial.empty());
  EXPECT_EQ(child->getAttr("test_resolver_payload"), payload);
  EXPECT_EQ(operations, identities(*file));
  for (unsigned i = 0; i < operations.size(); ++i) {
    EXPECT_EQ(operations[i]->getAttrDictionary(), attributes[i]);
    EXPECT_EQ(std::vector<mlir::Value>(operations[i]->operand_begin(),
                                       operations[i]->operand_end()),
              operands[i]);
  }
}

TEST_F(TransactionStateSSATest,
       DeepAndExponentiallyRepeatedInvocationsAreBounded) {
  for (bool exponential : {false, true}) {
    auto file = parse();
    ASSERT_TRUE(file);
    auto child = module(*file, "Child");
    auto relay = method(*file, "relay");
    auto exemplar = first<SourceMethodCallOp>(relay.getBody());
    auto bump = method(*file, "bump");
    auto &bumpBody = bump.getBody().front();
    builder.setInsertionPoint(bumpBody.getTerminator());
    mlir::OperationState observation(bump.getLoc(),
                                     SourceObserveOp::getOperationName());
    observation.addOperands({bumpBody.getArgument(1), bumpBody.getArgument(0)});
    observation.addAttribute("kind", builder.getStringAttr("report"));
    observation.addAttribute("spec",
                             builder.getDictionaryAttr({builder.getNamedAttr(
                                 "name", builder.getStringAttr("expanded"))}));
    observation.addAttribute("occurrence", occurrence("Child", 90));
    observation.addAttribute("ac.location", span("state.py", 90));
    builder.create(observation);
    unsigned levels = exponential ? 22 : 140;
    for (unsigned i = 0; i < levels; ++i) {
      auto *copy = relay->clone();
      copy->setAttr("sym_name",
                    builder.getStringAttr("chain" + std::to_string(i)));
      copy->setAttr("occurrence", occurrence("Child", 100 + i));
      child.getBody().front().getOperations().insert(relay->getIterator(),
                                                     copy);
      auto level = mlir::cast<SourceMethodOp>(copy);
      auto call = first<SourceMethodCallOp>(level.getBody());
      call->setAttr("callee",
                    mlir::SymbolRefAttr::get(
                        &context, "Child",
                        {mlir::FlatSymbolRefAttr::get(
                            &context, i + 1 == levels
                                          ? "noop"
                                          : "chain" + std::to_string(i + 1))}));
      // The terminal call retains relay's one-argument signature by using bump.
      if (i + 1 == levels)
        call->setAttr("callee", exemplar.getCalleeAttr());
      call->setAttr("occurrence", occurrence("Child", 300 + 2 * i));
      if (exponential && i + 1 != levels) {
        auto *second = call->clone();
        second->setAttr("occurrence", occurrence("Child", 301 + 2 * i));
        level.getBody().front().getOperations().insert(call->getIterator(),
                                                       second);
      }
    }
    first<SourceMethodCallOp>(transaction(*file).getBody())
        ->setAttr("callee",
                  mlir::SymbolRefAttr::get(
                      &context, "Child",
                      {mlir::FlatSymbolRefAttr::get(&context, "chain0")}));
    reject(*file);
    EXPECT_TRUE(diagnostics.find("depth") != std::string::npos ||
                diagnostics.find("budget") != std::string::npos)
        << diagnostics;
  }
}

} // namespace
} // namespace acir::ac
