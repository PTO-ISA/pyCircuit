#include "Compiler/FinalProgram.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
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
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-zero-rule-final", path));
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

size_t countSubstring(llvm::StringRef text, llvm::StringRef needle) {
  size_t count = 0;
  for (size_t offset = 0;
       (offset = text.find(needle, offset)) != llvm::StringRef::npos;
       offset += needle.size())
    ++count;
  return count;
}

class ZeroRuleFinalProgramTest : public ::testing::Test {
protected:
  ZeroRuleFinalProgramTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    ASSERT_TRUE(compile("idle.py",
                        R"py(from pycircuit import system

@system
def Idle():
    state: bool = False
)py",
                        idleBody, idleHeader));
  }

  bool compile(llvm::StringRef relativePath, llvm::StringRef sourceText,
               mlir::OwningOpRef<mlir::ModuleOp> &body,
               mlir::OwningOpRef<mlir::ModuleOp> &header) {
    const std::string source = sourceRoot + "/" + relativePath.str();
    const std::string capture = temporary.child(relativePath.str() + ".mlir");
    const std::string bodyPath =
        temporary.child(relativePath.str() + ".body.mlir");
    const std::string headerPath =
        temporary.child(relativePath.str() + ".interface.mlir");
    writeFile(source, sourceText);
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
    ProcessResult captured =
        run(ACIR_TEST_PYTHON,
            {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT, source,
             sourceRoot, capture},
            temporary.child(relativePath.str() + ".capture.log"));
    if (captured.status != 0) {
      ADD_FAILURE() << captured.output;
      return false;
    }
    ProcessResult compiled =
        run(ACIR_TEST_SOURCE_UNIT_HARNESS,
            {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture, "--package",
             "verify", "--path", relativePath.str(), "--body-out", bodyPath,
             "--interface-out", headerPath},
            temporary.child(relativePath.str() + ".compile.log"));
    if (compiled.status != 0) {
      ADD_FAILURE() << compiled.output;
      return false;
    }
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    return body && header;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  mlir::FailureOr<FinalProgram> buildIdle() {
    SourceLinkUnit unit{*idleBody, *idleHeader};
    auto analysis =
        buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
  mlir::OwningOpRef<mlir::ModuleOp> idleBody, idleHeader;
};

TEST_F(ZeroRuleFinalProgramTest,
       PythonSystemExecutesAsQuiescentCppAndHoldingVerilog) {
  SourceLinkUnit unit{*idleBody, *idleHeader};
  auto analysis =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(analysis));
  EXPECT_EQ(analysis->state(), FinalProgramState::AnalysisClosed);
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*analysis, emitError())));

  auto ready = materializeFinalProgram(std::move(*analysis), emitError());
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  std::string finalText;
  {
    llvm::raw_string_ostream out(finalText);
    ASSERT_TRUE(ready->hardware());
    ready->hardware().print(out, mlir::OpPrintingFlags().enableDebugInfo());
  }
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto independent = mlir::parseSourceString<mlir::ModuleOp>(finalText, &fresh);
  ASSERT_TRUE(independent);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*independent)));
  for (auto *view : ready->modules().views)
    EXPECT_TRUE(mlir::succeeded(
        mlir::verify(view->module->getParentOfType<mlir::ModuleOp>())));
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  ASSERT_EQ(ready->instances().size(), 1u);
  EXPECT_TRUE(ready->instances().front().ruleOrdinals.empty());
  ASSERT_EQ(ready->stateCarriers().size(), 1u);

  auto cpp = emitFinalCpp(*ready, emitError());
  auto verilog = emitFinalVerilog(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(cpp));
  ASSERT_TRUE(mlir::succeeded(verilog));
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));

  const std::string cppModel = temporary.child("Idle.generated.h");
  const std::string cppDriver = temporary.child("idle_driver.cpp");
  const std::string cppBinary = temporary.child("idle_cpp");
  writeFile(cppModel, *cpp);
  writeFile(cppDriver, R"cpp(#include "Idle.generated.h"
#include "gfsim/SimSystem.h"

#include <iostream>

int main() {
  FinalSystem system;
  system.Build();
  system.Reset();
  if (system.state() != gfsim::SimSystemState::Ready || system.cycle() != 0)
    return 10;
  if (system.Step() != gfsim::SimStepResult::Quiescent ||
      system.state() != gfsim::SimSystemState::Ready || system.cycle() != 0)
    return 11;
  std::cout << "STEP Quiescent Ready 0\n";
  system.Reset();
  if (system.state() != gfsim::SimSystemState::Ready || system.cycle() != 0 ||
      system.Step() != gfsim::SimStepResult::Quiescent)
    return 12;
  std::cout << "RESET Quiescent Ready 0\n";
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
  EXPECT_EQ(cppRun.output, "STEP Quiescent Ready 0\n"
                           "RESET Quiescent Ready 0\n");

  EXPECT_EQ(countSubstring(*verilog, "always_ff @(posedge clk)"), 1u)
      << "one source reg must lower to exactly one RTL state element";
  EXPECT_EQ(countSubstring(*verilog, "logic q0;"), 1u);
  EXPECT_EQ(verilog->find("logic q1;"), std::string::npos);
  const std::string rtlModel = temporary.child("Idle.generated.sv");
  const std::string rtlDriver = temporary.child("idle_tb.sv");
  const std::string rtlBinary = temporary.child("idle.vvp");
  writeFile(rtlModel, *verilog);
  writeFile(rtlDriver, R"sv(module tb;
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
    $display("R %0d %0d", dut.q0, dut.global_permit);
    reset = 1'b0;
    tick();
    $display("A %0d %0d", dut.q0, dut.global_permit);
    tick();
    $display("B %0d %0d", dut.q0, dut.global_permit);
    reset = 1'b1;
    tick();
    $display("C %0d %0d", dut.q0, dut.global_permit);
  end
endmodule
)sv");
  ProcessResult rtlCompile = run(ACIR_TEST_IVERILOG,
                                 {ACIR_TEST_IVERILOG, "-g2012", "-s", "tb",
                                  "-o", rtlBinary, rtlModel, rtlDriver},
                                 temporary.child("rtl-build.log"));
  ASSERT_EQ(rtlCompile.status, 0) << rtlCompile.output;
  ProcessResult rtlRun = run(ACIR_TEST_VVP, {ACIR_TEST_VVP, rtlBinary},
                             temporary.child("rtl-run.log"));
  ASSERT_EQ(rtlRun.status, 0) << rtlRun.output;
  EXPECT_EQ(rtlRun.output, "R 0 1\nA 0 1\nB 0 1\nC 0 1\n");
}

TEST_F(ZeroRuleFinalProgramTest, RejectsMissingRootAndStrippedSourceMetadata) {
  EXPECT_TRUE(mlir::failed(
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>{}, emitError())));

  auto bodyClone = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(idleBody->clone()));
  auto headerClone = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(idleHeader->clone()));
  auto bodyDefinition = *bodyClone->getBody()->getOps<ac::ModuleOp>().begin();
  bodyDefinition->removeAttr("ac.ports");
  SourceLinkUnit missingBodyMetadata{*bodyClone, *headerClone};
  EXPECT_TRUE(mlir::failed(buildFinalProgram(
      llvm::ArrayRef<SourceLinkUnit>(missingBodyMetadata), emitError())));

  bodyClone = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(idleBody->clone()));
  headerClone = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(idleHeader->clone()));
  auto declaration =
      *headerClone->getBody()->getOps<ac::ModuleImportOp>().begin();
  declaration->removeAttr("ac.contract");
  SourceLinkUnit missingHeaderMetadata{*bodyClone, *headerClone};
  EXPECT_TRUE(mlir::failed(buildFinalProgram(
      llvm::ArrayRef<SourceLinkUnit>(missingHeaderMetadata), emitError())));
}

TEST_F(ZeroRuleFinalProgramTest,
       RejectsAnalysisClosureWithRuleButNoSourceCarriers) {
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
  ASSERT_TRUE(compile("empty_rule.py",
                      R"py(from pycircuit import rule, system

@system
def EmptyRule():
    state: bool = False

    @rule
    def tick():
        return

    tick()
)py",
                      body, header));
  auto definition = *body->getBody()->getOps<ac::ModuleOp>().begin();
  ASSERT_EQ(llvm::range_size(definition.getBody().front().getOps<ac::RuleOp>()),
            1u);
  EXPECT_TRUE(definition.getBody().front().getOps<ac::SourceReadOp>().empty());
  EXPECT_TRUE(definition.getBody().front().getOps<ac::SourceUseOp>().empty());
  SourceLinkUnit unit{*body, *header};
  EXPECT_TRUE(mlir::failed(
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError())));
}

TEST_F(ZeroRuleFinalProgramTest,
       EmitReadyGraphMutationIsRejectedByBothEmitters) {
  for (llvm::StringRef backend : {"cpp", "verilog"}) {
    SCOPED_TRACE(backend.str());
    auto ready = buildIdle();
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
    auto &modules = const_cast<ModuleGraph &>(ready->modules());
    modules.root = nullptr;
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
    if (backend == "cpp")
      EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())));
    else
      EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
  }
}

TEST_F(ZeroRuleFinalProgramTest, NativeResetMustFitLogicalRange) {
  mlir::OwningOpRef<mlir::ModuleOp> source, header;
  ASSERT_TRUE(compile("range_idle.py", R"py(from typing import Annotated
from pycircuit import system
Word = Annotated[int, range(10)]
@system
def RangeIdle():
    state: Word = 0
)py",
                      source, header));
  SourceLinkUnit unit{*source, *header};
  auto analysis =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(analysis));
  auto ready = materializeFinalProgram(std::move(*analysis), emitError());
  ASSERT_TRUE(mlir::succeeded(ready));
  std::string text;
  llvm::raw_string_ostream out(text);
  ASSERT_TRUE(ready->hardware());
  ready->hardware().print(out, mlir::OpPrintingFlags().enableDebugInfo());
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
  ASSERT_TRUE(reparsed);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*reparsed)));
  mlir::Builder b(&fresh);
  reparsed->walk([&](ac::RegOp reg) {
    reg->setAttr("ac.initial_value", b.getIntegerAttr(b.getIntegerType(4), 15));
  });
  EXPECT_TRUE(mlir::failed(mlir::verify(*reparsed)));
}

} // namespace
} // namespace acir::compiler
