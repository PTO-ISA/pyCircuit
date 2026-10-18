#include "pycircuit/Transforms/SourceControlFlow.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

namespace {
using namespace mlir;
namespace ac = acir::ac;

class SourceControlFlowTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect, arith::ArithDialect, scf::SCFDialect,
                        func::FuncDialect>();
    handler = std::make_unique<ScopedDiagnosticHandler>(
        &context, [&](Diagnostic &diagnostic) {
          llvm::raw_string_ostream output(diagnostics);
          diagnostic.print(output);
          return success();
        });
  }
  OwningOpRef<ModuleOp> parse(StringRef body, StringRef owner = "fold") {
    std::string text =
        R"ir(#span = {path = "scf.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
)ir";
    for (unsigned i = 0; i != 8; ++i)
      text += "#o" + std::to_string(i) + " = {site = {definition = @" +
              owner.str() +
              ", ast_path = [{kind = \"index\", value = " + std::to_string(i) +
              " : i64}]}, expansion = []}\n";
    for (unsigned value : {0u, 1u, 2u, 8u})
      text += "#w" + std::to_string(value) +
              " = #ac.static_expr<{kind = \"literal\", value = {kind = "
              "\"integer\", value = #ac.math_int<" +
              std::to_string(value) + ">}, origin = #o0, location = #span}>\n";
    text += "!b1 = !ac.bits<#w1>\n!b2 = !ac.bits<#w2>\n!b8 = !ac.bits<#w8>\n";
    text += body.str();
    // Transient SCF effect placement is intentionally normalized before the
    // complete source verifier; no production/global verifier is disabled.
    return parseSourceString<ModuleOp>(text, ParserConfig(&context, false));
  }
  std::string print(ModuleOp module) {
    std::string text;
    llvm::raw_string_ostream output(text);
    module.print(output);
    return text;
  }
  void normalized(ModuleOp module) {
    ASSERT_TRUE(succeeded(acir::normalizeSourceControlFlow(module)))
        << diagnostics;
    EXPECT_TRUE(succeeded(verify(module))) << diagnostics;
    module.walk([&](Operation *operation) {
      EXPECT_NE(operation->getName().getDialectNamespace(), "scf");
      EXPECT_FALSE((isa<ac::SourceIndexOp, ac::SourcePredicateOp>(operation)));
    });
  }
};

TEST_F(SourceControlFlowTest,
       FiniteForCarriesPayloadAndClonesIterationIdentity) {
  auto file = parse(R"ir(module {
  func.func @fold(%seed: !b8) -> !b8 {
    %zero = arith.constant 0 : index
    %one = arith.constant 1 : index
    %three = arith.constant 3 : index
    %result = "scf.for"(%zero, %three, %one, %seed) ({
    ^bb0(%i: index, %carried: !b8):
      %index = "ac.source.index"(%i) {occurrence = #o1, source_location = #span, is_signed = false} : (index) -> !b8
      %next = "ac.bits.binary"(%carried, %index) {opcode = "add", occurrence = #o2} : (!b8, !b8) -> !b8
      "scf.yield"(%next) : (!b8) -> ()
    }) {occurrence = #o0} : (index, index, index, !b8) -> !b8
    return %result : !b8
  }
})ir");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned additions = 0;
  file->walk([&](ac::BitsBinaryOp add) {
    auto occurrence = add->getAttrOfType<DictionaryAttr>("occurrence");
    auto frames = occurrence.getAs<ArrayAttr>("expansion");
    ASSERT_EQ(frames.size(), 1u);
    auto frame = cast<DictionaryAttr>(frames[0]);
    EXPECT_EQ(frame.getAs<StringAttr>("kind").getValue(), "iteration");
    EXPECT_EQ(frame.getAs<IntegerAttr>("ordinal").getUInt(), additions);
    auto value =
        frame.getAs<DictionaryAttr>("value").getAs<ac::MathIntAttr>("value");
    EXPECT_EQ(value.getCanonicalValue(), std::to_string(additions));
    ++additions;
  });
  EXPECT_EQ(additions, 3u);
}

TEST_F(SourceControlFlowTest,
       ExecuteRegionPreservesTypedSSAAndEliminatesTransport) {
  auto file = parse(R"ir(module {
  func.func @fold(%seed: !b8) -> !b8 {
    %result = "scf.execute_region"() ({
      %next = "ac.bits.binary"(%seed, %seed) {opcode = "xor"} : (!b8, !b8) -> !b8
      "scf.yield"(%next) : (!b8) -> ()
    }) : () -> !b8
    return %result : !b8
  }
})ir");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  auto function = *file->getOps<func::FuncOp>().begin();
  auto result = cast<func::ReturnOp>(function.front().getTerminator());
  EXPECT_TRUE(result.getOperand(0).getDefiningOp<ac::BitsBinaryOp>());
}

TEST_F(SourceControlFlowTest,
       MonotoneWhileProvesProgressAndCarriesHardwareValues) {
  auto file = parse(R"ir(module {
  func.func @fold(%seed: !b8) -> !b8 {
    %zero = arith.constant 0 : index
    %one = arith.constant 1 : index
    %three = arith.constant 3 : index
    %result:2 = "scf.while"(%zero, %seed) ({
    ^bb0(%i: index, %carried: !b8):
      %condition = arith.cmpi slt, %i, %three : index
      "scf.condition"(%condition, %i, %carried) : (i1, index, !b8) -> ()
    }, {
    ^bb0(%i: index, %carried: !b8):
      %index = "ac.source.index"(%i) {occurrence = #o1, source_location = #span, is_signed = false} : (index) -> !b8
      %next = "ac.bits.binary"(%carried, %index) {opcode = "add"} : (!b8, !b8) -> !b8
      %advance = arith.addi %i, %one : index
      "scf.yield"(%advance, %next) : (index, !b8) -> ()
    }) {occurrence = #o0} : (index, !b8) -> (index, !b8)
    return %result#1 : !b8
  }
})ir");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned additions = 0;
  file->walk([&](ac::BitsBinaryOp) { ++additions; });
  EXPECT_EQ(additions, 3u);
}

TEST_F(SourceControlFlowTest,
       FourStatePredicateBecomesOriginalBranchCondition) {
  auto file = parse(R"ir(module {
  "ac.module"() ({
  ^bb0(%flag: !b1, %clk: !b1, %rst: !b1):
    %result = "ac.transaction"(%clk, %rst, %flag) ({
    ^bb0(%captured: !b1):
      "ac.yield"(%captured) : (!b1) -> ()
    }, {
    ^bb0(%captured: !b1):
      %condition = "ac.source.predicate"(%captured) : (!b1) -> i1
      %choice = "scf.if"(%condition) ({
        %one = "ac.bits.constant"() {value = #w1} : () -> !b8
        "scf.yield"(%one) : (!b8) -> ()
      }, {
        %zero = "ac.bits.constant"() {value = #w0} : () -> !b8
        "scf.yield"(%zero) : (!b8) -> ()
      }) {occurrence = #o2, ac.condition_location = #span} : (i1) -> !b8
      "ac.yield"(%choice) : (!b8) -> ()
    }) {name = "choose", declaration = #o0, occurrence = #o1, ac.guard_location = #span} : (!b1, !b1, !b1) -> !b8
    "ac.yield"(%result) : (!b8) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "scf.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !b8, input_names = ["flag", "pyc_clk", "pyc_rst"], output_names = ["answer"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
})ir",
                    "Owner");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned branches = 0;
  file->walk([&](ac::SourceBranchOp branch) {
    auto transaction = branch->getParentOfType<ac::SourceTransactionOp>();
    EXPECT_EQ(branch.getCondition(),
              transaction.getBody().front().getArgument(0));
    ++branches;
  });
  EXPECT_EQ(branches, 1u);
}

TEST_F(SourceControlFlowTest, FailedTerminationProofLeavesInputUnchanged) {
  auto file = parse(R"ir(module {
  func.func @fold() {
    %zero = arith.constant 0 : index
    %one = arith.constant 1 : index
    %three = arith.constant 3 : index
    %result = "scf.while"(%zero) ({
    ^bb0(%i: index):
      %condition = arith.cmpi slt, %i, %three : index
      "scf.condition"(%condition, %i) : (i1, index) -> ()
    }, {
    ^bb0(%i: index):
      %retreat = arith.subi %i, %one : index
      "scf.yield"(%retreat) : (index) -> ()
    }) {occurrence = #o0, max_iterations = 3 : i64} : (index) -> index
    return
  }
})ir");
  ASSERT_TRUE(file) << diagnostics;
  auto before = print(*file);
  EXPECT_TRUE(failed(acir::normalizeSourceControlFlow(*file)));
  EXPECT_EQ(print(*file), before);
  EXPECT_NE(diagnostics.find("termination"), std::string::npos) << diagnostics;
}

// This is raw intermediate-IR development coverage, not Python admission or
// backend execution evidence. Independent source/System acceptance is separate.
static std::string runtimeWhileFixture() {
  return R"ir(module {
  "ac.module"() ({
  ^bb0(%input_bound: !b2, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b2
    "ac.method"(%yes) ({
    ^bb0(%path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1):
      "ac.yield"() : () -> ()
    }) {sym_name = "noop", function_type = () -> (), argument_names = [], ac.parameters = [], ac.result_constraints = [], ac.return_form = "none", ac.guard_location = #span, occurrence = #o4} : (!b1) -> ()
    %result = "ac.transaction"(%clk, %rst, %yes, %input_bound, %zero) ({
    ^bb0(%path: !b1, %limit: !b2, %initial: !b2):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %limit: !b2, %initial: !b2):
      %one = "ac.bits.constant"() {value = #w1} : () -> !b2
      %loop:2 = "scf.while"(%initial, %limit) ({
      ^bb0(%counter: !b2, %bound: !b2):
        %compare = "ac.bits.compare"(%counter, %bound) {predicate = "ult"} : (!b2, !b2) -> !b1
        %condition = "ac.source.predicate"(%compare) : (!b1) -> i1
        "scf.condition"(%condition, %counter, %bound) : (i1, !b2, !b2) -> ()
      }, {
      ^bb0(%counter: !b2, %bound: !b2):
        "ac.method.call"(%path) {callee = @Owner::@noop, instance_path = [], occurrence = #o5} : (!b1) -> ()
        "ac.expect"(%path, %path) {kind = "assert", location = #span, occurrence = #o3} : (!b1, !b1) -> ()
        %next = "ac.bits.binary"(%counter, %one) {opcode = "add"} : (!b2, !b2) -> !b2
        "scf.yield"(%next, %bound) : (!b2, !b2) -> ()
      }) {occurrence = #o2, ac.condition_location = #span} : (!b2, !b2) -> (!b2, !b2)
      "ac.yield"(%loop#0) : (!b2) -> ()
    }) {name = "run", declaration = #o0, occurrence = #o1, ac.guard_location = #span} : (!b1, !b1, !b1, !b2, !b2) -> !b2
    "ac.yield"(%result) : (!b2) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "scf.py"}, parameters = [], type_parameters = [], function_type = (!b2, !b1, !b1) -> !b2, input_names = ["bound", "pyc_clk", "pyc_rst"], output_names = ["answer"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
})ir";
}

TEST_F(SourceControlFlowTest,
       RuntimeWhileKeepsReachedControlExitCheckAndEffectIterationIdentity) {
  auto file = parse(runtimeWhileFixture(), "Owner");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned branches = 0, calls = 0, assertions = 0;
  file->walk([&](ac::SourceBranchOp branch) {
    auto marker = branch->getAttrOfType<BoolAttr>("ac.loop_continuation");
    ASSERT_TRUE(marker);
    EXPECT_TRUE(marker.getValue());
    EXPECT_EQ(branch.getNumResults(), branches == 3 ? 0u : 2u);
    auto control = branch.getCondition().getDefiningOp<ac::BitsBinaryOp>();
    ASSERT_TRUE(control);
    EXPECT_EQ(control.getOpcode(), "and");
    auto original = control.getRhs().getDefiningOp<ac::BitsCompareOp>();
    ASSERT_TRUE(original);
    EXPECT_EQ(original.getPredicate(), "ult");
    ++branches;
  });
  file->walk([&](ac::SourceMethodCallOp call) {
    auto frame = cast<DictionaryAttr>(
        call.getOccurrence().getAs<ArrayAttr>("expansion")[0]);
    EXPECT_EQ(frame.getAs<IntegerAttr>("ordinal").getUInt(), calls++);
  });
  file->walk([&](ac::SourceExpectOp check) {
    auto occurrence = check->getAttrOfType<DictionaryAttr>("occurrence");
    auto frame =
        cast<DictionaryAttr>(occurrence.getAs<ArrayAttr>("expansion")[0]);
    EXPECT_EQ(frame.getAs<IntegerAttr>("ordinal").getUInt(), assertions++);
  });
  EXPECT_EQ(branches, 4u); // Three potential bodies plus reached exit check.
  EXPECT_EQ(calls, 3u);
  EXPECT_EQ(assertions, 3u);
  EXPECT_EQ(print(*file).find("case_eq"), std::string::npos);
}

TEST_F(SourceControlFlowTest, FiniteForClonesDistinctCallAndAssertionSites) {
  auto file = parse(runtimeWhileFixture(), "Owner");
  ASSERT_TRUE(file) << diagnostics;
  scf::WhileOp original;
  file->walk([&](scf::WhileOp loop) { original = loop; });
  ASSERT_TRUE(original);
  OpBuilder builder(original);
  auto zero = arith::ConstantIndexOp::create(builder, original.getLoc(), 0);
  auto one = arith::ConstantIndexOp::create(builder, original.getLoc(), 1);
  auto three = arith::ConstantIndexOp::create(builder, original.getLoc(), 3);
  auto loop = scf::ForOp::create(builder, original.getLoc(), zero, three, one,
                                 original.getOperands());
  loop->setAttr("occurrence", original->getAttr("occurrence"));
  auto &body = loop.getRegion().front();
  body.getOperations().clear();
  IRMapping mapping;
  for (auto [formal, actual] :
       llvm::zip(original.getAfter().front().getArguments(),
                 body.getArguments().drop_front()))
    mapping.map(formal, actual);
  OpBuilder inside = OpBuilder::atBlockEnd(&body);
  for (Operation &operation : original.getAfter().front())
    inside.clone(operation, mapping);
  original.replaceAllUsesWith(loop.getResults());
  original.erase();
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned calls = 0, assertions = 0;
  file->walk([&](ac::SourceMethodCallOp call) {
    auto frames = call.getOccurrence().getAs<ArrayAttr>("expansion");
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(
        cast<DictionaryAttr>(frames[0]).getAs<IntegerAttr>("ordinal").getUInt(),
        calls++);
  });
  file->walk([&](ac::SourceExpectOp check) {
    auto frames = check->getAttrOfType<DictionaryAttr>("occurrence")
                      .getAs<ArrayAttr>("expansion");
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(
        cast<DictionaryAttr>(frames[0]).getAs<IntegerAttr>("ordinal").getUInt(),
        assertions++);
  });
  EXPECT_EQ(calls, 3u);
  EXPECT_EQ(assertions, 3u);
}

TEST_F(SourceControlFlowTest, PureRuntimeContinuationRetainsItsSamplingMarker) {
  auto text = runtimeWhileFixture();
  for (StringRef operation :
       {"        \"ac.method.call\"", "        \"ac.expect\""}) {
    auto start = text.find(operation.str());
    ASSERT_NE(start, std::string::npos);
    text.erase(start, text.find('\n', start) - start + 1);
  }
  auto file = parse(text, "Owner");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned branches = 0;
  file->walk([&](ac::SourceBranchOp branch) {
    EXPECT_TRUE(
        branch->getAttrOfType<BoolAttr>("ac.loop_continuation").getValue());
    ++branches;
  });
  EXPECT_EQ(branches, 4u);
}

TEST_F(SourceControlFlowTest, RuntimeCounterWrongDirectionCannotUseTrustedCap) {
  auto text = runtimeWhileFixture();
  auto start = text.find("opcode = \"add\"");
  ASSERT_NE(start, std::string::npos);
  text.replace(start, StringRef("opcode = \"add\"").size(), "opcode = \"sub\"");
  start = text.find("occurrence = #o2, ac.condition_location");
  ASSERT_NE(start, std::string::npos);
  text.insert(start, "max_iterations = 3 : i64, ");
  auto file = parse(text, "Owner");
  ASSERT_TRUE(file) << diagnostics;
  auto before = print(*file);
  EXPECT_TRUE(failed(acir::normalizeSourceControlFlow(*file)));
  EXPECT_EQ(print(*file), before);
  EXPECT_NE(diagnostics.find("unit progress"), std::string::npos)
      << diagnostics;
}

static std::string runtimeBreakFixture() {
  auto text = runtimeWhileFixture();
  auto rewrite = [&](StringRef before, StringRef after) {
    size_t offset = 0;
    bool found = false;
    while ((offset = text.find(before.str(), offset)) != std::string::npos) {
      text.replace(offset, before.size(), after.str());
      offset += after.size();
      found = true;
    }
    EXPECT_TRUE(found) << before.str();
  };
  rewrite("%input_bound: !b2, %clk:",
          "%input_bound: !b2, %input_stop: !b1, %clk:");
  rewrite("%yes, %input_bound, %zero)",
          "%yes, %input_bound, %zero, %input_stop)");
  rewrite("%initial: !b2):", "%initial: !b2, %stop: !b1):");
  rewrite("(!b1, !b1, !b1, !b2, !b2) -> !b2",
          "(!b1, !b1, !b1, !b2, !b2, !b1) -> !b2");
  rewrite("function_type = (!b2, !b1, !b1) -> !b2",
          "function_type = (!b2, !b1, !b1, !b1) -> !b2");
  rewrite("input_names = [\"bound\", \"pyc_clk\", \"pyc_rst\"]",
          "input_names = [\"bound\", \"stop\", \"pyc_clk\", \"pyc_rst\"]");
  rewrite("clock = 1 : i64, reset = 2 : i64",
          "clock = 2 : i64, reset = 3 : i64");
  rewrite("%loop:2", "%loop:3");
  rewrite("\"scf.while\"(%initial, %limit)",
          "\"scf.while\"(%initial, %limit, %path)");
  rewrite("^bb0(%counter: !b2, %bound: !b2):",
          "^bb0(%counter: !b2, %bound: !b2, %alive: !b1):");
  rewrite("        %condition = \"ac.source.predicate\"(%compare)",
          "        %live = \"ac.bits.binary\"(%compare, %alive) {opcode = "
          "\"and\"} : (!b1, !b1) -> !b1\n        %condition = "
          "\"ac.source.predicate\"(%live)");
  rewrite("\"scf.condition\"(%condition, %counter, %bound) : (i1, !b2, !b2)",
          "\"scf.condition\"(%condition, %counter, %bound, %alive) : (i1, !b2, "
          "!b2, !b1)");
  rewrite("        \"scf.yield\"(%next, %bound) : (!b2, !b2) -> ()",
          R"ir(        %stopped = "ac.bits.constant"() {value = #w0} : () -> !b1
        %choice:2 = "ac.branch"(%stop) ({
          "ac.yield"(%counter, %stopped) : (!b2, !b1) -> ()
        }, {
          "ac.yield"(%next, %alive) : (!b2, !b1) -> ()
        }) {occurrence = #o6, ac.condition_location = #span} : (!b1) -> (!b2, !b1)
        "scf.yield"(%choice#0, %bound, %choice#1) : (!b2, !b2, !b1) -> ())ir");
  rewrite("(!b2, !b2) -> (!b2, !b2)", "(!b2, !b2, !b1) -> (!b2, !b2, !b1)");
  return text;
}

TEST_F(SourceControlFlowTest, BreakBeforeIncrementHasAnActualExitWitness) {
  auto file = parse(runtimeBreakFixture(), "Owner");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  unsigned continuation = 0, ordinary = 0;
  file->walk([&](ac::SourceBranchOp branch) {
    if (branch->hasAttr("ac.loop_continuation"))
      ++continuation;
    else
      ++ordinary;
  });
  EXPECT_EQ(continuation, 4u);
  EXPECT_EQ(ordinary, 3u);
}

TEST_F(SourceControlFlowTest,
       ContinueWithoutAdvanceOrExitCannotProveTermination) {
  auto text = runtimeBreakFixture();
  auto position = text.find("\"ac.yield\"(%next, %alive)");
  ASSERT_NE(position, std::string::npos);
  text.replace(position, StringRef("\"ac.yield\"(%next, %alive)").size(),
               "\"ac.yield\"(%counter, %alive)");
  auto file = parse(text, "Owner");
  ASSERT_TRUE(file) << diagnostics;
  auto before = print(*file);
  EXPECT_TRUE(failed(acir::normalizeSourceControlFlow(*file)));
  EXPECT_EQ(print(*file), before);
  EXPECT_NE(diagnostics.find("unit progress"), std::string::npos)
      << diagnostics;
}

TEST_F(SourceControlFlowTest,
       NoTransientControlFlowPreservesExistingSSAHandles) {
  auto file = parse(R"ir(module {
    func.func @fold(%seed: !b8) -> !b8 { return %seed : !b8 }
  })ir");
  ASSERT_TRUE(file);
  auto function = *file->getOps<func::FuncOp>().begin();
  Value argument = function.getArgument(0);
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  EXPECT_EQ(*file->getOps<func::FuncOp>().begin(), function);
  EXPECT_EQ(function.getArgument(0), argument);
}

TEST_F(SourceControlFlowTest, ForIndexWrapCannotServeAsFiniteProgress) {
  auto file = parse(R"ir(module {
    func.func @fold() {
      %start = arith.constant 9223372036854775806 : index
      %end = arith.constant 9223372036854775807 : index
      %step = arith.constant 2 : index
      "scf.for"(%start, %end, %step) ({
      ^bb0(%i: index):
        "scf.yield"() : () -> ()
      }) {occurrence = #o0} : (index, index, index) -> ()
      return
    }
  })ir");
  ASSERT_TRUE(file) << diagnostics;
  auto before = print(*file);
  EXPECT_TRUE(failed(acir::normalizeSourceControlFlow(*file)));
  EXPECT_EQ(print(*file), before);
  EXPECT_NE(diagnostics.find("wrap"), std::string::npos) << diagnostics;
}

TEST_F(SourceControlFlowTest,
       CanonicalWhileRetainsExitConditionForwardedPayload) {
  auto file = parse(R"ir(module {
    func.func @fold(%seed: !b8) -> !b8 {
      %zero = arith.constant 0 : index
      %one = arith.constant 1 : index
      %three = arith.constant 3 : index
      %loop:2 = "scf.while"(%zero, %seed) ({
      ^bb0(%i: index, %carried: !b8):
        %payload = "ac.source.index"(%i) {occurrence = #o1, source_location = #span, is_signed = false} : (index) -> !b8
        %condition = arith.cmpi slt, %i, %three : index
        "scf.condition"(%condition, %i, %payload) : (i1, index, !b8) -> ()
      }, {
      ^bb0(%i: index, %carried: !b8):
        %advance = arith.addi %i, %one : index
        "scf.yield"(%advance, %carried) : (index, !b8) -> ()
      }) {occurrence = #o0} : (index, !b8) -> (index, !b8)
      return %loop#1 : !b8
    }
  })ir");
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_NO_FATAL_FAILURE(normalized(*file));
  auto function = *file->getOps<func::FuncOp>().begin();
  auto result = cast<func::ReturnOp>(function.front().getTerminator());
  auto payload = result.getOperand(0).getDefiningOp<ac::BitsConstantOp>();
  ASSERT_TRUE(payload);
  EXPECT_EQ(payload.getValue()
                .getTree()
                .getAs<DictionaryAttr>("value")
                .getAs<ac::MathIntAttr>("value")
                .getCanonicalValue(),
            "3");
}

} // namespace
