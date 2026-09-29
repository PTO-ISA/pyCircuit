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
        llvm::sys::fs::createUniqueDirectory("acir-multi-instance-rtl", path));
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

struct ObservationRecord {
  std::uint32_t ordinal = 0;
  std::uint64_t owner = 0;
  std::uint64_t registration = 0;
  std::uint64_t site = 0;
  std::uint64_t value = 0;
  std::uint64_t evaluationEpoch = 0;
  std::uint64_t commitEpoch = 0;

  bool operator==(const ObservationRecord &) const = default;
};

bool parseObservation(llvm::StringRef line, ObservationRecord &record) {
  std::istringstream input(line.str());
  std::string marker;
  return static_cast<bool>(input >> marker >> record.ordinal >> record.owner >>
                           record.registration >> record.site >> record.value >>
                           record.evaluationEpoch >> record.commitEpoch) &&
         marker == "AC_OBS" && input.eof();
}

class ReusableFamilyVerilogTest : public ::testing::Test {
protected:
  ReusableFamilyVerilogTest() : context(dialects) {
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
        assert source, "children require the old source Q"
        log("info", "private_state", private_state)
        private_state = source
        sink = private_state
        return
    transfer()
)py");
    writeFile(rootSource, R"py(from pycircuit import log, module, rule
from .child import Child

@module
def Root():
    source: bool = True
    source_next: bool = True
    source_later: bool = True
    source_final: bool = False
    left_sink: bool = False
    right_sink: bool = False
    left = Child(source, left_sink)
    right = Child(source, right_sink)
    @rule
    def observe():
        nonlocal source, source_next, source_later, source_final
        log("info", "left_sink", left_sink)
        log("info", "right_sink", right_sink)
        log("info", "source", source)
        source = source_next
        source_next = source_later
        source_later = source_final
        source_final = False
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

  bool
  compileUnit(llvm::StringRef source, llvm::StringRef relativePath,
              std::optional<llvm::StringRef> dependencyHeader,
              llvm::StringRef body, llvm::StringRef header,
              std::optional<llvm::StringRef> additionalHeader = std::nullopt) {
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
    std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                          "--capture",
                                          capture,
                                          "--package",
                                          "verify",
                                          "--path",
                                          relativePath.str()};
    for (std::optional<llvm::StringRef> dependency :
         {dependencyHeader, additionalHeader}) {
      if (!dependency)
        continue;
      arguments.push_back("--header");
      arguments.push_back(dependency->str());
    }
    arguments.insert(arguments.end(), {"--body-out", body.str(),
                                       "--interface-out", header.str()});
    const std::string log = temporary.child(stem + ".compile.log");
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log) == 0)
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

TEST_F(ReusableFamilyVerilogTest,
       ReusedParentFamilyPreservesTiedAndDistinctLeafCurrentAliases) {
  sourceRoot = temporary.child("formal-alias-src");
  ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  const std::string probeSource = sourceRoot + "/probe.py";
  const std::string bridgeSource = sourceRoot + "/bridge.py";
  const std::string rootSource = sourceRoot + "/alias_root.py";
  writeFile(probeSource, R"py(from pycircuit import log, module, rule

@module
def Probe(first: bool, second: bool, first_out: bool, second_out: bool):
    @rule
    def transfer():
        nonlocal first_out, second_out
        log("info", "first", first)
        log("info", "second", second)
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

  const std::string probeBodyPath = temporary.child("probe.body.mlir");
  const std::string probeHeaderPath = temporary.child("probe.interface.mlir");
  const std::string bridgeBodyPath = temporary.child("bridge.body.mlir");
  const std::string bridgeHeaderPath = temporary.child("bridge.interface.mlir");
  const std::string aliasRootBodyPath = temporary.child("alias-root.body.mlir");
  const std::string aliasRootHeaderPath =
      temporary.child("alias-root.interface.mlir");
  ASSERT_TRUE(compileUnit(probeSource, "probe.py", std::nullopt, probeBodyPath,
                          probeHeaderPath));
  ASSERT_TRUE(compileUnit(bridgeSource, "bridge.py", probeHeaderPath,
                          bridgeBodyPath, bridgeHeaderPath));
  ASSERT_TRUE(compileUnit(rootSource, "alias_root.py", bridgeHeaderPath,
                          aliasRootBodyPath, aliasRootHeaderPath,
                          llvm::StringRef(probeHeaderPath)));
  auto probeBody =
      mlir::parseSourceFile<mlir::ModuleOp>(probeBodyPath, &context);
  auto probeHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(probeHeaderPath, &context);
  auto bridgeBody =
      mlir::parseSourceFile<mlir::ModuleOp>(bridgeBodyPath, &context);
  auto bridgeHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(bridgeHeaderPath, &context);
  auto aliasRootBody =
      mlir::parseSourceFile<mlir::ModuleOp>(aliasRootBodyPath, &context);
  auto aliasRootHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(aliasRootHeaderPath, &context);
  ASSERT_TRUE(probeBody && probeHeader && bridgeBody && bridgeHeader &&
              aliasRootBody && aliasRootHeader);

  llvm::SmallVector<SourceLinkUnit> units = {
      {*aliasRootBody, *aliasRootHeader},
      {*bridgeBody, *bridgeHeader},
      {*probeBody, *probeHeader},
  };
  auto analysis = buildFinalProgram(units, emitError());
  ASSERT_TRUE(mlir::succeeded(analysis));
  auto ready = materializeFinalProgram(std::move(*analysis), emitError());
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_EQ(ready->instances().size(), 5u);
  const auto instances = ready->instances();
  const auto rootOrdinal = ready->rootInstanceOrdinal();
  ASSERT_EQ(instances[rootOrdinal].childOrdinals.size(), 2u);
  const size_t tiedBridgeOrdinal = instances[rootOrdinal].childOrdinals[0];
  const size_t splitBridgeOrdinal = instances[rootOrdinal].childOrdinals[1];
  ASSERT_EQ(instances[tiedBridgeOrdinal].childOrdinals.size(), 1u);
  ASSERT_EQ(instances[splitBridgeOrdinal].childOrdinals.size(), 1u);
  const size_t tiedProbeOrdinal =
      instances[tiedBridgeOrdinal].childOrdinals.front();
  const size_t splitProbeOrdinal =
      instances[splitBridgeOrdinal].childOrdinals.front();
  ASSERT_EQ(instances[tiedProbeOrdinal].definition,
            instances[splitProbeOrdinal].definition);

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
  const auto tiedFirstState = formalStateID(tiedProbeOrdinal, "first");
  const auto tiedSecondState = formalStateID(tiedProbeOrdinal, "second");
  const auto splitFirstState = formalStateID(splitProbeOrdinal, "first");
  const auto splitSecondState = formalStateID(splitProbeOrdinal, "second");
  ASSERT_TRUE(tiedFirstState && tiedSecondState && splitFirstState &&
              splitSecondState);
  EXPECT_EQ(tiedFirstState, tiedSecondState)
      << "the first Bridge binds both leaf current formals to one parent Q";
  EXPECT_NE(splitFirstState, splitSecondState)
      << "the second Bridge binds the leaf current formals to different parent "
         "Qs";

  auto verilog = emitFinalVerilog(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(verilog));
  const std::regex bridgeFamily(R"(module\s+(ac_[a-z0-9_]*bridge)\s*\()");
  const std::regex probeFamily(R"(module\s+(ac_[a-z0-9_]*probe)\s*\()");
  const std::regex aliasRootFamily(
      R"(module\s+(ac_[a-z0-9_]*alias_root)\s*\()");
  std::smatch bridgeMatch, probeMatch, aliasRootMatch;
  ASSERT_TRUE(std::regex_search(*verilog, aliasRootMatch, aliasRootFamily));
  ASSERT_TRUE(std::regex_search(*verilog, bridgeMatch, bridgeFamily));
  ASSERT_TRUE(std::regex_search(*verilog, probeMatch, probeFamily));
  const std::string aliasRootName = aliasRootMatch[1].str();
  const std::string bridgeName = bridgeMatch[1].str();
  const std::string probeName = probeMatch[1].str();
  EXPECT_EQ(std::distance(std::sregex_iterator(verilog->begin(), verilog->end(),
                                               bridgeFamily),
                          std::sregex_iterator()),
            1);
  EXPECT_EQ(std::distance(std::sregex_iterator(verilog->begin(), verilog->end(),
                                               probeFamily),
                          std::sregex_iterator()),
            1);

  auto extractModule = [&](llvm::StringRef name) {
    const std::string opening = "module " + name.str();
    const size_t begin = verilog->find(opening);
    if (begin == std::string::npos)
      return std::string{};
    const size_t end = verilog->find("endmodule", begin);
    if (end == std::string::npos)
      return std::string{};
    return verilog->substr(begin, end - begin);
  };
  const std::string rootText = extractModule(aliasRootName);
  const std::string bridgeText = extractModule(bridgeName);
  const std::string probeText = extractModule(probeName);
  ASSERT_FALSE(rootText.empty());
  ASSERT_FALSE(bridgeText.empty());
  ASSERT_FALSE(probeText.empty());
  EXPECT_NE(probeText.find("input logic first_q"), std::string::npos);
  EXPECT_NE(probeText.find("input logic second_q"), std::string::npos);
  EXPECT_NE(probeText.find("output logic first_out_d"), std::string::npos);
  EXPECT_NE(probeText.find("output logic first_out_e"), std::string::npos);
  EXPECT_NE(probeText.find("output logic second_out_d"), std::string::npos);
  EXPECT_NE(probeText.find("output logic second_out_e"), std::string::npos);
  const std::regex probeRegister(R"(always_ff\s*@\(posedge\s+clk\))");
  EXPECT_EQ(std::distance(std::sregex_iterator(probeText.begin(),
                                               probeText.end(), probeRegister),
                          std::sregex_iterator()),
            0)
      << "R-only formal inputs and proposal outputs do not add Probe storage";
  const std::regex probeInstance(probeName + R"(\s+child_[0-9]+\s*\()");
  EXPECT_EQ(std::distance(std::sregex_iterator(bridgeText.begin(),
                                               bridgeText.end(), probeInstance),
                          std::sregex_iterator()),
            1);
  const std::regex bridgeInstance(bridgeName + R"(\s+child_[0-9]+\s*\()");
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               bridgeInstance),
                          std::sregex_iterator()),
            2);
  EXPECT_NE(bridgeText.find(".first_q(first_q)"), std::string::npos);
  EXPECT_NE(bridgeText.find(".second_q(second_q)"), std::string::npos)
      << "the reusable parent definition forwards both current ports "
         "separately";

  const std::regex bridgeCall(bridgeName +
                              R"(\s+child_([0-9]+)\s*\(([\s\S]*?)\);)");
  std::map<size_t, std::pair<std::string, std::string>> actualInputs;
  for (std::sregex_iterator it(rootText.begin(), rootText.end(), bridgeCall),
       end;
       it != end; ++it) {
    const size_t ordinal = std::stoul((*it)[1].str());
    const std::string connections = (*it)[2].str();
    const std::regex firstActual(R"(\.first_q\s*\(([^)]+)\))");
    const std::regex secondActual(R"(\.second_q\s*\(([^)]+)\))");
    std::smatch first, second;
    ASSERT_TRUE(std::regex_search(connections, first, firstActual));
    ASSERT_TRUE(std::regex_search(connections, second, secondActual));
    actualInputs.emplace(ordinal,
                         std::make_pair(first[1].str(), second[1].str()));
  }
  ASSERT_EQ(actualInputs.size(), 2u);
  size_t tiedInputBindings = 0;
  size_t splitInputBindings = 0;
  for (const auto &[ordinal, inputsForBridge] : actualInputs) {
    (void)ordinal;
    if (inputsForBridge.first == inputsForBridge.second)
      ++tiedInputBindings;
    else
      ++splitInputBindings;
  }
  EXPECT_EQ(tiedInputBindings, 1u);
  EXPECT_EQ(splitInputBindings, 1u);

  const std::string svPath = temporary.child("FormalAlias.generated.sv");
  const std::string tbPath = temporary.child("formal_alias_tb.sv");
  const std::string simPath = temporary.child("formal_alias.vvp");
  const std::string simLog = temporary.child("formal_alias.log");
  writeFile(svPath, *verilog);
  writeFile(tbPath, R"sv(module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModelSim dut(.clk(clk), .reset(reset));
  task automatic tick;
    begin #1 clk = 1'b1; #2 clk = 1'b0; #1; end
  endtask
  initial begin
    tick();
    reset = 1'b0;
    tick();
    tick();
    reset = 1'b1;
    tick();
    reset = 1'b0;
    tick();
    tick();
    $finish;
  end
endmodule
)sv");
  const std::string iverilog = ACIR_TEST_IVERILOG;
  const std::string vvp = ACIR_TEST_VVP;
  ASSERT_EQ(run(iverilog,
                {iverilog, "-g2012", "-s", "tb", "-o", simPath, svPath, tbPath},
                temporary.child("formal-alias-iverilog.log")),
            0)
      << readFile(temporary.child("formal-alias-iverilog.log"));
  ASSERT_EQ(run(vvp, {vvp, simPath}, simLog), 0) << readFile(simLog);

  std::vector<ObservationRecord> records;
  std::istringstream simulation(readFile(simLog));
  std::string line;
  while (std::getline(simulation, line)) {
    if (line.rfind("AC_OBS ", 0) != 0)
      continue;
    ObservationRecord record;
    ASSERT_TRUE(parseObservation(line, record)) << line;
    records.push_back(record);
  }
  ASSERT_EQ(records.size(), 32u);
  ASSERT_EQ(ready->observations().bindings.size(), 8u);
  for (size_t index = 0; index < records.size(); ++index) {
    const size_t perRun = index % 16;
    const size_t cycle = perRun / 8;
    const size_t ordinal = perRun % 8;
    const auto &binding = ready->observations().bindings[ordinal];
    EXPECT_EQ(records[index].ordinal, ordinal);
    EXPECT_EQ(records[index].site, binding.requiredIndex);
    EXPECT_EQ(records[index].registration, 0u);
    EXPECT_EQ(records[index].evaluationEpoch, cycle);
    EXPECT_EQ(records[index].commitEpoch, cycle + 1);
    auto owner = llvm::find_if(instances, [&](const auto &instance) {
      return instance.view == binding.owner;
    });
    ASSERT_NE(owner, instances.end());
    const size_t ownerOrdinal = owner->ordinal;
    EXPECT_EQ(records[index].owner, ownerOrdinal);

    std::uint64_t expected = 0;
    if (ownerOrdinal == rootOrdinal) {
      const std::array<std::uint64_t, 4> outputOldQ =
          cycle == 0 ? std::array<std::uint64_t, 4>{0, 0, 0, 0}
                     : std::array<std::uint64_t, 4>{1, 1, 0, 1};
      ASSERT_LT(binding.requiredIndex, outputOldQ.size());
      expected = outputOldQ[binding.requiredIndex];
    } else {
      const auto parentOrdinal = instances[ownerOrdinal].parentOrdinal;
      ASSERT_TRUE(parentOrdinal);
      if (*parentOrdinal == tiedBridgeOrdinal)
        expected = 1;
      else {
        ASSERT_EQ(*parentOrdinal, splitBridgeOrdinal);
        expected = binding.requiredIndex == 0 ? 0 : 1;
      }
    }
    EXPECT_EQ(records[index].value, expected)
        << "two R-only formal reads must stay distinct through repeated "
           "parent and leaf families";
  }
  for (size_t index = 0; index < 16; ++index)
    EXPECT_EQ(records[index + 16], records[index]);
}

} // namespace
} // namespace acir::compiler
