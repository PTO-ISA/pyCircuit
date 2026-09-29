#include "Compiler/FinalProgram.h"
#include "Compiler/ModuleGraph.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-multi-instance", path));
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

class MultiInstanceEmitTest : public ::testing::Test {
protected:
  MultiInstanceEmitTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    childSource = sourceRoot + "/child.py";
    rootSource = sourceRoot + "/root.py";
    writeFile(childSource, R"py(from pycircuit import log, module, rule

@module
def Child(source: bool, sink: bool):
    private_state: bool = False
    @rule
    def transfer():
        nonlocal private_state, sink
        assert source, "source starts high"
        private_state = source
        sink = private_state
        log("info", "private_state", private_state)
        return
    transfer()
)py");
    writeFile(rootSource, R"py(from pycircuit import log, module, rule
from .child import Child

@module
def Root():
    source: bool = True
    left_sink: bool = False
    right_sink: bool = False
    left = Child(source, left_sink)
    right = Child(source, right_sink)
    @rule
    def observe():
        nonlocal source
        log("info", "left_sink", left_sink)
        log("info", "right_sink", right_sink)
        log("info", "source", source)
        source = False
        return
    observe()
)py");

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

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, childSource, rootSource;
  std::string childBodyPath = temporary.child("child.body.mlir");
  std::string childHeaderPath = temporary.child("child.interface.mlir");
  std::string rootBodyPath = temporary.child("root.body.mlir");
  std::string rootHeaderPath = temporary.child("root.interface.mlir");
  mlir::OwningOpRef<mlir::ModuleOp> childBody, childHeader, rootBody,
      rootHeader;
};

TEST_F(MultiInstanceEmitTest,
       RepeatedChildrenEmitAndExecuteAsIndependentOwnerViews) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  ASSERT_EQ(ready->instances().size(), 3u);
  const auto instances = ready->instances();
  const auto root = ready->rootInstanceOrdinal();
  ASSERT_LT(root, instances.size());
  ASSERT_EQ(instances[root].childOrdinals.size(), 2u);
  const auto &left = instances[instances[root].childOrdinals[0]];
  const auto &right = instances[instances[root].childOrdinals[1]];
  EXPECT_NE(left.owner, right.owner);
  EXPECT_NE(left.view, right.view);
  EXPECT_EQ(left.definition, right.definition);
  ASSERT_EQ(instances[root].ownedStateOrdinals.size(), 3u);
  ASSERT_EQ(left.ownedStateOrdinals.size(), 1u);
  ASSERT_EQ(right.ownedStateOrdinals.size(), 1u);
  EXPECT_NE(left.ownedStateOrdinals.front(), right.ownedStateOrdinals.front());
  ASSERT_EQ(ready->stateCarriers().size(), ready->proposals().states.size());
  for (const auto &state : ready->proposals().states)
    EXPECT_EQ(std::count_if(
                  ready->stateCarriers().begin(), ready->stateCarriers().end(),
                  [&](const auto &c) { return c.stateID == state.stateID; }),
              1);

  auto cpp = emitFinalCpp(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(cpp))
      << "W10 RED: emitFinalCpp rejected repeated child ownership";
  EXPECT_EQ(*cpp, *emitFinalCpp(*ready, emitError()));
  for (llvm::StringRef forbidden :
       {"bool Check()", "bool Drive(", "Queue", "SimQueue"})
    EXPECT_EQ(cpp->find(forbidden.str()), std::string::npos) << forbidden.str();
  // This fixture has two unique definitions (Root and Child), but three
  // instances. The repeated Child objects must have the same concrete type.
  const std::regex classDefinition(R"(class FinalModuleDef[0-9]+ final :)");
  const std::regex childField(R"((FinalModuleDef[0-9]+) child_[0-9]+_;)");
  const std::regex rootField(R"(FinalModuleDef[0-9]+ root_;)");
  EXPECT_EQ(std::distance(
                std::sregex_iterator(cpp->begin(), cpp->end(), classDefinition),
                std::sregex_iterator()),
            2);
  std::smatch leftField, rightField;
  const std::regex leftPattern(R"((FinalModuleDef[0-9]+) child_0_;)");
  const std::regex rightPattern(R"((FinalModuleDef[0-9]+) child_1_;)");
  ASSERT_TRUE(std::regex_search(*cpp, leftField, leftPattern));
  ASSERT_TRUE(std::regex_search(*cpp, rightField, rightPattern));
  EXPECT_EQ(leftField[1].str(), rightField[1].str());
  EXPECT_EQ(
      std::distance(std::sregex_iterator(cpp->begin(), cpp->end(), childField),
                    std::sregex_iterator()),
      2);
  EXPECT_EQ(
      std::distance(std::sregex_iterator(cpp->begin(), cpp->end(), rootField),
                    std::sregex_iterator()),
      1);
  llvm::SmallVector<const FinalProgram::FinalInstanceSnapshot *> uniqueSpecs;
  size_t declaredOwnedRegisters = 0;
  for (const auto &instance : instances) {
    auto prior = llvm::find_if(uniqueSpecs, [&](const auto *candidate) {
      return candidate->definition == instance.definition &&
             candidate->staticArguments == instance.staticArguments;
    });
    if (prior == uniqueSpecs.end()) {
      uniqueSpecs.push_back(&instance);
      declaredOwnedRegisters += instance.ownedStateOrdinals.size();
    }
  }
  const std::regex ownedRegister(R"(SimDFFE<[^>]+> q[0-9]+_\{)");
  EXPECT_EQ(std::distance(
                std::sregex_iterator(cpp->begin(), cpp->end(), ownedRegister),
                std::sregex_iterator()),
            declaredOwnedRegisters);
  EXPECT_EQ(ready->proposals().states.size(), 5u);

  const std::string modelPath = temporary.child("FinalModel.generated.h");
  const std::string driverPath = temporary.child("driver.cpp");
  const std::string binaryPath = temporary.child("dut");
  writeFile(modelPath, *cpp);
  const std::string childCppType = leftField[1].str();
  std::string driver = R"cpp(#include "FinalModel.generated.h"
#include "gfsim/SimSystem.h"

#include <iostream>
#include <type_traits>

template <typename T> concept HasRegisterAll = requires(T &value) {
  value.RegisterAll(std::declval<gfsim::SimSystem &>());
};
template <typename T> concept HasFreezeObjects = requires(T &value) {
  value.FreezeObjects();
};
template <typename T> concept HasFreezeOwned = requires(T &value) {
  value.FreezeOwned(true);
};
template <typename T> concept HasWork = requires(T &value) {
  value.Work(std::uint64_t{});
};
template <typename T> concept HasXfer = requires(T &value) { value.Xfer(); };
template <typename T> concept HasReset = requires(T &value) { value.Reset(); };
template <typename T> concept HasDiscardNext = requires(T &value) {
  value.DiscardNext();
};

static_assert(!HasRegisterAll<FinalModel>);
static_assert(!HasFreezeObjects<FinalModel>);
static_assert(!HasFreezeOwned<FinalModel>);
static_assert(!HasWork<FinalModel>);
static_assert(!HasXfer<FinalModel>);
static_assert(!HasReset<FinalModel>);
static_assert(!HasDiscardNext<FinalModel>);
static_assert(!HasRegisterAll<CHILD_TYPE>);
static_assert(!HasFreezeObjects<CHILD_TYPE>);
static_assert(!HasFreezeOwned<CHILD_TYPE>);
static_assert(!HasWork<CHILD_TYPE>);
static_assert(!HasXfer<CHILD_TYPE>);
static_assert(!HasReset<CHILD_TYPE>);
static_assert(!HasDiscardNext<CHILD_TYPE>);

static void printEvents(char tag, FinalSystem &system) {
  const auto events = system.Observations().Events();
  std::cout << tag << ' ' << system.cycle() << ' ' << events.size();
  for (const auto &event : events)
    std::cout << ' ' << event.descriptor.ownerKey << ':' << event.value.bits;
  std::cout << '\n';
}

int main() {
  FinalSystem system;
  system.Build();
  system.Reset();
  if (system.Step() != gfsim::SimStepResult::Running) return 11;
  printEvents('A', system);
  if (system.Step() != gfsim::SimStepResult::Failed) return 12;
  if (system.cycle() != 1 ||
      system.state() != gfsim::SimSystemState::Failed) return 15;
  printEvents('B', system);
  system.Reset();
  if (system.cycle() != 0) return 13;
  if (system.Step() != gfsim::SimStepResult::Running) return 14;
  printEvents('C', system);
  return 0;
}
)cpp";
  for (size_t position = 0;
       (position = driver.find("CHILD_TYPE", position)) != std::string::npos;
       position += childCppType.size())
    driver.replace(position, std::string("CHILD_TYPE").size(), childCppType);
  writeFile(driverPath, driver);
  const std::string cxx = ACIR_TEST_CXX;
  ASSERT_EQ(run(cxx,
                {cxx, "-std=c++20", "-I",
                 std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
                 driverPath, "-o", binaryPath},
                temporary.child("compile.log")),
            0)
      << readFile(temporary.child("compile.log"));
  ASSERT_EQ(run(binaryPath, {binaryPath}, temporary.child("run.log")), 0)
      << readFile(temporary.child("run.log"));
  const std::string trace = readFile(temporary.child("run.log"));
  EXPECT_EQ(std::count(trace.begin(), trace.end(), '\n'), 3);
  EXPECT_NE(trace.find("A 1 5 "), std::string::npos);
  EXPECT_NE(trace.find("B 1 5 "), std::string::npos);
  EXPECT_NE(trace.find("C 1 5 "), std::string::npos);
  const auto firstLineEnd = trace.find('\n');
  ASSERT_NE(firstLineEnd, std::string::npos);
  const auto secondLineEnd = trace.find('\n', firstLineEnd + 1);
  ASSERT_NE(secondLineEnd, std::string::npos);
  const auto thirdLineEnd = trace.find('\n', secondLineEnd + 1);
  ASSERT_NE(thirdLineEnd, std::string::npos);
  const std::string firstCycle = trace.substr(0, firstLineEnd);
  const std::string secondCycle =
      trace.substr(firstLineEnd + 1, secondLineEnd - firstLineEnd - 1);
  const std::string thirdCycle =
      trace.substr(secondLineEnd + 1, thirdLineEnd - secondLineEnd - 1);
  auto values = [](const std::string &line) {
    const size_t first = line.find(' ');
    const size_t second = line.find(' ', first + 1);
    const size_t third = line.find(' ', second + 1);
    return line.substr(third + 1);
  };
  EXPECT_EQ(values(firstCycle), values(secondCycle));
  EXPECT_EQ(values(firstCycle), values(thirdCycle));
  EXPECT_NE(firstCycle.find(":0"), std::string::npos);
  EXPECT_NE(firstCycle.find(":1"), std::string::npos);
  std::istringstream eventTokens(firstCycle);
  std::string token;
  eventTokens >> token >> token >> token;
  std::set<std::string> ownerKeys;
  size_t parsedEvents = 0;
  while (eventTokens >> token) {
    const size_t colon = token.find(':');
    ASSERT_NE(colon, std::string::npos);
    ownerKeys.insert(token.substr(0, colon));
    ++parsedEvents;
  }
  EXPECT_EQ(parsedEvents, 5u);
  EXPECT_EQ(ownerKeys.size(), 3u)
      << "the two repeated child views and root have distinct owner keys";
}

TEST_F(MultiInstanceEmitTest,
       SourceUnitPermutationPreservesSemanticObservationTrace) {
  auto forward = build(false);
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(forward));
  ASSERT_TRUE(mlir::succeeded(reverse));
  auto forwardCpp = emitFinalCpp(*forward, emitError());
  auto reverseCpp = emitFinalCpp(*reverse, emitError());
  ASSERT_TRUE(mlir::succeeded(forwardCpp));
  ASSERT_TRUE(mlir::succeeded(reverseCpp));
  EXPECT_EQ(*forwardCpp, *reverseCpp);
}

TEST_F(MultiInstanceEmitTest, RejectsFormalAliasToForeignOwnerState) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_EQ(ready->instances().size(), 3u);
  auto instances = ready->instances();
  auto *left =
      instances[instances[ready->rootInstanceOrdinal()].childOrdinals.front()]
          .view;
  auto *right =
      instances[instances[ready->rootInstanceOrdinal()].childOrdinals.back()]
          .view;
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  ASSERT_FALSE(left->formalAliases.empty());
  ASSERT_FALSE(right->ownedStates.empty());
  auto alias = left->formalAliases.begin();
  alias->second = right->ownedStates.begin()->second;
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
  EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())));
}

} // namespace
} // namespace acir::compiler
