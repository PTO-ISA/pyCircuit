#if __has_include("Compiler/FinalProgram.h")
#include "Compiler/FinalProgram.h"
#define ACIR_HAS_FINAL_PROGRAM_API 1
#else
#define ACIR_HAS_FINAL_PROGRAM_API 0
#endif

#include "gtest/gtest.h"

#if ACIR_HAS_FINAL_PROGRAM_API
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-final-system", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

template <typename Program>
concept HasFinalInstanceTable = requires(const Program &program) {
  program.instances();
  program.rootInstanceOrdinal();
};

template <typename Row>
concept HasFinalInstanceFacts = requires(Row row) {
  row.ordinal;
  row.view;
  row.owner;
  row.parentOrdinal;
  row.childOrdinals;
  row.ownedStateOrdinals;
};

template <typename Carrier>
concept HasOwnedCarrierFacts = requires(Carrier carrier) {
  carrier.stateID;
  carrier.view;
  carrier.owner;
  carrier.handle;
  carrier.declaration;
  carrier.formalPort;
};

template <typename Program, typename Emit, typename Callback>
void withFinalInstances(const Program &program, Emit emit, Callback callback) {
  if constexpr (!HasFinalInstanceTable<Program>) {
    ADD_FAILURE() << "W10 RED: FinalProgram must expose instances() and "
                     "rootInstanceOrdinal() for the closed system plan";
  } else {
    auto instances = program.instances();
    using Row = std::remove_cvref_t<decltype(instances.front())>;
    if constexpr (!HasFinalInstanceFacts<Row>) {
      ADD_FAILURE() << "W10 RED: FinalInstanceSnapshot must expose ordinal, "
                       "view, owner, parentOrdinal, childOrdinals, and "
                       "ownedStateOrdinals";
    } else {
      ASSERT_FALSE(instances.empty());
      EXPECT_LT(program.rootInstanceOrdinal(), instances.size());
      EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(program, emit)));
      callback(instances, program.rootInstanceOrdinal());
    }
  }
}

class FinalSystemPlanTest : public ::testing::Test {
protected:
  FinalSystemPlanTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    leafSource = sourceRoot + "/leaf.py";
    rootSource = sourceRoot + "/root.py";
    write(leafSource, R"py(from pycircuit import module, rule, log

@module
def Leaf(source: bool, sink: bool):
    state: bool = False
    shadow: bool = False
    @rule
    def transfer():
        nonlocal state, sink
        assert source, "source is set"
        log("info", "transfer", source)
        state = source
        sink = state
        return
    transfer()
    @rule
    def alternate():
        nonlocal shadow
        assert source, "alternate source is set"
        log("debug", "alternate", source)
        shadow = True
        return
    alternate()
)py");
    write(rootSource, R"py(from pycircuit import module
from .leaf import Leaf

@module
def Root():
    source: bool = True
    left_sink: bool = False
    right_sink: bool = False
    left = Leaf(source, left_sink)
    right = Leaf(source, right_sink)
)py");
    leafHeaderPath = temporary.child("leaf.interface.mlir");
    leafBodyPath = temporary.child("leaf.body.mlir");
    rootHeaderPath = temporary.child("root.interface.mlir");
    rootBodyPath = temporary.child("root.body.mlir");
    ASSERT_TRUE(compileUnit(leafSource, "leaf.py", std::nullopt, leafBodyPath,
                            leafHeaderPath));
    ASSERT_TRUE(compileUnit(rootSource, "root.py", leafHeaderPath, rootBodyPath,
                            rootHeaderPath));
    leafBody = mlir::parseSourceFile<mlir::ModuleOp>(leafBodyPath, &context);
    leafHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(leafHeaderPath, &context);
    rootBody = mlir::parseSourceFile<mlir::ModuleOp>(rootBodyPath, &context);
    rootHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(rootHeaderPath, &context);
    ASSERT_TRUE(leafBody && leafHeader && rootBody && rootHeader);
    ASSERT_FALSE(llvm::sys::fs::remove_directories(sourceRoot));
  }

  void write(llvm::StringRef path, llvm::StringRef contents) {
    std::error_code error;
    llvm::raw_fd_ostream output(path, error);
    ASSERT_FALSE(error);
    output << contents;
  }

  bool compileUnit(llvm::StringRef source, llvm::StringRef relativePath,
                   std::optional<llvm::StringRef> dependencyHeader,
                   llvm::StringRef body, llvm::StringRef header) {
    std::string stem = relativePath.str();
    std::replace(stem.begin(), stem.end(), '/', '_');
    std::replace(stem.begin(), stem.end(), '.', '_');
    const std::string capture = temporary.child(stem + ".capture.mlir");
    static constexpr llvm::StringLiteral captureScript = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
capture = _capture_source_file(Path(sys.argv[2]), source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
    if (run(ACIR_TEST_PYTHON,
            {ACIR_TEST_PYTHON, "-c", captureScript.str(), ACIR_TEST_REPO_ROOT,
             source.str(), sourceRoot, capture},
            temporary.child(stem + ".capture.log")) != 0)
      return false;
    std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                          "--capture",
                                          capture,
                                          "--package",
                                          "verify",
                                          "--path",
                                          relativePath.str()};
    if (dependencyHeader) {
      arguments.push_back("--header");
      arguments.push_back(dependencyHeader->str());
    }
    arguments.insert(arguments.end(), {"--body-out", body.str(),
                                       "--interface-out", header.str()});
    const std::string logPath = temporary.child(stem + ".compile.log");
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, logPath) == 0)
      return true;
    auto log = llvm::MemoryBuffer::getFile(logPath);
    if (log)
      llvm::errs() << "source unit compiler failed for " << relativePath
                   << ":\n"
                   << (*log)->getBuffer() << "\n";
    return false;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  SourceLinkUnit leafUnit() const { return {*leafBody, *leafHeader}; }
  SourceLinkUnit rootUnit() const { return {*rootBody, *rootHeader}; }
  llvm::SmallVector<SourceLinkUnit> hierarchy(bool reverse = false) const {
    if (reverse)
      return {leafUnit(), rootUnit()};
    return {rootUnit(), leafUnit()};
  }
  mlir::FailureOr<FinalProgram> build(bool reverse = false) {
    auto units = hierarchy(reverse);
    auto analysis = buildFinalProgram(units, emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, leafSource, rootSource;
  std::string leafBodyPath, leafHeaderPath, rootBodyPath, rootHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> leafBody, leafHeader, rootBody, rootHeader;
};

TEST_F(FinalSystemPlanTest,
       PortlessRootOwnsRegistersAndRepeatedChildrenAreViews) {
  auto program = build();
  ASSERT_TRUE(mlir::succeeded(program));
  withFinalInstances(*program, emitError(), [&](auto instances, size_t root) {
    const auto &rootRow = instances[root];
    EXPECT_FALSE(rootRow.parentOrdinal);
    ASSERT_EQ(rootRow.childOrdinals.size(), 2u);
    const auto &left = instances[rootRow.childOrdinals[0]];
    const auto &right = instances[rootRow.childOrdinals[1]];
    EXPECT_EQ(left.parentOrdinal, root);
    EXPECT_EQ(right.parentOrdinal, root);
    EXPECT_NE(left.view, right.view);
    EXPECT_NE(left.owner, right.owner);
    EXPECT_EQ(left.view->definition, right.view->definition);
    ASSERT_EQ(left.ownedStateOrdinals.size(), 2u);
    ASSERT_EQ(right.ownedStateOrdinals.size(), 2u);
    EXPECT_NE(left.ownedStateOrdinals[0], right.ownedStateOrdinals[0]);
    EXPECT_NE(left.ownedStateOrdinals[1], right.ownedStateOrdinals[1]);
    EXPECT_EQ(rootRow.ownedStateOrdinals.size(), 3u);
  });
}

TEST_F(FinalSystemPlanTest, UnboundFormalRootPortsAreNotSynthesizedAsStorage) {
  const std::string source = sourceRoot + "/unbound.py";
  const std::string bodyPath = temporary.child("unbound.body.mlir");
  const std::string headerPath = temporary.child("unbound.interface.mlir");
  ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  write(source, R"py(from pycircuit import module, rule
@module
def Unbound(source: bool, sink: bool):
    local: bool = False
    @rule
    def transfer():
        nonlocal sink
        assert source, "source is supplied by a system"
        sink = source
        return
    transfer()
)py");
  ASSERT_TRUE(
      compileUnit(source, "unbound.py", std::nullopt, bodyPath, headerPath));
  auto body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
  auto header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
  ASSERT_TRUE(body && header);
  SourceLinkUnit unit{*body, *header};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  EXPECT_TRUE(mlir::failed(program))
      << "root data formals must be bound by a system composition; no hidden "
         "stimulus register may be synthesized";
}

TEST_F(FinalSystemPlanTest, OwnedCarrierInventoryIsExactAndFormalsOnlyAlias) {
  auto program = build();
  ASSERT_TRUE(mlir::succeeded(program));
  withFinalInstances(*program, emitError(), [&](auto instances, size_t) {
    EXPECT_EQ(program->stateCarriers().size(),
              program->proposals().states.size());
    for (const auto &carrier : program->stateCarriers()) {
      if constexpr (HasOwnedCarrierFacts<decltype(carrier)>) {
        EXPECT_TRUE(carrier.stateID);
        EXPECT_NE(carrier.view, nullptr);
        EXPECT_TRUE(carrier.owner);
        EXPECT_NE(carrier.declaration, nullptr);
        EXPECT_FALSE(carrier.formalPort);
        EXPECT_TRUE(mlir::isa<ac::RegOp>(carrier.declaration));
      } else {
        ADD_FAILURE() << "W10 RED: StateCarrierSnapshot must distinguish "
                         "owned declarations from formal aliases";
      }
    }
    for (const auto &state : program->proposals().states) {
      EXPECT_EQ(std::count_if(program->stateCarriers().begin(),
                              program->stateCarriers().end(),
                              [&](const auto &carrier) {
                                return carrier.stateID == state.stateID;
                              }),
                1)
          << "every emitted StateID must have exactly one owning ac.reg";
    }
    size_t ownedStateCount = 0;
    llvm::SmallVector<size_t> ownedStateOrdinals;
    for (const auto &instance : instances) {
      ownedStateCount += instance.ownedStateOrdinals.size();
      for (size_t stateOrdinal : instance.ownedStateOrdinals) {
        ASSERT_LT(stateOrdinal, program->proposals().states.size());
        ownedStateOrdinals.push_back(stateOrdinal);
        const auto stateID = program->proposals().states[stateOrdinal].stateID;
        size_t carrierCount = 0;
        for (const auto &carrier : program->stateCarriers()) {
          if (carrier.stateID != stateID)
            continue;
          ++carrierCount;
          EXPECT_EQ(carrier.view, instance.view);
          EXPECT_EQ(carrier.owner, instance.owner);
        }
        EXPECT_EQ(carrierCount, 1u)
            << "owned state ordinal must identify its unique local carrier";
      }
    }
    EXPECT_EQ(ownedStateCount, program->proposals().states.size());
    std::sort(ownedStateOrdinals.begin(), ownedStateOrdinals.end());
    EXPECT_EQ(std::adjacent_find(ownedStateOrdinals.begin(),
                                 ownedStateOrdinals.end()),
              ownedStateOrdinals.end());
    ASSERT_EQ(ownedStateOrdinals.size(), program->proposals().states.size());
    for (auto [ordinal, state] : llvm::enumerate(program->proposals().states)) {
      EXPECT_EQ(ownedStateOrdinals[ordinal], ordinal)
          << "every ProposalGraph StateID must have one owning instance";
      (void)state;
    }
    for (const auto &alias : program->stateAliases()) {
      ASSERT_NE(alias.view, nullptr);
      ASSERT_TRUE(alias.handle);
      ASSERT_TRUE(alias.stateID);
      EXPECT_EQ(program->resolveStateID(alias.view, alias.handle),
                alias.stateID);
      if (alias.formalState) {
        for (const auto &carrier : program->stateCarriers())
          EXPECT_NE(carrier.handle, alias.handle)
              << "formal state is an alias and cannot be a storage carrier";
      }
    }
  });
}

TEST_F(FinalSystemPlanTest,
       InstanceOrdinalsAreStableUnderSourceUnitPermutation) {
  auto forward = build(false);
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(forward));
  ASSERT_TRUE(mlir::succeeded(reverse));
  withFinalInstances(*forward, emitError(), [&](auto left, size_t leftRoot) {
    withFinalInstances(
        *reverse, emitError(), [&](auto right, size_t rightRoot) {
          ASSERT_EQ(left.size(), right.size());
          EXPECT_EQ(leftRoot, rightRoot);
          for (size_t i = 0; i < left.size(); ++i) {
            EXPECT_EQ(left[i].ordinal, right[i].ordinal);
            EXPECT_EQ(left[i].owner, right[i].owner);
            EXPECT_EQ(left[i].parentOrdinal, right[i].parentOrdinal);
            EXPECT_EQ(left[i].childOrdinals, right[i].childOrdinals);
            EXPECT_EQ(left[i].ownedStateOrdinals, right[i].ownedStateOrdinals);
          }
        });
  });
}

template <typename Program>
void mutateSystemPlan(Program &program, llvm::StringRef mutation, size_t root) {
  if constexpr (!HasFinalInstanceTable<Program>) {
    ADD_FAILURE() << "W10 RED: FinalProgram instance table is required to "
                     "exercise ownership mutations";
  } else {
    auto instances = program.instances();
    using Row = std::remove_cvref_t<decltype(instances.front())>;
    if (mutation == "parent") {
      ASSERT_FALSE(instances[root].childOrdinals.empty());
      auto &child =
          const_cast<Row &>(instances[instances[root].childOrdinals.front()]);
      child.parentOrdinal = std::nullopt;
    } else if (mutation == "children") {
      const_cast<Row &>(instances[root]).childOrdinals.clear();
    } else if (mutation == "owner") {
      ASSERT_GE(instances.size(), 3u);
      const_cast<Row &>(instances[1]).owner = instances[2].owner;
    } else if (mutation == "owned-state") {
      ASSERT_EQ(instances[1].ownedStateOrdinals.size(), 2u);
      ASSERT_EQ(instances[2].ownedStateOrdinals.size(), 2u);
      const_cast<Row &>(instances[1]).ownedStateOrdinals.front() =
          instances[2].ownedStateOrdinals.front();
    } else if (mutation == "final-ports") {
      auto child = instances[instances[root].childOrdinals.front()].view;
      auto ports =
          child->module->template getAttrOfType<mlir::ArrayAttr>("ac.ports");
      ASSERT_GE(ports.size(), 2u);
      llvm::SmallVector<mlir::Attribute> reordered(ports.begin(), ports.end());
      std::swap(reordered[0], reordered[1]);
      child->module->setAttr(
          "ac.ports",
          mlir::ArrayAttr::get(child->module.getContext(), reordered));
    } else if (mutation == "formal-alias") {
      auto child = instances[instances[root].childOrdinals.front()].view;
      ASSERT_GE(child->formalAliases.size(), 2u);
      auto first = child->formalAliases.begin();
      auto second = std::next(first);
      ASSERT_NE(first->second, second->second);
      first->second = second->second;
    } else if (mutation == "state-alias") {
      ASSERT_FALSE(program.stateAliases().empty());
      using Alias =
          std::remove_cvref_t<decltype(program.stateAliases().front())>;
      const_cast<Alias &>(program.stateAliases().front()).view = nullptr;
    } else {
      auto &proposals = const_cast<ProposalGraph &>(program.proposals());
      ASSERT_FALSE(proposals.contributions.empty());
      ASSERT_GE(instances.size(), 3u);
      proposals.contributions.front().owner = instances[2].view;
    }
  }
}

TEST_F(FinalSystemPlanTest,
       VerifierRejectsInstancePortAliasAndCommitOwnershipMutations) {
  for (llvm::StringRef mutation :
       {"parent", "children", "owner", "owned-state", "final-ports",
        "formal-alias", "state-alias", "commit-owner"}) {
    SCOPED_TRACE(mutation.str());
    auto program = build();
    ASSERT_TRUE(mlir::succeeded(program));
    withFinalInstances(*program, emitError(), [&](auto, size_t root) {
      mutateSystemPlan(*program, mutation, root);
      EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())))
          << mutation.str() << " mutation was accepted";
    });
  }
}

TEST_F(FinalSystemPlanTest,
       EveryFrozenOwnerAnchorAndSemanticMutationFailsBeforeEitherEmitter) {
  for (llvm::StringRef mutation :
       {"definition", "static-arguments", "module", "placement",
        "contribution-owner", "contribution-rule", "contribution-value",
        "contribution-source-id", "check-rule", "observation-rule"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = build();
    ASSERT_TRUE(mlir::succeeded(ready));
    auto instances = ready->instances();
    ASSERT_EQ(instances.size(), 3u);
    auto *left =
        instances[instances[ready->rootInstanceOrdinal()].childOrdinals.front()]
            .view;
    auto *right =
        instances[instances[ready->rootInstanceOrdinal()].childOrdinals.back()]
            .view;
    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);

    if (mutation == "definition") {
      ASSERT_NE(left->definition,
                instances[ready->rootInstanceOrdinal()].view->definition);
      left->definition =
          instances[ready->rootInstanceOrdinal()].view->definition;
    } else if (mutation == "static-arguments") {
      mlir::Builder builder(&context);
      auto replacement = builder.getArrayAttr({builder.getStringAttr("stale")});
      ASSERT_NE(left->staticArguments, replacement);
      left->staticArguments = replacement;
    } else if (mutation == "module") {
      auto root = instances[ready->rootInstanceOrdinal()].view;
      ASSERT_NE(root, nullptr);
      ASSERT_NE(left->module, root->module);
      left->module = root->module;
    } else if (mutation == "placement") {
      ASSERT_NE(left->placement, nullptr);
      ASSERT_NE(right->placement, nullptr);
      ASSERT_NE(left->placement, right->placement);
      left->placement = right->placement;
    } else if (mutation.starts_with("contribution-")) {
      auto &proposals = const_cast<ProposalGraph &>(ready->proposals());
      ProposalContribution *selected = nullptr;
      ProposalContribution *alternative = nullptr;
      for (ProposalContribution &first : proposals.contributions) {
        for (ProposalContribution &second : proposals.contributions) {
          if (first.owner == second.owner && first.rule != second.rule) {
            if (mutation != "contribution-value" ||
                first.value != second.value) {
              selected = &first;
              alternative = &second;
              break;
            }
          }
        }
        if (selected)
          break;
      }
      ASSERT_NE(selected, nullptr);
      ASSERT_NE(alternative, nullptr);
      if (mutation == "contribution-owner") {
        ASSERT_NE(selected->owner, right);
        selected->owner = right;
      } else if (mutation == "contribution-rule") {
        selected->rule = alternative->rule;
      } else if (mutation == "contribution-value") {
        selected->value = alternative->value;
      } else {
        ASSERT_NE(selected->sourceID, alternative->sourceID);
        selected->sourceID = alternative->sourceID;
      }
    } else if (mutation == "check-rule") {
      auto &checks = const_cast<CheckGraph &>(ready->checks());
      ASSERT_GE(checks.bindings.size(), 2u);
      CheckBinding *selected = nullptr;
      CheckBinding *alternative = nullptr;
      for (CheckBinding &first : checks.bindings)
        for (CheckBinding &second : checks.bindings)
          if (first.owner == second.owner && first.rule != second.rule) {
            selected = &first;
            alternative = &second;
            break;
          }
      ASSERT_NE(selected, nullptr);
      ASSERT_NE(alternative, nullptr);
      selected->rule = alternative->rule;
    } else {
      auto &observations =
          const_cast<ObservationGraph &>(ready->observations());
      ASSERT_GE(observations.bindings.size(), 2u);
      ObservationBinding *selected = nullptr;
      ObservationBinding *alternative = nullptr;
      for (ObservationBinding &first : observations.bindings)
        for (ObservationBinding &second : observations.bindings)
          if (first.owner == second.owner && first.rule != second.rule) {
            selected = &first;
            alternative = &second;
            break;
          }
      ASSERT_NE(selected, nullptr);
      ASSERT_NE(alternative, nullptr);
      selected->rule = alternative->rule;
    }

    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << mutation.str() << " mutation passed FinalProgram verification";
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())))
        << mutation.str() << " mutation reached C++ emission";
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())))
        << mutation.str() << " mutation reached RTL emission";
  }
}

} // namespace
} // namespace acir::compiler
#endif
