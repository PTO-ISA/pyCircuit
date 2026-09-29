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

class MultiInstanceVerilogTest : public ::testing::Test {
protected:
  MultiInstanceVerilogTest() : context(dialects) {
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

TEST_F(MultiInstanceVerilogTest,
       RepeatedChildrenUseHierarchicalRtlAndTransactionalObservationRecords) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  ASSERT_EQ(ready->instances().size(), 3u);
  for (const auto &instance : ready->instances())
    EXPECT_TRUE(instance.staticArguments.empty())
        << "this RTL slice intentionally covers empty static arguments only";

  auto verilog = emitFinalVerilog(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(verilog));
  EXPECT_EQ(*verilog, *emitFinalVerilog(*ready, emitError()));

  const std::regex rootFamily(R"(module\s+(ac_[a-z0-9_]*root)\s*\()");
  const std::regex childFamily(R"(module\s+(ac_[a-z0-9_]*child)\s*\()");
  const std::regex synthTop(R"(module\s+FinalModel\s*\()");
  const std::regex simulationTop(R"(module\s+FinalModelSim\s*\()");
  auto countMatches = [&](const std::regex &pattern) {
    return std::distance(
        std::sregex_iterator(verilog->begin(), verilog->end(), pattern),
        std::sregex_iterator());
  };
  EXPECT_EQ(countMatches(rootFamily), 1);
  EXPECT_EQ(countMatches(childFamily), 1)
      << "the repeated Child source definition has one reusable RTL family";
  EXPECT_EQ(countMatches(synthTop), 1);
  EXPECT_EQ(countMatches(simulationTop), 1);
  EXPECT_NE(verilog->find("subtree_error"), std::string::npos);
  EXPECT_NE(verilog->find("root_commit_ok"), std::string::npos);
  EXPECT_NE(verilog->find("FinalModelSim"), std::string::npos);

  std::smatch rootMatch, childMatch;
  ASSERT_TRUE(std::regex_search(*verilog, rootMatch, rootFamily));
  ASSERT_TRUE(std::regex_search(*verilog, childMatch, childFamily));
  const std::string rootName = rootMatch[1].str();
  const std::string childName = childMatch[1].str();
  std::string childText = *verilog;
  const size_t childStart = childText.find("module " + childName);
  ASSERT_NE(childStart, std::string::npos);
  const size_t childEnd = childText.find("endmodule", childStart);
  ASSERT_NE(childEnd, std::string::npos);
  childText = childText.substr(childStart, childEnd - childStart);
  EXPECT_NE(childText.find("input logic source_q"), std::string::npos);
  EXPECT_NE(childText.find("output logic sink_d"), std::string::npos);
  EXPECT_NE(childText.find("output logic sink_e"), std::string::npos);
  EXPECT_NE(childText.find("input logic root_commit_ok"), std::string::npos);
  EXPECT_NE(childText.find("output logic subtree_error"), std::string::npos);
  const std::regex childRegister(R"(always_ff\s*@\(posedge\s+clk\))");
  EXPECT_EQ(std::distance(std::sregex_iterator(childText.begin(),
                                               childText.end(), childRegister),
                          std::sregex_iterator()),
            1)
      << "Child's private state is the only child-owned storage; sink is a "
         "formal D/E output";
  EXPECT_NE(childText.find("local_error"), std::string::npos);

  auto moduleText = [&](llvm::StringRef name) {
    const std::string opening = "module " + name.str();
    const size_t begin = verilog->find(opening);
    if (begin == std::string::npos)
      return std::string{};
    const size_t end = verilog->find("endmodule", begin);
    if (end == std::string::npos)
      return std::string{};
    return verilog->substr(begin, end - begin);
  };
  const std::string rootText = moduleText(rootName);
  const std::string synthText = moduleText("FinalModel");
  ASSERT_FALSE(rootText.empty());
  ASSERT_FALSE(synthText.empty());
  const size_t rootFfCount = std::distance(
      std::sregex_iterator(rootText.begin(), rootText.end(), childRegister),
      std::sregex_iterator());
  EXPECT_EQ(rootFfCount, 6u)
      << "all six root-owned registers have storage in the root definition";
  EXPECT_EQ(std::distance(std::sregex_iterator(synthText.begin(),
                                               synthText.end(), childRegister),
                          std::sregex_iterator()),
            0)
      << "the synth top only composes definition families";

  const std::regex enableAssignment(
      R"((?:assign\s+)?q[0-9]+_e\s*=\s*([^;]+);)");
  size_t gatedEnableCount = 0;
  const std::regex ffEnableUse(R"(else\s+if\s*\(\s*(q[0-9]+_e)\s*\))");
  for (const std::string *family :
       std::array<const std::string *, 2>{&rootText, &childText}) {
    for (std::sregex_iterator
             it(family->begin(), family->end(), enableAssignment),
         end;
         it != end; ++it) {
      const std::string rhs = (*it)[1].str();
      EXPECT_EQ(rhs.find("root_commit_ok"), rhs.rfind("root_commit_ok"))
          << "each physical/formal enable contains one root commit gate";
      EXPECT_NE(rhs.find("root_commit_ok"), std::string::npos)
          << "each owner enable is gated by the system commit decision";
      ++gatedEnableCount;
    }
    for (std::sregex_iterator it(family->begin(), family->end(), ffEnableUse),
         end;
         it != end; ++it) {
      const std::string localEnable = (*it)[1].str();
      const std::regex localAssignment(localEnable + R"(\s*=\s*([^;]+);)");
      std::smatch assignment;
      ASSERT_TRUE(std::regex_search(*family, assignment, localAssignment));
      const std::string rhs = assignment[1].str();
      EXPECT_NE(rhs.find("root_commit_ok"), std::string::npos);
      EXPECT_EQ(rhs.find("root_commit_ok"), rhs.rfind("root_commit_ok"));
    }
  }
  EXPECT_EQ(gatedEnableCount, 7u)
      << "every owned physical register enable is gated once; formal next-port "
         "E is a proposal";
  const std::regex formalEnable(R"(sink_e\s*=\s*([^;]+);)");
  std::smatch formalEnableMatch;
  ASSERT_TRUE(std::regex_search(childText, formalEnableMatch, formalEnable));
  EXPECT_EQ(formalEnableMatch[1].str().find("root_commit_ok"),
            std::string::npos)
      << "child formal E remains the ungated local proposal until its physical "
         "owner";

  const std::regex childInstance(childName + R"(\s+child_[0-9]+\s*\()");
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               childInstance),
                          std::sregex_iterator()),
            2)
      << "both child placements instantiate the same source-derived family";
  const std::regex childSinkData(R"(\.sink_d\s*\()");
  const std::regex childSinkEnable(R"(\.sink_e\s*\()");
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               childSinkData),
                          std::sregex_iterator()),
            2);
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               childSinkEnable),
                          std::sregex_iterator()),
            2)
      << "child proposals connect directly to parent-owned D/E inputs";
  const std::regex childErrorDeclaration(
      R"(logic\s+child_[0-9]+_subtree_error\s*;)");
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               childErrorDeclaration),
                          std::sregex_iterator()),
            2)
      << "each child contributes one subtree-error signal to its parent";
  const std::regex downstreamCommit(R"(\.root_commit_ok\s*\(root_commit_ok\))");
  EXPECT_EQ(std::distance(std::sregex_iterator(rootText.begin(), rootText.end(),
                                               downstreamCommit),
                          std::sregex_iterator()),
            2);

  const std::string svPath = temporary.child("FinalModel.generated.sv");
  const std::string tbPath = temporary.child("tb.sv");
  const std::string simPath = temporary.child("sim.vvp");
  const std::string simLog = temporary.child("sim.log");
  writeFile(svPath, *verilog);
  writeFile(tbPath, R"sv(module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModelSim dut(.clk(clk), .reset(reset));

  task automatic tick;
    begin
      #1 clk = 1'b1;
      #2 clk = 1'b0;
      #1;
    end
  endtask

  initial begin
    tick();
    reset = 1'b0;
    tick();
    tick();
    tick();
    tick();
    reset = 1'b1;
    tick();
    reset = 1'b0;
    tick();
    tick();
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
                temporary.child("iverilog.log")),
            0)
      << readFile(temporary.child("iverilog.log"));
  ASSERT_EQ(run(vvp, {vvp, simPath}, simLog), 0) << readFile(simLog);

  auto verilator = llvm::sys::findProgramByName("verilator");
  ASSERT_TRUE(static_cast<bool>(verilator));
  ASSERT_EQ(run(*verilator,
                {*verilator, "--lint-only", "--language", "1800-2017",
                 "--top-module", "FinalModel", svPath},
                temporary.child("verilator.log")),
            0)
      << readFile(temporary.child("verilator.log"));
  ASSERT_EQ(run(*verilator,
                {*verilator, "--lint-only", "--language", "1800-2017",
                 "--top-module", "FinalModelSim", svPath},
                temporary.child("verilator-sim-wrapper.log")),
            0)
      << readFile(temporary.child("verilator-sim-wrapper.log"));

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
  ASSERT_EQ(records.size(), 30u)
      << "each run has three committed cycles; the descendant-failed fourth "
         "cycle emits nothing";
  ASSERT_EQ(ready->observations().bindings.size(), 5u);
  for (size_t index = 0; index < 30; ++index) {
    const size_t withinCycle = index % 5;
    const size_t cycle = (index / 5) % 3;
    EXPECT_EQ(records[index].ordinal, withinCycle);
    EXPECT_EQ(records[index].evaluationEpoch, cycle);
    EXPECT_EQ(records[index].commitEpoch, cycle + 1);
    EXPECT_LT(records[index].value, 2u);
  }
  for (size_t index = 0; index < 15; ++index)
    EXPECT_EQ(records[index + 15], records[index]);
  std::set<std::uint64_t> owners;
  for (size_t index = 0; index < 5; ++index) {
    owners.insert(records[index].owner);
  }
  EXPECT_EQ(owners.size(), 3u)
      << "root and the two repeated child instances have separate owner keys";
  std::array<unsigned, 2> firstCycleValues{};
  std::array<unsigned, 2> secondCycleValues{};
  std::array<unsigned, 2> thirdCycleValues{};
  for (size_t index = 0; index < 5; ++index) {
    ++firstCycleValues[records[index].value];
    ++secondCycleValues[records[index + 5].value];
    ++thirdCycleValues[records[index + 10].value];
  }
  std::string secondCycleTrace;
  for (size_t index = 5; index < 10; ++index) {
    if (!secondCycleTrace.empty())
      secondCycleTrace += ' ';
    secondCycleTrace += std::to_string(records[index].ordinal) + ":" +
                        std::to_string(records[index].value);
  }
  EXPECT_EQ(firstCycleValues[0], 4u);
  EXPECT_EQ(firstCycleValues[1], 1u)
      << "the first records report old Q before the first successful edge";
  EXPECT_EQ(secondCycleValues[1], 3u)
      << "the next cycle sees child private Q committed from the first cycle; "
         "observed "
      << secondCycleTrace;
  EXPECT_EQ(secondCycleValues[0], 2u);
  EXPECT_EQ(thirdCycleValues[1], 5u)
      << "cycle three sees parent sink Q committed from child D/E on cycle two";
}

TEST_F(MultiInstanceVerilogTest,
       ReversingSourceUnitInputOrderPreservesGeneratedRtl) {
  auto forward = build(false);
  auto reverse = build(true);
  ASSERT_TRUE(mlir::succeeded(forward));
  ASSERT_TRUE(mlir::succeeded(reverse));
  auto forwardRtl = emitFinalVerilog(*forward, emitError());
  auto reverseRtl = emitFinalVerilog(*reverse, emitError());
  ASSERT_TRUE(mlir::succeeded(forwardRtl));
  ASSERT_TRUE(mlir::succeeded(reverseRtl));
  EXPECT_EQ(*forwardRtl, *reverseRtl);
}

} // namespace
} // namespace acir::compiler
