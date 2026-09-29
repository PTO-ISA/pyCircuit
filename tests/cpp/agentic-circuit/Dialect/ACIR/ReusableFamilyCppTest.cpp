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

class ReusableFamilyCppTest : public ::testing::Test {
protected:
  ReusableFamilyCppTest() : context(dialects) {
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
    leaf = Probe(first, second, first_out, second_out)
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

TEST_F(ReusableFamilyCppTest,
       SpecFamiliesKeepTiedAndDistinctLeafPortsAndObjectPathsAcrossReset) {
  auto ready = build(false);
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(mlir::succeeded(reverse));
  ASSERT_EQ(ready->instances().size(), 5u);
  ASSERT_EQ(reverse->instances().size(), 5u);
  for (const auto &instance : ready->instances())
    EXPECT_TRUE(instance.staticArguments.empty());

  auto generated = emitFinalCpp(*ready, emitError());
  auto reversed = emitFinalCpp(*reverse, emitError());
  ASSERT_TRUE(mlir::succeeded(generated));
  ASSERT_TRUE(mlir::succeeded(reversed));
  EXPECT_EQ(*generated, *reversed) << "definition SpecKeys and actual nested "
                                      "paths are independent of unit order";

  const auto instances = ready->instances();
  const size_t root = ready->rootInstanceOrdinal();
  ASSERT_EQ(instances[root].childOrdinals.size(), 2u);
  const size_t tiedBridge = instances[root].childOrdinals[0];
  const size_t splitBridge = instances[root].childOrdinals[1];
  ASSERT_EQ(instances[tiedBridge].childOrdinals.size(), 1u);
  ASSERT_EQ(instances[splitBridge].childOrdinals.size(), 1u);
  const size_t tiedProbe = instances[tiedBridge].childOrdinals.front();
  const size_t splitProbe = instances[splitBridge].childOrdinals.front();
  EXPECT_EQ(instances[tiedBridge].definition,
            instances[splitBridge].definition);
  EXPECT_EQ(instances[tiedProbe].definition, instances[splitProbe].definition);
  EXPECT_EQ(instances[root].ownedStateOrdinals.size(), 7u);
  EXPECT_TRUE(instances[tiedBridge].ownedStateOrdinals.empty());
  EXPECT_TRUE(instances[splitBridge].ownedStateOrdinals.empty());
  ASSERT_EQ(instances[tiedProbe].ownedStateOrdinals.size(), 1u);
  ASSERT_EQ(instances[splitProbe].ownedStateOrdinals.size(), 1u);
  EXPECT_NE(instances[tiedProbe].ownedStateOrdinals.front(),
            instances[splitProbe].ownedStateOrdinals.front())
      << "repeated Probe objects own independent hidden leaf state";
  EXPECT_EQ(ready->proposals().states.size(), 9u)
      << "two R-only current formals and two next formals add no storage";

  const std::regex familyDefinition(R"(class FinalModuleDef[0-9]+ final :)");
  EXPECT_EQ(
      std::distance(std::sregex_iterator(generated->begin(), generated->end(),
                                         familyDefinition),
                    std::sregex_iterator()),
      3)
      << "Root, Bridge, and Probe each have one reusable definition class";
  std::smatch rootAlias;
  const std::regex rootAliasPattern(
      R"(using FinalModel = (FinalModuleDef[0-9]+);)");
  ASSERT_TRUE(std::regex_search(*generated, rootAlias, rootAliasPattern));
  const std::string rootType = rootAlias[1].str();
  auto classBody = [&](llvm::StringRef type) {
    const std::string beginToken = "class " + type.str() + " final";
    size_t begin = generated->find(beginToken);
    if (begin == std::string::npos)
      return std::string{};
    size_t end = generated->find("\n};", begin);
    if (end == std::string::npos)
      return std::string{};
    return generated->substr(begin, end + 3 - begin);
  };
  const std::string rootClass = classBody(rootType);
  ASSERT_FALSE(rootClass.empty());
  const std::regex childField(R"((FinalModuleDef[0-9]+) child_[0-9]+_;)");
  std::vector<std::string> rootChildTypes;
  for (std::sregex_iterator it(rootClass.begin(), rootClass.end(), childField),
       end;
       it != end; ++it)
    rootChildTypes.push_back((*it)[1].str());
  ASSERT_EQ(rootChildTypes.size(), 2u);
  EXPECT_EQ(rootChildTypes[0], rootChildTypes[1]);
  const std::string bridgeType = rootChildTypes.front();
  const std::string bridgeClass = classBody(bridgeType);
  ASSERT_FALSE(bridgeClass.empty());
  std::smatch probeField;
  ASSERT_TRUE(std::regex_search(bridgeClass, probeField, childField));
  const std::string probeType = probeField[1].str();
  const std::string probeClass = classBody(probeType);
  ASSERT_FALSE(probeClass.empty());
  EXPECT_NE(probeClass.find("input_0_"), std::string::npos);
  EXPECT_NE(probeClass.find("input_1_"), std::string::npos)
      << "the shared Probe class retains distinct current-port views";

  auto formalStateID = [&](size_t ordinal, llvm::StringRef parameter) {
    for (const auto &alias : ready->stateAliases()) {
      if (alias.view != instances[ordinal].view || !alias.formalState)
        continue;
      auto name = alias.formalState.getAs<mlir::StringAttr>("parameter");
      if (name && name.getValue() == parameter)
        return alias.stateID;
    }
    return mlir::DictionaryAttr{};
  };
  const auto tiedFirst = formalStateID(tiedProbe, "first");
  const auto tiedSecond = formalStateID(tiedProbe, "second");
  const auto splitFirst = formalStateID(splitProbe, "first");
  const auto splitSecond = formalStateID(splitProbe, "second");
  ASSERT_TRUE(tiedFirst && tiedSecond && splitFirst && splitSecond);
  EXPECT_EQ(tiedFirst, tiedSecond);
  EXPECT_NE(splitFirst, splitSecond);

  auto emittedObjectPath = [&](size_t target) {
    std::vector<size_t> path;
    for (size_t cursor = target; cursor != root;
         cursor = *instances[cursor].parentOrdinal)
      path.push_back(cursor);
    std::string result = "root_";
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      const size_t child = *it;
      const size_t parent = *instances[child].parentOrdinal;
      const auto &actualChildren = instances[parent].childOrdinals;
      const auto actualPosition =
          llvm::find(actualChildren, child) - actualChildren.begin();
      if (actualPosition == actualChildren.size())
        return std::string{};
      result += ".child_" + std::to_string(actualPosition) + "_";
    }
    return result;
  };
  const std::string tiedPath = emittedObjectPath(tiedProbe);
  const std::string splitPath = emittedObjectPath(splitProbe);
  EXPECT_NE(tiedPath, splitPath);
  EXPECT_NE(generated->find(tiedPath), std::string::npos)
      << "Precommit must address the tied leaf through its actual Bridge "
         "object";
  EXPECT_NE(generated->find(splitPath), std::string::npos)
      << "Precommit must address the split leaf through its actual Bridge "
         "object";
  for (auto [ordinal, binding] :
       llvm::enumerate(ready->observations().bindings)) {
    auto owner = llvm::find_if(instances, [&](const auto &instance) {
      return instance.view == binding.owner;
    });
    ASSERT_NE(owner, instances.end());
    const auto local = llvm::find(owner->observationOrdinals, ordinal);
    if (owner->ordinal == tiedProbe || owner->ordinal == splitProbe) {
      ASSERT_NE(local, owner->observationOrdinals.end());
      const size_t representativeLocal =
          static_cast<size_t>(local - owner->observationOrdinals.begin());
      const size_t parent = *owner->parentOrdinal;
      const size_t parentPosition = static_cast<size_t>(
          llvm::find(instances[parent].childOrdinals, owner->ordinal) -
          instances[parent].childOrdinals.begin());
      const size_t rootPosition = static_cast<size_t>(
          llvm::find(instances[root].childOrdinals, parent) -
          instances[root].childOrdinals.begin());
      const std::string path = "root_.child_" + std::to_string(rootPosition) +
                               "_.child_" + std::to_string(parentPosition) +
                               "_.observation_" +
                               std::to_string(representativeLocal) + "_value_";
      EXPECT_NE(generated->find(path), std::string::npos) << path;
    }
  }

  const std::string modelPath = temporary.child("FinalModel.generated.h");
  const std::string driverPath = temporary.child("driver.cpp");
  const std::string binaryPath = temporary.child("dut");
  const std::string runLog = temporary.child("dut.log");
  writeFile(modelPath, *generated);
  std::string driver = R"cpp(#include "FinalModel.generated.h"
#include "gfsim/SimSystem.h"

#include <iostream>
#include <type_traits>

static void printTrace(char tag, FinalSystem &system) {
  const auto events = system.Observations().Events();
  std::cout << "TRACE " << tag << ' ' << system.cycle() << ' ' << events.size();
  for (const auto &event : events)
    std::cout << ' ' << event.descriptor.stableOrdinal << ':'
              << event.descriptor.ownerKey << ':' << event.value.bits << ':'
              << event.epoch;
  std::cout << '\n';
}

int main() {
  std::cout << "LIFECYCLE "
            << std::is_copy_constructible_v<FinalSystem> << ' '
            << std::is_move_constructible_v<FinalSystem> << ' '
            << std::is_copy_constructible_v<ROOT_TYPE> << ' '
            << std::is_move_constructible_v<ROOT_TYPE> << ' '
            << std::is_copy_constructible_v<BRIDGE_TYPE> << ' '
            << std::is_move_constructible_v<BRIDGE_TYPE> << ' '
            << std::is_copy_constructible_v<PROBE_TYPE> << ' '
            << std::is_move_constructible_v<PROBE_TYPE> << '\n';
  FinalSystem system;
  system.Build();
  system.Reset();
  if (system.Step() != gfsim::SimStepResult::Running) return 11;
  printTrace('A', system);
  if (system.Step() != gfsim::SimStepResult::Running) return 12;
  printTrace('B', system);
  system.Reset();
  if (system.cycle() != 0) return 13;
  if (system.Step() != gfsim::SimStepResult::Running) return 14;
  printTrace('C', system);
  if (system.Step() != gfsim::SimStepResult::Running) return 15;
  printTrace('D', system);
  return 0;
}
)cpp";
  auto replaceAll = [&](llvm::StringRef token, llvm::StringRef value) {
    size_t position = 0;
    while ((position = driver.find(token.str(), position)) !=
           std::string::npos) {
      driver.replace(position, token.size(), value.str());
      position += value.size();
    }
  };
  replaceAll("ROOT_TYPE", rootType);
  replaceAll("BRIDGE_TYPE", bridgeType);
  replaceAll("PROBE_TYPE", probeType);
  writeFile(driverPath, driver);
  const std::string cxx = ACIR_TEST_CXX;
  ASSERT_EQ(run(cxx,
                {cxx, "-std=c++20", "-I",
                 std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
                 driverPath, "-o", binaryPath},
                temporary.child("cpp-build.log")),
            0)
      << readFile(temporary.child("cpp-build.log"));
  ASSERT_EQ(run(binaryPath, {binaryPath}, runLog), 0) << readFile(runLog);

  std::vector<CppObservation> observed;
  std::istringstream output(readFile(runLog));
  std::string line;
  std::array<bool, 8> lifecycle{};
  bool lifecycleSeen = false;
  while (std::getline(output, line)) {
    if (line.rfind("LIFECYCLE ", 0) == 0) {
      std::istringstream traits(line.substr(10));
      for (bool &trait : lifecycle) {
        int value = -1;
        ASSERT_TRUE(static_cast<bool>(traits >> value));
        trait = value != 0;
      }
      lifecycleSeen = true;
      continue;
    }
    if (line.rfind("TRACE ", 0) != 0)
      continue;
    std::istringstream trace(line);
    std::string marker, tag;
    std::uint64_t cycle = 0;
    size_t count = 0;
    ASSERT_TRUE(static_cast<bool>(trace >> marker >> tag >> cycle >> count));
    ASSERT_EQ(count, 10u);
    for (size_t index = 0; index < count; ++index) {
      std::string token;
      CppObservation event;
      ASSERT_TRUE(static_cast<bool>(trace >> token));
      ASSERT_TRUE(parseCppObservation(token, event)) << token;
      EXPECT_EQ(event.epoch, cycle);
      observed.push_back(event);
    }
  }
  ASSERT_TRUE(lifecycleSeen);
  for (bool trait : lifecycle)
    EXPECT_FALSE(trait) << "generated definition modules and FinalSystem must "
                           "disable copy/move";
  ASSERT_EQ(observed.size(), 40u);
  std::map<size_t, std::uint64_t> ownerKeyByInstance;
  std::set<std::uint64_t> uniqueOwnerKeys;
  for (size_t index = 0; index < observed.size(); ++index) {
    const size_t withinRun = index % 20;
    const size_t cycle = withinRun / 10;
    const size_t ordinal = withinRun % 10;
    EXPECT_EQ(observed[index].ordinal, ordinal);
    EXPECT_EQ(observed[index].epoch, cycle + 1);
    const auto &binding = ready->observations().bindings[ordinal];
    auto owner = llvm::find_if(instances, [&](const auto &instance) {
      return instance.view == binding.owner;
    });
    ASSERT_NE(owner, instances.end());
    auto [ownerKey, inserted] =
        ownerKeyByInstance.try_emplace(owner->ordinal, observed[index].owner);
    if (!inserted)
      EXPECT_EQ(ownerKey->second, observed[index].owner);
    uniqueOwnerKeys.insert(observed[index].owner);
    std::uint64_t expected = 0;
    if (owner->ordinal == root) {
      const std::array<std::uint64_t, 4> oldOutputs =
          cycle == 0 ? std::array<std::uint64_t, 4>{0, 0, 0, 0}
                     : std::array<std::uint64_t, 4>{1, 1, 0, 1};
      ASSERT_LT(binding.requiredIndex, oldOutputs.size());
      expected = oldOutputs[binding.requiredIndex];
    } else {
      ASSERT_TRUE(instances[owner->ordinal].parentOrdinal);
      if (*instances[owner->ordinal].parentOrdinal == tiedBridge)
        expected = binding.requiredIndex == 0 || binding.requiredIndex == 1
                       ? 1
                       : (cycle == 0 ? 0 : 1);
      else {
        ASSERT_EQ(*instances[owner->ordinal].parentOrdinal, splitBridge);
        if (binding.requiredIndex == 0)
          expected = 0;
        else if (binding.requiredIndex == 1)
          expected = 1;
        else
          expected = 0;
      }
    }
    EXPECT_EQ(observed[index].value, expected);
  }
  EXPECT_EQ(uniqueOwnerKeys.size(), 3u)
      << "root and both Probe leaf instances have distinct owner keys";
  for (size_t index = 0; index < 20; ++index)
    EXPECT_EQ(observed[index + 20], observed[index]);
}

} // namespace
} // namespace acir::compiler
