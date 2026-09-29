#include "Compiler/ProposalGraph.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-proposal-contracts", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void writeFile(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int emitCapture(llvm::StringRef source, llvm::StringRef root,
                llvm::StringRef output, llvm::StringRef log) {
  static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
  return run(ACIR_TEST_PYTHON,
             {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
              source.str(), root.str(), output.str()},
             log);
}

int compileCapture(llvm::StringRef capture, llvm::StringRef sourcePath,
                   llvm::StringRef body, llvm::StringRef header,
                   llvm::StringRef log) {
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS,
             {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture.str(),
              "--package", "verify", "--path", sourcePath.str(), "--body-out",
              body.str(), "--interface-out", header.str()},
             log);
}

mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

struct Compiled {
  int status;
  std::string log;
  mlir::OwningOpRef<mlir::ModuleOp> body;
  mlir::OwningOpRef<mlir::ModuleOp> header;
};

struct Analysis {
  std::unique_ptr<ModuleGraph> modules;
  std::optional<ProposalGraph> proposals;
  bool verified = false;
  std::string diagnostic;
};

class ProposalContractsTest : public ::testing::Test {
protected:
  ProposalContractsTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
  }

  Compiled compile(llvm::StringRef stem, llvm::StringRef sourceText) {
    std::string source = root + "/" + stem.str() + ".py";
    std::string capture = temporary.child(stem.str() + ".transport.mlir");
    std::string body = temporary.child(stem.str() + ".body.mlir");
    std::string header = temporary.child(stem.str() + ".interface.mlir");
    std::string pythonLog = temporary.child(stem.str() + ".python.log");
    std::string compileLog = temporary.child(stem.str() + ".compile.log");
    writeFile(source, sourceText);
    EXPECT_EQ(emitCapture(source, root, capture, pythonLog), 0);
    int status =
        compileCapture(capture, stem.str() + ".py", body, header, compileLog);
    auto log = llvm::MemoryBuffer::getFile(compileLog);
    std::string output = log ? log.get()->getBuffer().str() : std::string{};
    mlir::OwningOpRef<mlir::ModuleOp> parsed;
    mlir::OwningOpRef<mlir::ModuleOp> parsedHeader;
    if (status == 0) {
      parsed = mlir::parseSourceFile<mlir::ModuleOp>(body, &context);
      parsedHeader = mlir::parseSourceFile<mlir::ModuleOp>(header, &context);
    }
    return {status, std::move(output), std::move(parsed),
            std::move(parsedHeader)};
  }

  Analysis analyze(mlir::ModuleOp body, mlir::ModuleOp header) {
    Analysis analysis;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(analysis.diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    auto registry = SourceHeaderRegistry::create({header}, emit);
    if (mlir::failed(registry))
      return analysis;
    llvm::SmallVector<SourceLinkUnit> units{{body, header}};
    auto modules = buildSourceModuleGraph(units, *registry, emit);
    if (mlir::failed(modules))
      return analysis;
    analysis.modules = std::make_unique<ModuleGraph>(std::move(*modules));
    auto proposals = buildSourceProposalGraph(*analysis.modules, emit);
    if (mlir::failed(proposals))
      return analysis;
    analysis.proposals.emplace(std::move(*proposals));
    analysis.verified =
        mlir::succeeded(verifySourceProposals(*analysis.proposals, emit));
    return analysis;
  }

  std::pair<bool, std::string> admit(mlir::ModuleOp body,
                                     mlir::ModuleOp header) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    llvm::SmallVector<SourceLinkUnit> units{{body, header}};
    return {mlir::succeeded(admitSourceLinkUnits(units, emit)), diagnostic};
  }

  std::pair<bool, std::string> verify(ProposalGraph &graph) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return {mlir::succeeded(verifySourceProposals(graph, emit)), diagnostic};
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string root;
};

TEST_F(ProposalContractsTest, DifferentTargetsRemainIndependentWithoutPolicy) {
  Compiled compiled =
      compile("parallel", R"py(from pycircuit import module, rule

@module
def Parallel(source: bool, left: bool, right: bool):
    @rule
    def fanout():
        nonlocal left, right
        left = source
        right = source
        return

    fanout()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  ASSERT_TRUE(compiled.body && compiled.header);
  auto module = *compiled.body->getOps<ac::ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<ac::RuleOp>().begin();
  auto uses = llvm::to_vector(rule.getBody().front().getOps<ac::SourceUseOp>());
  ASSERT_EQ(uses.size(), 2u);
  auto leftTarget = uses[0].getTarget().getAs<mlir::DictionaryAttr>("state");
  auto rightTarget = uses[1].getTarget().getAs<mlir::DictionaryAttr>("state");
  EXPECT_NE(leftTarget, rightTarget);
  auto yield = mlir::cast<ac::YieldOp>(rule.getBody().front().back());
  ASSERT_EQ(yield.getValues().size(), 4u);
  EXPECT_EQ(yield.getValues()[0], uses[0].getData());
  EXPECT_EQ(yield.getValues()[1], uses[0].getEnabled());
  EXPECT_EQ(yield.getValues()[2], uses[1].getData());
  EXPECT_EQ(yield.getValues()[3], uses[1].getEnabled());
  EXPECT_FALSE(rule->hasAttr("priority"));
  EXPECT_FALSE(rule->hasAttr("stall"));

  Analysis analysis = analyze(*compiled.body, *compiled.header);
  ASSERT_TRUE(analysis.modules && analysis.proposals.has_value())
      << analysis.diagnostic;
  ASSERT_TRUE(analysis.verified) << analysis.diagnostic;
  ProposalGraph &graph = *analysis.proposals;
  ASSERT_EQ(graph.contributions.size(), 2u);
  llvm::DenseSet<mlir::Attribute> stateIDs;
  for (auto [index, contribution] : llvm::enumerate(graph.contributions)) {
    EXPECT_EQ(contribution.use, uses[index]);
    EXPECT_EQ(contribution.useID, uses[index].getId());
    EXPECT_EQ(contribution.sourceID, uses[index].getSource());
    EXPECT_EQ(contribution.data, uses[index].getData());
    EXPECT_EQ(contribution.enabled, uses[index].getEnabled());
    stateIDs.insert(contribution.stateID);
    auto state =
        llvm::find_if(graph.states, [&](const StateProposals &candidate) {
          return candidate.stateID == contribution.stateID;
        });
    ASSERT_NE(state, graph.states.end());
    EXPECT_EQ(state->commit.kind, CommitPairKind::Forward);
    ASSERT_EQ(state->commit.contributions.size(), 1u);
    EXPECT_EQ(state->commit.data, contribution.data);
    EXPECT_EQ(state->commit.enable, contribution.enabled);
  }
  EXPECT_EQ(stateIDs.size(), 2u);
  EXPECT_EQ(llvm::range_size(rule.getBody().front().getOps<ac::SourceUseOp>()),
            2u)
      << "proposal extraction must retain source operations";
}

TEST_F(ProposalContractsTest, SourcePathValidAndProposalEnableStayDistinct) {
  Compiled compiled =
      compile("conditional", R"py(from pycircuit import module, rule

@module
def Conditional(source: bool, sink: bool, ready: bool):
    @rule
    def forward():
        nonlocal sink
        if ready:
            sink = source
            return
        return

    forward()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  ASSERT_TRUE(compiled.body && compiled.header);
  auto module = *compiled.body->getOps<ac::ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<ac::RuleOp>().begin();
  auto use = *rule.getBody().front().getOps<ac::SourceUseOp>().begin();
  EXPECT_NE(use.getValid(), use.getPath());
  auto pathRead = use.getPath().getDefiningOp<ac::SourceReadOp>();
  ASSERT_TRUE(pathRead);
  auto yield = mlir::cast<ac::YieldOp>(rule.getBody().front().back());
  EXPECT_EQ(yield.getValues()[1], use.getEnabled());
  Analysis analysis = analyze(*compiled.body, *compiled.header);
  ASSERT_TRUE(analysis.proposals.has_value()) << analysis.diagnostic;
  ASSERT_TRUE(analysis.verified) << analysis.diagnostic;
  ASSERT_EQ(analysis.proposals->contributions.size(), 1u);
  const ProposalContribution &contribution =
      analysis.proposals->contributions.front();
  EXPECT_EQ(contribution.useID, use.getId());
  EXPECT_EQ(contribution.sourceID, use.getSource());
  EXPECT_EQ(contribution.path, use.getPath());
  EXPECT_EQ(contribution.valid, use.getValid());
  auto state = llvm::find_if(analysis.proposals->states,
                             [&](const StateProposals &candidate) {
                               return candidate.stateID == contribution.stateID;
                             });
  ASSERT_NE(state, analysis.proposals->states.end());
  EXPECT_EQ(state->commit.kind, CommitPairKind::Forward);
}

TEST_F(ProposalContractsTest,
       AlwaysOverlappingWritersRemainSourceFactsUntilLinkAnalysis) {
  Compiled compiled = compile("overlap", R"py(from pycircuit import module, rule

@module
def Overlap(source: bool, sink: bool):
    @rule
    def first():
        nonlocal sink
        sink = source
        return

    @rule
    def second():
        nonlocal sink
        sink = source
        return

    first()
    second()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  ASSERT_TRUE(compiled.body);
  auto module = *compiled.body->getOps<ac::ModuleOp>().begin();
  auto rules = llvm::to_vector(module.getBody().front().getOps<ac::RuleOp>());
  ASSERT_EQ(rules.size(), 2u);
  llvm::SmallVector<mlir::DictionaryAttr> targets;
  for (ac::RuleOp rule : rules) {
    auto uses =
        llvm::to_vector(rule.getBody().front().getOps<ac::SourceUseOp>());
    ASSERT_EQ(uses.size(), 1u);
    targets.push_back(uses[0].getTarget().getAs<mlir::DictionaryAttr>("state"));
  }
  EXPECT_EQ(targets[0], targets[1]);
  auto result = admit(*compiled.body, *compiled.header);
  EXPECT_FALSE(result.first);
  EXPECT_NE(result.second.find("necessarily overlapping writers"),
            std::string::npos)
      << result.second;
}

TEST_F(ProposalContractsTest, PossibleOverlapWithoutProofCarrierIsRejected) {
  Compiled compiled =
      compile("possible", R"py(from pycircuit import module, rule

@module
def Possible(source: bool, sink: bool, left_guard: bool, right_guard: bool):
    @rule
    def left():
        nonlocal sink
        if left_guard:
            sink = source
            return
        return

    @rule
    def right():
        nonlocal sink
        if right_guard:
            sink = source
            return
        return

    left()
    right()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  ASSERT_TRUE(compiled.body && compiled.header);
  auto result = admit(*compiled.body, *compiled.header);
  EXPECT_FALSE(result.first);
  EXPECT_NE(result.second.find("unavailable precommit overlap check"),
            std::string::npos)
      << result.second;
}

TEST_F(ProposalContractsTest, ConstantFalseProposalDoesNotBlockTrueCommit) {
  Compiled compiled =
      compile("constant_false", R"py(from pycircuit import module, rule

@module
def ConstantFalse(source: bool, sink: bool):
    @rule
    def left():
        nonlocal sink
        sink = source
        return

    @rule
    def right():
        nonlocal sink
        sink = source
        return

    left()
    right()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  ASSERT_TRUE(compiled.body && compiled.header);
  auto candidate = clone(*compiled.body);
  auto module = *candidate->getOps<ac::ModuleOp>().begin();
  auto rules = llvm::to_vector(module.getBody().front().getOps<ac::RuleOp>());
  ASSERT_EQ(rules.size(), 2u);
  auto firstUse = *rules[0].getBody().front().getOps<ac::SourceUseOp>().begin();
  mlir::OpBuilder builder(firstUse);
  auto disabled = mlir::arith::ConstantOp::create(builder, firstUse.getLoc(),
                                                  builder.getI1Type(),
                                                  builder.getBoolAttr(false));
  firstUse.getPathMutable().set(disabled.getResult());

  Analysis analysis = analyze(*candidate, *compiled.header);
  ASSERT_TRUE(analysis.proposals.has_value()) << analysis.diagnostic;
  ASSERT_TRUE(analysis.verified) << analysis.diagnostic;
  auto active = llvm::find_if(
      analysis.proposals->states, [](const StateProposals &state) {
        return state.commit.kind == CommitPairKind::Forward;
      });
  ASSERT_NE(active, analysis.proposals->states.end());
  ASSERT_EQ(active->commit.contributions.size(), 1u);
  const ProposalContribution &only =
      analysis.proposals->contributions[active->commit.contributions.front()];
  EXPECT_EQ(only.rule, rules[1]);
  EXPECT_TRUE(analysis.proposals->globalPermitAlways);
}

TEST_F(ProposalContractsTest,
       VerifierRejectsContributionIdentityAndSsaExchange) {
  Compiled compiled =
      compile("identity_mutation", R"py(from pycircuit import module, rule

@module
def IdentityMutation(source: bool, left: bool, right: bool):
    @rule
    def fanout():
        nonlocal left, right
        left = source
        right = source
        return

    fanout()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  Analysis analysis = analyze(*compiled.body, *compiled.header);
  ASSERT_TRUE(analysis.verified && analysis.proposals.has_value())
      << analysis.diagnostic;
  ProposalGraph baseline = *analysis.proposals;
  ASSERT_EQ(baseline.contributions.size(), 2u);

  auto expectSwapRejected = [&](llvm::StringRef label, auto swapField) {
    SCOPED_TRACE(label.str());
    ProposalGraph mutated = baseline;
    swapField(mutated.contributions[0], mutated.contributions[1]);
    auto result = verify(mutated);
    EXPECT_FALSE(result.first) << label.str();
  };
  expectSwapRejected("StateID", [](auto &left, auto &right) {
    std::swap(left.stateID, right.stateID);
  });
  expectSwapRejected("UseID", [](auto &left, auto &right) {
    std::swap(left.useID, right.useID);
  });
  expectSwapRejected("ValueID", [](auto &left, auto &right) {
    std::swap(left.sourceID, right.sourceID);
  });
  expectSwapRejected("data", [](auto &left, auto &right) {
    std::swap(left.data, right.data);
  });
  expectSwapRejected("enabled", [](auto &left, auto &right) {
    std::swap(left.enabled, right.enabled);
  });
  expectSwapRejected("value", [](auto &left, auto &right) {
    std::swap(left.value, right.value);
  });
  expectSwapRejected("valid", [](auto &left, auto &right) {
    std::swap(left.valid, right.valid);
  });
  expectSwapRejected("path", [](auto &left, auto &right) {
    std::swap(left.path, right.path);
  });
}

TEST_F(ProposalContractsTest, VerifierRejectsForgedConflictRecipes) {
  Compiled compiled =
      compile("exclusive", R"py(from pycircuit import module, rule

@module
def Exclusive(source: bool, sink: bool, other: bool, left_guard: bool, right_guard: bool):
    @rule
    def choose_left():
        nonlocal sink
        if left_guard:
            sink = source
            return

    @rule
    def choose_right():
        nonlocal sink
        if right_guard:
            sink = source
            return

    @rule
    def independent():
        nonlocal other
        other = source
        return

    choose_left()
    choose_right()
    independent()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  auto candidate = clone(*compiled.body);
  auto module = *candidate->getOps<ac::ModuleOp>().begin();
  auto rules = llvm::to_vector(module.getBody().front().getOps<ac::RuleOp>());
  ASSERT_EQ(rules.size(), 3u);
  auto leftUse = *rules[0].getBody().front().getOps<ac::SourceUseOp>().begin();
  auto rightUse = *rules[1].getBody().front().getOps<ac::SourceUseOp>().begin();
  mlir::OpBuilder builder(rightUse);
  auto one = mlir::arith::ConstantOp::create(builder, rightUse.getLoc(),
                                             builder.getI1Type(),
                                             builder.getBoolAttr(true));
  auto inverted = mlir::arith::XOrIOp::create(
      builder, rightUse.getLoc(), leftUse.getPath(), one.getResult());
  rightUse.getPathMutable().set(inverted.getResult());

  Analysis analysis = analyze(*candidate, *compiled.header);
  ASSERT_TRUE(analysis.verified && analysis.proposals.has_value())
      << analysis.diagnostic;
  ProposalGraph baseline = *analysis.proposals;
  auto exclusive =
      llvm::find_if(baseline.states, [](const StateProposals &state) {
        return state.commit.kind == CommitPairKind::ExclusiveMerge;
      });
  ASSERT_NE(exclusive, baseline.states.end());
  ASSERT_EQ(exclusive->conflicts.size(), 1u);
  auto forward =
      llvm::find_if(baseline.states, [](const StateProposals &state) {
        return state.commit.kind == CommitPairKind::Forward;
      });
  ASSERT_NE(forward, baseline.states.end());

  size_t exclusiveIndex =
      static_cast<size_t>(exclusive - baseline.states.begin());
  size_t forwardContribution = forward->contributions.front();
  {
    ProposalGraph mutated = baseline;
    mutated.states[exclusiveIndex].conflicts[0].left =
        mutated.contributions.size();
    EXPECT_FALSE(verify(mutated).first) << "out-of-range conflict index";
  }
  {
    ProposalGraph mutated = baseline;
    mutated.states[exclusiveIndex].conflicts[0].right = forwardContribution;
    EXPECT_FALSE(verify(mutated).first) << "cross-state conflict index";
  }
  {
    ProposalGraph mutated = baseline;
    ProposalConflict &proof = mutated.states[exclusiveIndex].conflicts[0];
    mutated.contributions[proof.right].path =
        mutated.contributions[proof.left].path;
    mutated.contributions[proof.right].valid =
        mutated.contributions[proof.left].valid;
    proof.kind = ProposalConflictKind::MutuallyExclusive;
    EXPECT_FALSE(verify(mutated).first) << "forged mutual-exclusion proof";
  }
  {
    ProposalGraph mutated = baseline;
    StateProposals &state = mutated.states[exclusiveIndex];
    state.contributions.push_back(state.contributions.front());
    state.commit.contributions.push_back(state.commit.contributions.front());
    EXPECT_FALSE(verify(mutated).first) << "missing pairwise conflict coverage";
  }
  {
    ProposalGraph mutated = baseline;
    mutated.states[exclusiveIndex].commit.contributions.pop_back();
    EXPECT_FALSE(verify(mutated).first)
        << "ExclusiveMerge missing one contributor";
  }
  {
    ProposalGraph mutated = baseline;
    mutated.states[exclusiveIndex].commit.contributions.push_back(
        forwardContribution);
    EXPECT_FALSE(verify(mutated).first)
        << "ExclusiveMerge contains a foreign contributor";
  }
}

TEST_F(ProposalContractsTest, VerifierRejectsForgedCommitRecipes) {
  Compiled compiled =
      compile("commit_mutation", R"py(from pycircuit import module, rule

@module
def CommitMutation(source: bool, left: bool, right: bool):
    @rule
    def fanout():
        nonlocal left, right
        left = source
        right = source
        return

    fanout()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  Analysis analysis = analyze(*compiled.body, *compiled.header);
  ASSERT_TRUE(analysis.verified && analysis.proposals.has_value())
      << analysis.diagnostic;
  ProposalGraph baseline = *analysis.proposals;
  llvm::SmallVector<size_t> forwards;
  for (auto [index, state] : llvm::enumerate(baseline.states))
    if (state.commit.kind == CommitPairKind::Forward)
      forwards.push_back(index);
  ASSERT_EQ(forwards.size(), 2u);

  {
    ProposalGraph mutated = baseline;
    StateProposals &left = mutated.states[forwards[0]];
    const StateProposals &right = mutated.states[forwards[1]];
    size_t foreign = right.commit.contributions.front();
    left.commit.contributions.assign({foreign});
    left.commit.data = mutated.contributions[foreign].data;
    left.commit.enable = mutated.contributions[foreign].enabled;
    EXPECT_FALSE(verify(mutated).first) << "cross-state forward contribution";
  }
  {
    ProposalGraph mutated = baseline;
    StateProposals &left = mutated.states[forwards[0]];
    left.commit.contributions.assign({mutated.contributions.size()});
    EXPECT_EXIT(
        {
          auto result = verify(mutated);
          std::_Exit(result.first ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "")
        << "out-of-range Forward contribution must return failure, not crash";
  }
}

TEST_F(ProposalContractsTest, VerifierRejectsForgedGlobalPermit) {
  Compiled compiled =
      compile("permit_mutation", R"py(from pycircuit import module, rule

@module
def PermitMutation(source: bool, sink: bool):
    @rule
    def forward():
        nonlocal sink
        sink = source
        return

    forward()
)py");
  ASSERT_EQ(compiled.status, 0) << compiled.log;
  Analysis analysis = analyze(*compiled.body, *compiled.header);
  ASSERT_TRUE(analysis.verified && analysis.proposals.has_value())
      << analysis.diagnostic;
  ProposalGraph mutated = *analysis.proposals;
  mutated.globalPermitAlways = false;
  EXPECT_FALSE(verify(mutated).first);
}

} // namespace
} // namespace acir::compiler
