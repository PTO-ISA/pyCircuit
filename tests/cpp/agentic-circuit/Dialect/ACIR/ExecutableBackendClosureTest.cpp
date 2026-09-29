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

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;

  TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "acir-executable-backend-closure", path));
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

struct ProcessResult {
  int status = -1;
  std::string output;
};

ProcessResult run(llvm::StringRef program,
                  const std::vector<std::string> &arguments,
                  llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs;
  for (const std::string &argument : arguments)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return {llvm::sys::ExecuteAndWait(program, refs, std::nullopt, redirects),
          readFile(log)};
}

TEST(ExecutableBackendClosureTest,
     SameEmitReadyProgramExecutesCppAndVerilogCycleOracle) {
  TemporaryDirectory temporary;
  mlir::DialectRegistry dialects;
  dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto emitError = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };

  const std::string sourceRoot = temporary.child("source");
  ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  const std::string source = sourceRoot + "/main.py";
  writeFile(source, R"py(from typing import Annotated
from pycircuit import module, rule, report

@module
def Main():
    condition: bool = True
    next_condition: bool = False
    value: Annotated[int, range(256)] = 3
    next_value: Annotated[int, range(256)] = 7

    @rule
    def step():
        nonlocal condition, value
        assert condition, "condition must permit the cycle"
        report("value", value)
        condition = next_condition
        value = next_value

    step()
)py");

  const std::string capture = temporary.child("main.transport.mlir");
  const std::string bodyPath = temporary.child("main.body.mlir");
  const std::string headerPath = temporary.child("main.interface.mlir");
  static constexpr llvm::StringLiteral captureScript = R"py(
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
  ProcessResult captured =
      run(ACIR_TEST_PYTHON,
          {ACIR_TEST_PYTHON, "-c", captureScript.str(), ACIR_TEST_REPO_ROOT,
           source, sourceRoot, capture},
          temporary.child("capture.log"));
  ASSERT_EQ(captured.status, 0) << captured.output;
  ProcessResult compiled =
      run(ACIR_TEST_SOURCE_UNIT_HARNESS,
          {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture, "--package",
           "verify", "--path", "main.py", "--body-out", bodyPath,
           "--interface-out", headerPath},
          temporary.child("source-compile.log"));
  ASSERT_EQ(compiled.status, 0) << compiled.output;

  auto body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
  auto header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
  ASSERT_TRUE(body && header);
  SourceLinkUnit unit{*body, *header};
  auto analysis =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError);
  ASSERT_TRUE(mlir::succeeded(analysis));
  auto ready = materializeFinalProgram(std::move(*analysis), emitError);
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError)));

  auto cpp = emitFinalCpp(*ready, emitError);
  auto verilog = emitFinalVerilog(*ready, emitError);
  ASSERT_TRUE(mlir::succeeded(cpp));
  ASSERT_TRUE(mlir::succeeded(verilog));
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError)));
  EXPECT_EQ(cpp->find("q4_"), std::string::npos);
  EXPECT_NE(verilog->find("// observation 0 kind=report"), std::string::npos);

  const std::string cppModel = temporary.child("FinalModel.generated.h");
  const std::string cppDriver = temporary.child("cpp_driver.cpp");
  const std::string cppBinary = temporary.child("cpp_dut");
  writeFile(cppModel, *cpp);
  writeFile(cppDriver, R"cpp(#include "FinalModel.generated.h"
#include "gfsim/SimSystem.h"

#include <iostream>

template <typename T>
concept HasPublicCheck = requires(T &module) { module.Check(); };
template <typename T>
concept HasPublicDrive = requires(T &module) { module.Drive(true); };
static_assert(!HasPublicCheck<FinalModel>);
static_assert(!HasPublicDrive<FinalModel>);

static void print_state(char tag, FinalSystem &system) {
  const auto gauges = system.Observations().Gauges();
  const long long gauge_value =
      gauges.empty() ? -1 : static_cast<long long>(gauges.front().value.bits);
  const long long gauge_update =
      gauges.empty() ? -1 : static_cast<long long>(gauges.front().lastUpdate);
  std::cout << tag << ' ' << system.cycle() << ' ' << gauge_value << ' '
            << gauge_update << '\n';
}

int main() {
  FinalSystem system;
  system.Build();
  system.Reset();
  print_state('R', system);
  const auto first = system.Step();
  if (first != gfsim::SimStepResult::Running ||
      system.state() != gfsim::SimSystemState::Ready)
    return 12;
  std::cout << "STEP1 Running Ready\n";
  print_state('A', system);
  const auto second = system.Step();
  if (second != gfsim::SimStepResult::Failed ||
      system.state() != gfsim::SimSystemState::Failed)
    return 13;
  std::cout << "STEP2 Failed Failed\n";
  print_state('B', system);
  system.Reset();
  print_state('C', system);
  return 0;
}
)cpp");
  ProcessResult cppCompile =
      run(ACIR_TEST_CXX,
          {ACIR_TEST_CXX, "-std=c++20", "-I",
           std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
           cppDriver, "-o", cppBinary},
          temporary.child("cpp-build.log"));
  ASSERT_EQ(cppCompile.status, 0) << cppCompile.output;
  ProcessResult cppRun =
      run(cppBinary, {cppBinary}, temporary.child("cpp-run.log"));
  ASSERT_EQ(cppRun.status, 0) << cppRun.output;
  EXPECT_EQ(cppRun.output, "R 0 0 0\n"
                           "STEP1 Running Ready\n"
                           "A 1 3 1\n"
                           "STEP2 Failed Failed\n"
                           "B 1 3 1\n"
                           "C 0 0 0\n");

  const std::string verilogModel = temporary.child("FinalModel.generated.sv");
  const std::string verilogDriver = temporary.child("verilog_driver.sv");
  const std::string verilogBinary = temporary.child("verilog_dut.vvp");
  writeFile(verilogModel, *verilog);
  writeFile(verilogDriver, R"sv(module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModel dut(.clk(clk), .reset(reset));

  task automatic tick;
    begin
      #1 clk = 1'b1;
      #1 clk = 1'b0;
      #1;
    end
  endtask

  initial begin
    tick();
    $display("R %0d %0d %0d %0d %0d", dut.q0, dut.q1, dut.q2, dut.q3,
             dut.global_permit);
    reset = 1'b0;
    tick();
    $display("A %0d %0d %0d %0d %0d", dut.q0, dut.q1, dut.q2, dut.q3,
             dut.global_permit);
    tick();
    $display("B %0d %0d %0d %0d %0d", dut.q0, dut.q1, dut.q2, dut.q3,
             dut.global_permit);
    reset = 1'b1;
    tick();
    $display("C %0d %0d %0d %0d %0d", dut.q0, dut.q1, dut.q2, dut.q3,
             dut.global_permit);
  end
endmodule
)sv");
  ProcessResult verilogCompile =
      run(ACIR_TEST_IVERILOG,
          {ACIR_TEST_IVERILOG, "-g2012", "-s", "tb", "-o", verilogBinary,
           verilogModel, verilogDriver},
          temporary.child("verilog-build.log"));
  ASSERT_EQ(verilogCompile.status, 0) << verilogCompile.output;
  ProcessResult verilogRun = run(ACIR_TEST_VVP, {ACIR_TEST_VVP, verilogBinary},
                                 temporary.child("verilog-run.log"));
  ASSERT_EQ(verilogRun.status, 0) << verilogRun.output;
  EXPECT_EQ(verilogRun.output, "R 1 0 3 7 1\n"
                               "A 0 0 7 7 0\n"
                               "B 0 0 7 7 0\n"
                               "C 1 0 3 7 1\n");
}

} // namespace
} // namespace acir::compiler
