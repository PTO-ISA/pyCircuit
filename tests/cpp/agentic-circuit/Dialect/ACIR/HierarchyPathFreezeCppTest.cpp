#include "Compiler/FinalProgram.h"
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
#include <cstdint>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-reusable-family-cpp", path));
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

struct CppObservation {
  std::uint32_t ordinal = 0;
  std::uint64_t owner = 0;
  std::uint64_t value = 0;
  std::uint64_t epoch = 0;
  bool operator==(const CppObservation &) const = default;
};

bool parseCppObservation(llvm::StringRef token, CppObservation &record) {
  std::string fields = token.str();
  std::replace(fields.begin(), fields.end(), ':', ' ');
  std::istringstream input(fields);
  return static_cast<bool>(input >> record.ordinal >> record.owner >>
                           record.value >> record.epoch) &&
         input.eof();
}

class HierarchyPathFreezeCppTest : public ::testing::Test {
protected:
  HierarchyPathFreezeCppTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    probeSource = sourceRoot + "/probe.py";
    bridgeSource = sourceRoot + "/bridge.py";
    rootSource = sourceRoot + "/alias_root.py";
    writeFile(probeSource, R"py(from pycircuit import log, module, rule

@module
def Probe(first: bool, second: bool, first_out: bool, second_out: bool):
    private_state: bool = False
    @rule
    def transfer():
        nonlocal private_state, first_out, second_out
        log("info", "first", first)
        log("info", "second", second)
        log("info", "private_state", private_state)
        private_state = first
        first_out = first
        second_out = second
        return
    transfer()
)py");
    writeFile(bridgeSource, R"py(from pycircuit import module
from .probe import Probe

@module
def Bridge(first: bool, second: bool,
           first_out: bool, second_out: bool):
    leaf_α = Probe(first, second, first_out, second_out)
)py");
    writeFile(rootSource, R"py(from pycircuit import log, module, rule
from .bridge import Bridge

@module
def AliasRoot():
    shared: bool = True
    distinct_left: bool = False
    distinct_right: bool = True
    tied_first: bool = False
    tied_second: bool = False
    split_first: bool = False
    split_second: bool = False
    tied = Bridge(shared, shared, tied_first, tied_second)
    split = Bridge(distinct_left, distinct_right,
                   split_first, split_second)
    @rule
    def observe_outputs():
        log("info", "tied_first", tied_first)
        log("info", "tied_second", tied_second)
        log("info", "split_first", split_first)
        log("info", "split_second", split_second)
        return
    observe_outputs()
)py");

    probeBodyPath = temporary.child("probe.body.mlir");
    probeHeaderPath = temporary.child("probe.interface.mlir");
    bridgeBodyPath = temporary.child("bridge.body.mlir");
    bridgeHeaderPath = temporary.child("bridge.interface.mlir");
    rootBodyPath = temporary.child("alias-root.body.mlir");
    rootHeaderPath = temporary.child("alias-root.interface.mlir");
    ASSERT_TRUE(compileUnit(probeSource, "probe.py", {}, probeBodyPath,
                            probeHeaderPath));
    ASSERT_TRUE(compileUnit(bridgeSource, "bridge.py", {probeHeaderPath},
                            bridgeBodyPath, bridgeHeaderPath));
    ASSERT_TRUE(compileUnit(rootSource, "alias_root.py",
                            {bridgeHeaderPath, probeHeaderPath}, rootBodyPath,
                            rootHeaderPath));
    probeBody = mlir::parseSourceFile<mlir::ModuleOp>(probeBodyPath, &context);
    probeHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(probeHeaderPath, &context);
    bridgeBody =
        mlir::parseSourceFile<mlir::ModuleOp>(bridgeBodyPath, &context);
    bridgeHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(bridgeHeaderPath, &context);
    rootBody = mlir::parseSourceFile<mlir::ModuleOp>(rootBodyPath, &context);
    rootHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(rootHeaderPath, &context);
    ASSERT_TRUE(probeBody && probeHeader && bridgeBody && bridgeHeader &&
                rootBody && rootHeader);
  }

  bool compileUnit(llvm::StringRef source, llvm::StringRef relativePath,
                   const std::vector<std::string> &headers,
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
    for (const std::string &dependency : headers) {
      args.push_back("--header");
      args.push_back(dependency);
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
    SourceLinkUnit probe{*probeBody, *probeHeader};
    SourceLinkUnit bridge{*bridgeBody, *bridgeHeader};
    SourceLinkUnit root{*rootBody, *rootHeader};
    llvm::SmallVector<SourceLinkUnit> units =
        reverse ? llvm::SmallVector<SourceLinkUnit>{probe, bridge, root}
                : llvm::SmallVector<SourceLinkUnit>{root, bridge, probe};
    auto analysis = buildFinalProgram(units, emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, probeSource, bridgeSource, rootSource;
  std::string probeBodyPath, probeHeaderPath, bridgeBodyPath, bridgeHeaderPath;
  std::string rootBodyPath, rootHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> probeBody, probeHeader, bridgeBody,
      bridgeHeader, rootBody, rootHeader;
};

TEST_F(HierarchyPathFreezeCppTest,
       UnicodeActualPathsFreezeParentLinksAndRejectCorruptedGeneratedTree) {
  auto ready = build(false);
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_EQ(ready->instances().size(), 5u);
  auto generated = emitFinalCpp(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(generated));
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(reverse));
  auto reverseCpp = emitFinalCpp(*reverse, emitError());
  ASSERT_TRUE(mlir::succeeded(reverseCpp));
  EXPECT_EQ(*generated, *reverseCpp)
      << "nested actual paths are stable under source-unit permutation";

  const std::string escapedName = "\"leaf_\\316\\261\"";
  EXPECT_NE(generated->find(escapedName), std::string::npos)
      << "placement names must use valid C++ byte escapes for UTF-8";
  std::string instrumented = *generated;
  const std::string privateMarker = "private:\n";
  const std::string testFriend =
      "private:\n  friend struct GeneratedTestAccess;\n";
  size_t friendCount = 0;
  for (size_t position = 0;
       (position = instrumented.find(privateMarker, position)) !=
       std::string::npos;
       position += testFriend.size()) {
    instrumented.replace(position, privateMarker.size(), testFriend);
    ++friendCount;
  }
  ASSERT_GE(friendCount, 4u);

  const std::string modelPath = temporary.child("FinalModel.generated.h");
  const std::string driverPath = temporary.child("driver.cpp");
  const std::string binaryPath = temporary.child("dut");
  const std::string runLog = temporary.child("dut.log");
  writeFile(modelPath, instrumented);
  writeFile(driverPath, R"cpp(#include "FinalModel.generated.h"
#include "gfsim/SimSystem.h"
#include <iostream>
#include <string>

struct GeneratedTestAccess {
  template <typename T> static auto *Parent(const T &value) {
    return value.parent_;
  }
  template <typename T> static const std::string &Path(const T &value) {
    return value.instance_path_;
  }
  static FinalModel &Root(FinalSystem &system) { return system.root_; }
  static bool PathsAndParents(FinalSystem &system) {
    auto &root = Root(system);
    auto &tied = root.child_0_;
    auto &split = root.child_1_;
    auto &tiedLeaf = tied.child_0_;
    auto &splitLeaf = split.child_0_;
    const std::string suffix = "leaf_\xCE\xB1";
    return Parent(root) == nullptr && Path(root) == "root" &&
           root.name() == "root" && Parent(tied) == &root &&
           Path(tied) == "root/tied" && tied.name() == "root/tied" &&
           Parent(split) == &root && Path(split) == "root/split" &&
           split.name() == "root/split" && Parent(tiedLeaf) == &tied &&
           Parent(splitLeaf) == &split &&
           Path(tiedLeaf) == "root/tied/" + suffix &&
           Path(splitLeaf) == "root/split/" + suffix &&
           tiedLeaf.name() == "root/tied/" + suffix &&
           splitLeaf.name() == "root/split/" + suffix;
  }
  static bool FreezeAgain(FinalSystem &system) {
    return Root(system).FreezeObjects(nullptr, "root");
  }
};

int main() {
  FinalSystem system;
  system.Build();
  if (system.state() != gfsim::SimSystemState::Built ||
      !GeneratedTestAccess::PathsAndParents(system)) return 1;
  std::cout << "PATHS_OK\n";
  if (GeneratedTestAccess::FreezeAgain(system)) return 2;
  std::cout << "FREEZE_REJECTED\n";
  system.Reset();
  if (system.state() != gfsim::SimSystemState::Ready ||
      !GeneratedTestAccess::PathsAndParents(system)) return 3;
  if (system.Step() != gfsim::SimStepResult::Running ||
      !GeneratedTestAccess::PathsAndParents(system)) return 4;
  system.Reset();
  if (!GeneratedTestAccess::PathsAndParents(system)) return 5;
  std::cout << "RESET_PATHS_OK\n";
  return 0;
}
)cpp");
  const std::string cxx = ACIR_TEST_CXX;
  ASSERT_EQ(run(cxx,
                {cxx, "-std=c++20", "-Werror", "-I",
                 std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
                 driverPath, "-o", binaryPath},
                temporary.child("cpp-build.log")),
            0)
      << readFile(temporary.child("cpp-build.log"));
  ASSERT_EQ(run(binaryPath, {binaryPath}, runLog), 0) << readFile(runLog);
  EXPECT_EQ(readFile(runLog), "PATHS_OK\nFREEZE_REJECTED\nRESET_PATHS_OK\n");

  const std::string badDirectory = temporary.child("bad-tree");
  ASSERT_FALSE(llvm::sys::fs::create_directories(badDirectory));
  const std::string badModelPath = badDirectory + "/FinalModel.generated.h";
  const std::string badDriverPath = badDirectory + "/driver.cpp";
  const std::string badBinaryPath = badDirectory + "/dut";
  std::string badModel = *generated;
  const std::string validChildConstructor =
      "child_0_(this, instance_path_ + \"/\" + " + escapedName;
  const size_t constructor = badModel.find(validChildConstructor);
  ASSERT_NE(constructor, std::string::npos);
  badModel.replace(constructor, std::string("child_0_(this").size(),
                   "child_0_(nullptr");
  writeFile(badModelPath, badModel);
  writeFile(badDriverPath, R"cpp(#include "FinalModel.generated.h"
#include "gfsim/SimSystem.h"
#include <iostream>
int main() {
  FinalSystem system;
  system.Build();
  if (system.state() != gfsim::SimSystemState::Failed) return 11;
  system.Reset();
  if (system.state() != gfsim::SimSystemState::Failed) return 12;
  std::cout << "BAD_PARENT_REJECTED\n";
  return 0;
}
)cpp");
  ASSERT_EQ(run(cxx,
                {cxx, "-std=c++20", "-Werror", "-I",
                 std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
                 badDriverPath, "-o", badBinaryPath},
                temporary.child("bad-tree-build.log")),
            0)
      << readFile(temporary.child("bad-tree-build.log"));
  const std::string badRunLog = temporary.child("bad-tree-run.log");
  ASSERT_EQ(run(badBinaryPath, {badBinaryPath}, badRunLog), 0)
      << readFile(badRunLog);
  EXPECT_EQ(readFile(badRunLog), "BAD_PARENT_REJECTED\n");
}

} // namespace
} // namespace acir::compiler
