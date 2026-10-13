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

#include <memory>
#include <string>
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
    }) {sym_name = "bump", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #c2, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #c2} : (!b1, !b8, !b1, !b1, !b1) -> ()
    "ac.method"(%yes) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %nested = "ac.method.call"(%path, %delta) {callee = @Child::@bump, instance_path = [], occurrence = #c7} : (!b1, !b8) -> !b8
      "ac.yield"(%nested) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #c6, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #c6} : (!b1) -> ()
    "ac.method"(%yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      %guard_read = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c9} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %unused = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #c10} : (!b1) -> !b8
      "ac.yield"(%snapshot) : (!b8) -> ()
    }) {sym_name = "peek", function_type = () -> !b8, argument_names = [], ac.parameters = [], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #c8} : (!b1, !b8) -> ()
    "ac.method"(%yes) ({
    ^bb0(%path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1):
      "ac.yield"() : () -> ()
    }) {sym_name = "noop", function_type = () -> (), argument_names = [], ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", occurrence = #c11} : (!b1) -> ()
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
    }) {name = "run", declaration = #r3, occurrence = #r4} : (!b1, !b1, !b1, !b8, !b8, !b1, !b1) -> !b8
    "ac.transaction"(%clk, %rst, %yes, %q) ({
    ^bb0(%path: !b1, %snapshot: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %snapshot: !b8):
      %old = "ac.state.read"(%path) {state = "status", selectors = [], occurrence = #r16} : (!b1) -> !b8
      %unused = "ac.method.call"(%path) {callee = @Child::@peek, instance_path = ["left"], occurrence = #r17} : (!b1) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "watch", declaration = #r14, occurrence = #r15} : (!b1, !b1, !b1, !b8) -> ()
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
      EXPECT_EQ(version.value, plan->owners[version.owner].oldQ);
    } else {
      ASSERT_TRUE(version.predecessor);
      ASSERT_TRUE(version.write);
      auto &previous = plan->versions[*version.predecessor];
      auto &intent = plan->writes[*version.write];
      EXPECT_EQ(previous.owner, version.owner);
      EXPECT_EQ(previous.ordinal + 1, version.ordinal);
      EXPECT_EQ(intent.version, i);
      EXPECT_EQ(intent.predecessor, *version.predecessor);
      EXPECT_EQ(intent.value, version.value);
      EXPECT_EQ(intent.occurrence, intent.operation->getAttr("occurrence"));
      EXPECT_TRUE(intent.selectors.empty());
      EXPECT_TRUE(intent.indices.empty());
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
    auto &guard = plan->guards[bumps[i]->guard];
    ASSERT_EQ(guard.entryVersions.size(), 1u);
    EXPECT_EQ(guard.entryVersions[0].version, boundReads[0]->version);
  }
  auto firstResult = bumps[0]->results[0].result;
  auto firstYield = bumps[0]->results[0].yielded;
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
  EXPECT_EQ(plan->guards[noop->guard].domain, plan->domain);
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
      EXPECT_EQ(plan->guards[i].predecessor, i - 1);
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
        if (event.guard && *event.guard == call.guard) {
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
      EXPECT_EQ(version.value, intent.value);
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
