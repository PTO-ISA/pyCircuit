#include "Compiler/FinalProgram.h"
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
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-final-realization", path));
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

std::string readFile(llvm::StringRef path) {
  auto contents = llvm::MemoryBuffer::getFile(path);
  return contents ? contents.get()->getBuffer().str() : std::string{};
}

int run(llvm::StringRef program, const std::vector<std::string> &arguments,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs;
  for (const std::string &argument : arguments)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, refs, std::nullopt, redirects);
}

template <typename Program>
concept HasPostOrderInstanceOrdinals =
    requires(const Program &program) { program.postOrderInstanceOrdinals(); };

class FinalRealizationClosureTest : public ::testing::Test {
protected:
  FinalRealizationClosureTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    childSource = sourceRoot + "/child.py";
    rootSource = sourceRoot + "/root.py";
    writeFile(childSource, R"py(from pycircuit import module, rule, log

@module
def Child(first: bool, second: bool, sink: bool):
    private_state: bool = False
    @rule
    def transfer():
        nonlocal private_state, sink
        if first:
            private_state = second
            sink = private_state
            log("info", "state", private_state)
            return
        else:
            assert second, "second input is high on alternate path"
            log("debug", "alternate", second)
            return
    transfer()
)py");
    writeFile(rootSource, R"py(from pycircuit import module
from .child import Child

@module
def Root():
    one: bool = True
    zero: bool = False
    left_sink: bool = False
    right_sink: bool = False
    left = Child(one, zero, left_sink)
    right = Child(zero, one, right_sink)
)py");

    childBodyPath = temporary.child("child.body.mlir");
    childHeaderPath = temporary.child("child.interface.mlir");
    rootBodyPath = temporary.child("root.body.mlir");
    rootHeaderPath = temporary.child("root.interface.mlir");
    ASSERT_TRUE(compileUnit(childSource, "child.py", std::nullopt,
                            childBodyPath, childHeaderPath));
    ASSERT_TRUE(compileUnit(rootSource, "root.py", childHeaderPath,
                            rootBodyPath, rootHeaderPath));
    childBody = mlir::parseSourceFile<mlir::ModuleOp>(childBodyPath, &context);
    childHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(childHeaderPath, &context);
    rootBody = mlir::parseSourceFile<mlir::ModuleOp>(rootBodyPath, &context);
    rootHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(rootHeaderPath, &context);
    ASSERT_TRUE(childBody && childHeader && rootBody && rootHeader);
    ASSERT_FALSE(llvm::sys::fs::remove_directories(sourceRoot));
  }

  bool compileUnit(llvm::StringRef source, llvm::StringRef relativePath,
                   std::optional<llvm::StringRef> dependencyHeader,
                   llvm::StringRef body, llvm::StringRef header) {
    std::string stem = relativePath.str();
    std::replace(stem.begin(), stem.end(), '/', '_');
    std::replace(stem.begin(), stem.end(), '.', '_');
    const std::string capture = temporary.child(stem + ".capture.mlir");
    static constexpr llvm::StringLiteral script = R"py(
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
            {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
             source.str(), sourceRoot, capture},
            temporary.child(stem + ".capture.log")) != 0)
      return false;
    std::vector<std::string> args = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                     "--capture",
                                     capture,
                                     "--package",
                                     "verify",
                                     "--path",
                                     relativePath.str()};
    if (dependencyHeader) {
      args.push_back("--header");
      args.push_back(dependencyHeader->str());
    }
    args.insert(args.end(),
                {"--body-out", body.str(), "--interface-out", header.str()});
    const std::string log = temporary.child(stem + ".compile.log");
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, args, log) == 0)
      return true;
    llvm::errs() << readFile(log) << "\n";
    return false;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  mlir::FailureOr<FinalProgram> build(bool reverse = false) {
    SourceLinkUnit child{*childBody, *childHeader};
    SourceLinkUnit root{*rootBody, *rootHeader};
    llvm::SmallVector<SourceLinkUnit> units =
        reverse ? llvm::SmallVector<SourceLinkUnit>{child, root}
                : llvm::SmallVector<SourceLinkUnit>{root, child};
    auto analysis = buildFinalProgram(units, emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  template <typename Mutate> void expectMutationRejected(Mutate mutate) {
    auto ready = build();
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_TRUE(ready->isEmitReady());
    ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
    ASSERT_TRUE(mlir::succeeded(emitFinalVerilog(*ready, emitError())));
    ASSERT_TRUE(mutate(*ready));
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, childSource, rootSource;
  std::string childBodyPath, childHeaderPath, rootBodyPath, rootHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> childBody, childHeader, rootBody,
      rootHeader;
};

TEST_F(FinalRealizationClosureTest, RepeatedPortlessChildrenAreEmitReady) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  ASSERT_EQ(ready->instances().size(), 3u);
  EXPECT_TRUE(mlir::succeeded(emitFinalVerilog(*ready, emitError())));
}

TEST_F(FinalRealizationClosureTest,
       SourceUnitPermutationPreservesRealizationAndPostorder) {
  auto forward = build(false);
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(forward));
  ASSERT_TRUE(mlir::succeeded(reverse));
  auto forwardVerilog = emitFinalVerilog(*forward, emitError());
  auto reverseVerilog = emitFinalVerilog(*reverse, emitError());
  ASSERT_TRUE(mlir::succeeded(forwardVerilog));
  ASSERT_TRUE(mlir::succeeded(reverseVerilog));
  EXPECT_EQ(*forwardVerilog, *reverseVerilog);
  EXPECT_EQ(forward->postOrderInstanceOrdinals(),
            reverse->postOrderInstanceOrdinals());
}

TEST_F(FinalRealizationClosureTest, RejectsRepeatedChildActualOperandMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.size() != 2)
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement || placement->getNumOperands() < 2)
      return false;
    placement->setOperand(0, placement->getOperand(1));
    return true;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsChildCalleeAndStaticArgumentMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.empty())
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement)
      return false;
    placement->setAttr("definition", mlir::FlatSymbolRefAttr::get(
                                         &context, "UnknownDefinition"));
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement)
      return false;
    placement->setAttr("static_args", mlir::Builder(&context).getArrayAttr({}));
    return true;
  });
}

TEST_F(FinalRealizationClosureTest, RejectsRuleInputOperandPermutation) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      for (ac::RuleOp rule :
           instance.module.getBody().front().getOps<ac::RuleOp>()) {
        if (rule->getNumOperands() < 2 ||
            rule->getOperand(0).getType() != rule->getOperand(1).getType())
          continue;
        mlir::Value first = rule->getOperand(0);
        rule->setOperand(0, rule->getOperand(1));
        rule->setOperand(1, first);
        return true;
      }
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest, RejectsRuleInputRedirectToSameTypedHandle) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      for (ac::RuleOp rule :
           instance.module.getBody().front().getOps<ac::RuleOp>()) {
        if (rule->getNumOperands() < 2 ||
            rule->getOperand(0).getType() != rule->getOperand(1).getType())
          continue;
        rule->setOperand(0, rule->getOperand(1));
        return true;
      }
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest, RejectsConstantValueMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      bool changed = false;
      instance.module.walk([&](mlir::arith::ConstantOp constant) {
        if (changed || !mlir::isa<mlir::IntegerType>(constant.getType()))
          return;
        auto integer = mlir::dyn_cast<mlir::IntegerAttr>(constant.getValue());
        if (!integer)
          return;
        const auto next = integer.getValue().isZero() ? 1 : 0;
        constant->setAttr("value", mlir::Builder(&context).getIntegerAttr(
                                       constant.getType(), next));
        changed = true;
      });
      if (changed)
        return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest, RejectsSameTypedAndOrXorOperandMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      mlir::Value alternate;
      instance.module.walk([&](mlir::arith::ConstantOp constant) {
        if (!alternate && constant.getType().isInteger(1))
          alternate = constant.getResult();
      });
      if (!alternate)
        continue;
      bool changed = false;
      instance.module.walk([&](mlir::Operation *operation) {
        if (changed ||
            !mlir::isa<mlir::arith::AndIOp, mlir::arith::XOrIOp>(operation) ||
            operation->getNumOperands() != 2 ||
            operation->getOperand(0).getType() != alternate.getType() ||
            operation->getOperand(0) == alternate)
          return;
        operation->setOperand(0, alternate);
        changed = true;
      });
      if (changed)
        return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsSupportedOperationResultTypeMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      bool changed = false;
      instance.module.walk([&](mlir::arith::AndIOp operation) {
        if (changed || operation->getNumResults() != 1)
          return;
        operation->getResult(0).setType(mlir::IntegerType::get(&context, 2));
        changed = true;
      });
      if (changed)
        return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsSupportedOperationAttributeMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      bool changed = false;
      instance.module.walk([&](mlir::arith::AndIOp operation) {
        if (changed)
          return;
        operation->setAttr("ac.test_mutation",
                           mlir::Builder(&context).getUnitAttr());
        changed = true;
      });
      if (changed)
        return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsMovingSupportedExpressionIntoSecondRuleBlock) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      bool changed = false;
      instance.module.walk([&](mlir::Operation *operation) {
        if (changed || !mlir::isa<mlir::arith::ConstantOp, mlir::arith::AndIOp,
                                  mlir::arith::XOrIOp>(operation))
          return;
        ac::RuleOp rule = operation->getParentOfType<ac::RuleOp>();
        if (!rule || rule.getBody().empty())
          return;
        auto *secondBlock = new mlir::Block();
        rule.getBody().push_back(secondBlock);
        operation->moveBefore(secondBlock, secondBlock->end());
        changed = true;
      });
      if (changed)
        return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsMovingRuleIntoSecondOwnerModuleBlock) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      ac::RuleOp rule;
      instance.module.walk([&](ac::RuleOp candidate) {
        if (!rule)
          rule = candidate;
      });
      if (!rule)
        continue;
      auto *secondBlock = new mlir::Block();
      instance.module.getBody().push_back(secondBlock);
      rule->moveBefore(secondBlock, secondBlock->end());
      return true;
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest, RejectsRuleOutputTargetSwapAndRedirect) {
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      for (ac::RuleOp rule :
           instance.module.getBody().front().getOps<ac::RuleOp>()) {
        auto targets = rule.getTargets();
        if (targets.size() < 2 || targets[0].getType() != targets[1].getType())
          continue;
        const unsigned firstIndex = rule.getInputs().size();
        mlir::Value first = rule->getOperand(firstIndex);
        rule->setOperand(firstIndex, rule->getOperand(firstIndex + 1));
        rule->setOperand(firstIndex + 1, first);
        return true;
      }
    }
    return false;
  });
  expectMutationRejected([&](FinalProgram &program) {
    for (auto instance : program.instances()) {
      for (ac::RuleOp rule :
           instance.module.getBody().front().getOps<ac::RuleOp>()) {
        auto targets = rule.getTargets();
        if (targets.size() < 2 || targets[0].getType() != targets[1].getType())
          continue;
        const unsigned firstIndex = rule.getInputs().size();
        rule->setOperand(firstIndex, rule->getOperand(firstIndex + 1));
        return true;
      }
    }
    return false;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsPlacementControlAndActualSegmentMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.empty())
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement || placement.getClock() == placement.getReset())
      return false;
    placement.getClockMutable().set(placement.getReset());
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.empty())
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement || placement.getClock() == placement.getReset())
      return false;
    placement.getResetMutable().set(placement.getClock());
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.empty())
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    auto inputs = placement ? placement.getInputs() : mlir::ValueRange();
    if (inputs.size() < 2)
      return false;
    const unsigned inputSegmentStart = 2;
    placement->setOperand(inputSegmentStart, inputs[1]);
    return true;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsPlacementResultTypeMutationWhenResultsExist) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ac::InstanceOp placement;
  for (const auto &instance : ready->instances())
    if (instance.parentOrdinal && instance.placement &&
        instance.placement->getNumResults()) {
      placement = instance.placement;
      break;
    }
  if (!placement || placement->getNumResults() == 0)
    GTEST_SKIP() << "fixture placements have no results";
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  ASSERT_TRUE(mlir::succeeded(emitFinalVerilog(*ready, emitError())));
  placement->getResult(0).setType(mlir::IntegerType::get(&context, 2));
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
  EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
}

TEST_F(FinalRealizationClosureTest,
       RejectsArbitraryGenericPlacementAttributeMutation) {
  expectMutationRejected([&](FinalProgram &program) {
    auto &root = program.instances()[program.rootInstanceOrdinal()];
    if (root.childOrdinals.empty())
      return false;
    ac::InstanceOp placement =
        program.instances()[root.childOrdinals.front()].placement;
    if (!placement)
      return false;
    placement->setAttr("ac.review_only", mlir::Builder(&context).getUnitAttr());
    return true;
  });
}

TEST_F(FinalRealizationClosureTest,
       RejectsFrozenInstanceProposalCheckAndObservationPartitionMutations) {
  expectMutationRejected([&](FinalProgram &program) {
    auto *rows = const_cast<FinalProgram::FinalInstanceSnapshot *>(
        program.instances().data());
    if (program.instances().size() < 2 ||
        rows[1].proposalContributionOrdinals.empty())
      return false;
    rows[1].proposalContributionOrdinals.pop_back();
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto *rows = const_cast<FinalProgram::FinalInstanceSnapshot *>(
        program.instances().data());
    if (program.instances().size() < 2 || rows[1].proposalStateOrdinals.empty())
      return false;
    rows[1].proposalStateOrdinals.push_back(
        rows[1].proposalStateOrdinals.front());
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto *rows = const_cast<FinalProgram::FinalInstanceSnapshot *>(
        program.instances().data());
    if (program.instances().size() < 2 || rows[1].checkOrdinals.empty())
      return false;
    rows[1].checkOrdinals.push_back(rows[1].checkOrdinals.front());
    return true;
  });
  expectMutationRejected([&](FinalProgram &program) {
    auto *rows = const_cast<FinalProgram::FinalInstanceSnapshot *>(
        program.instances().data());
    if (program.instances().size() < 2 || rows[1].observationOrdinals.empty())
      return false;
    rows[1].observationOrdinals.pop_back();
    return true;
  });
}

TEST_F(FinalRealizationClosureTest,
       RequiresFrozenPostorderOrdinalsWhenExposed) {
  if constexpr (!HasPostOrderInstanceOrdinals<FinalProgram>) {
    GTEST_FAIL() << "W10 RED: FinalProgram needs a frozen postorder ordinal "
                    "accessor for realization closure verification";
  } else {
    expectMutationRejected([&](FinalProgram &program) {
      auto ordinals = program.postOrderInstanceOrdinals();
      if (ordinals.size() < 2)
        return false;
      auto *mutableOrdinals = const_cast<size_t *>(ordinals.data());
      std::swap(mutableOrdinals[0], mutableOrdinals[1]);
      return true;
    });
  }
}

} // namespace
} // namespace acir::compiler
