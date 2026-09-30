#include "Compiler/FinalProgram.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
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
        llvm::sys::fs::createUniqueDirectory("numeric-next-backend", path));
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

size_t count(llvm::StringRef text, llvm::StringRef needle) {
  size_t result = 0;
  for (size_t offset = 0;
       (offset = text.find(needle, offset)) != llvm::StringRef::npos;
       offset += needle.size())
    ++result;
  return result;
}

mlir::DictionaryAttr replaceField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name,
                                  mlir::Attribute replacement) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == name) {
      field = builder.getNamedAttr(name, replacement);
      return builder.getDictionaryAttr(fields);
    }
  ADD_FAILURE() << "missing dictionary field " << name.str();
  return dictionary;
}

class NumericNextBackendTest : public ::testing::Test {
protected:
  NumericNextBackendTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    const std::string source = sourceRoot + "/counter.py";
    writeFile(source, R"py(from typing import Annotated
from pycircuit import rule, system

Word = Annotated[int, range(256)]

@system
def Counter():
    state: Word = 254

    @rule
    def advance():
        nonlocal state
        state = (state + 1) & 255
        return

    advance()
)py");
    const std::string capture = temporary.child("counter.transport.mlir");
    bodyPath = temporary.child("counter.body.mlir");
    headerPath = temporary.child("counter.interface.mlir");
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
            temporary.child("capture.log"));
    ASSERT_EQ(captured.status, 0) << captured.output;
    ProcessResult compiled =
        run(ACIR_TEST_SOURCE_UNIT_HARNESS,
            {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture, "--package",
             "numeric_backend", "--path", "counter.py", "--body-out", bodyPath,
             "--interface-out", headerPath, "--lower-numeric"},
            temporary.child("compile.log"));
    ASSERT_EQ(compiled.status, 0) << compiled.output;
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    ASSERT_TRUE(body && header);
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  mlir::FailureOr<FinalProgram> build(mlir::ModuleOp candidate) {
    SourceLinkUnit unit{candidate, *header};
    auto analysis =
        buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  mlir::OwningOpRef<mlir::ModuleOp> cloneBody() {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(body->clone()));
  }

  bool graphAdmits(mlir::ModuleOp candidate) {
    SourceLinkUnit unit{candidate, *header};
    auto registry = SourceHeaderRegistry::create({*header}, emitError());
    if (mlir::failed(registry))
      return false;
    auto modules = buildSourceModuleGraph({unit}, *registry, emitError());
    if (mlir::failed(modules))
      return false;
    auto checks = buildSourceCheckGraph(*modules, emitError());
    auto observations = buildSourceObservationGraph(*modules, emitError());
    if (mlir::failed(checks) || mlir::failed(observations))
      return false;
    auto proposals = buildSourceProposalGraph(*modules, &*checks, emitError());
    return mlir::succeeded(proposals) &&
           mlir::succeeded(verifySourceChecks(*checks, emitError())) &&
           mlir::succeeded(
               verifySourceObservations(*observations, emitError())) &&
           mlir::succeeded(verifySourceProposals(*proposals, emitError()));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, bodyPath, headerPath;
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
};

TEST_F(NumericNextBackendTest, GraphsAcceptOnlyTheExactValueUsePacket) {
  ASSERT_TRUE(graphAdmits(*body));
  auto ready = build(*body);
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_EQ(ready->proposals().contributions.size(), 1u);
  const ProposalContribution &contribution =
      ready->proposals().contributions.front();
  ac::ValueUseOp valueUse = contribution.valueUse;
  EXPECT_FALSE(contribution.use);
  EXPECT_TRUE(valueUse);
  EXPECT_EQ(contribution.value, valueUse.getValue());
  EXPECT_EQ(contribution.valid, valueUse.getValid());
  EXPECT_EQ(contribution.path, valueUse.getPath());
  EXPECT_EQ(ready->checks().bindings.size(), 1u);
  EXPECT_TRUE(ready->observations().bindings.empty());

  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto candidate = cloneBody();
    mutate(*candidate);
    EXPECT_FALSE(graphAdmits(*candidate));
  };
  expectRejected("ValueUse actual SSA", [&](mlir::ModuleOp candidate) {
    ac::ValueUseOp use;
    ac::ValueBindingOp input;
    candidate.walk([&](ac::ValueUseOp op) { use = op; });
    candidate.walk([&](ac::ValueBindingOp binding) {
      if (!input)
        input = binding;
    });
    ASSERT_TRUE(use && input);
    use.getValueMutable().assign(input.getValue());
  });
  expectRejected("RequiredUse target", [&](mlir::ModuleOp candidate) {
    ac::RuleOp rule;
    candidate.walk([&](ac::RuleOp op) { rule = op; });
    auto uses = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
    auto item = mlir::cast<mlir::DictionaryAttr>(uses[0]);
    auto target = item.getAs<mlir::DictionaryAttr>("target");
    mlir::Builder builder(&context);
    auto state = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("formal")),
        builder.getNamedAttr("parameter", builder.getStringAttr("forged")),
        builder.getNamedAttr("ordinal", builder.getUnitAttr()),
    });
    target = replaceField(builder, target, "state", state);
    rule->setAttr(
        "ac.required_uses",
        builder.getArrayAttr({replaceField(builder, item, "target", target)}));
  });
  expectRejected("required check", [&](mlir::ModuleOp candidate) {
    candidate.walk(
        [&](ac::RuleOp rule) { rule->removeAttr("ac.required_checks"); });
  });
  expectRejected("forged observation", [&](mlir::ModuleOp candidate) {
    mlir::Builder builder(&context);
    candidate.walk([&](ac::RuleOp rule) {
      rule->setAttr("ac.required_observations",
                    builder.getArrayAttr({builder.getDictionaryAttr({})}));
    });
  });
}

TEST_F(NumericNextBackendTest, SameProgramExecutesCppAndRtlModuloCounter) {
  auto ready = build(*body);
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  for (auto *view : ready->modules().views)
    EXPECT_TRUE(mlir::succeeded(
        mlir::verify(view->module->getParentOfType<mlir::ModuleOp>())));
  auto cpp = emitFinalCpp(*ready, emitError());
  auto rtl = emitFinalVerilog(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(cpp));
  ASSERT_TRUE(mlir::succeeded(rtl));
  ASSERT_EQ(ready->stateCarriers().size(), 1u);
  EXPECT_EQ(ready->stateCarriers().front().width, 8u);
  EXPECT_EQ(count(*cpp, "::gfsim::SimDFFE<::std::uint64_t> q0_"), 1u);
  EXPECT_EQ(cpp->find(" q1_"), std::string::npos);
  EXPECT_NE(cpp->find("& UINT64_C(255)"), std::string::npos);
  EXPECT_EQ(count(*rtl, "logic [7:0] q0;"), 1u);
  EXPECT_EQ(rtl->find(" q1;"), std::string::npos);
  EXPECT_EQ(count(*rtl, "always_ff @(posedge clk)"), 1u);

  const std::string cppModel = temporary.child("Counter.generated.h");
  const std::string cppDriver = temporary.child("counter_driver.cpp");
  const std::string cppBinary = temporary.child("counter_cpp");
  writeFile(cppModel, *cpp);
  writeFile(cppDriver, R"cpp(#include "gfsim/ObservationSlot.h"
#include "gfsim/SimDFF.h"
#include "gfsim/SimSystem.h"
#include <array>
#include <bit>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#define private public
#define protected public
#include "Counter.generated.h"
#undef protected
#undef private
#include <iostream>

static unsigned q(FinalSystem &system) { return system.root_.q0_.Read(); }
int main() {
  FinalSystem system;
  system.Build();
  system.Reset();
  std::cout << "R " << q(system) << '\n';
  system.root_.Work(1);
  std::cout << "W " << q(system) << '\n';
  system.root_.DiscardNext();
  system.Reset();
  for (char tag : {'A', 'B', 'C'}) {
    if (system.Step() != gfsim::SimStepResult::Running) return 10;
    std::cout << tag << ' ' << q(system) << '\n';
  }
  system.Reset();
  std::cout << "D " << q(system) << '\n';
  for (char tag : {'E', 'F', 'G'}) {
    if (system.Step() != gfsim::SimStepResult::Running) return 11;
    std::cout << tag << ' ' << q(system) << '\n';
  }
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
  EXPECT_EQ(cppRun.output, "R 254\nW 254\nA 255\nB 0\nC 1\n"
                           "D 254\nE 255\nF 0\nG 1\n");

  const std::string rtlModel = temporary.child("Counter.generated.sv");
  const std::string rtlDriver = temporary.child("counter_tb.sv");
  const std::string rtlBinary = temporary.child("counter.vvp");
  writeFile(rtlModel, *rtl);
  writeFile(rtlDriver, R"sv(module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModel dut(.clk(clk), .reset(reset));
  task automatic tick;
    begin #1 clk = 1'b1; #1 clk = 1'b0; #1; end
  endtask
  initial begin
    tick(); $display("R %0d", dut.q0);
    reset = 1'b0;
    tick(); $display("A %0d", dut.q0);
    tick(); $display("B %0d", dut.q0);
    tick(); $display("C %0d", dut.q0);
    reset = 1'b1;
    tick(); $display("D %0d", dut.q0);
    reset = 1'b0;
    tick(); $display("E %0d", dut.q0);
    tick(); $display("F %0d", dut.q0);
    tick(); $display("G %0d", dut.q0);
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
  EXPECT_EQ(rtlRun.output, "R 254\nA 255\nB 0\nC 1\nD 254\nE 255\nF 0\nG 1\n");
}

TEST_F(NumericNextBackendTest, FrozenMutationsRejectBothEmitters) {
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto ready = build(*body);
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_TRUE(mutate(*ready));
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
  };
  for (bool binding : {false, true})
    expectRejected("unknown retained attribute", [&](FinalProgram &program) {
      mlir::Operation *evidence = nullptr;
      ac::ModuleOp module = program.instances().front().module;
      module.walk([&](mlir::Operation *op) {
        if ((binding && mlir::isa<ac::ValueBindingOp>(op)) ||
            (!binding && mlir::isa<ac::ValueUseOp>(op)))
          evidence = op;
      });
      if (!evidence)
        return false;
      evidence->setAttr("unknown", mlir::UnitAttr::get(&context));
      return true;
    });
  expectRejected("unused extra arithmetic", [&](FinalProgram &program) {
    ac::RuleOp rule;
    ac::ModuleOp module = program.instances().front().module;
    module.walk([&](ac::RuleOp op) { rule = op; });
    if (!rule)
      return false;
    mlir::OpBuilder b(rule.getBody().front().getTerminator());
    auto input = rule.getBody().front().getArgument(0);
    mlir::arith::OrIOp::create(b, rule.getLoc(), input, input);
    return true;
  });
  expectRejected("arithmetic", [&](FinalProgram &program) {
    mlir::arith::AddIOp add;
    ac::ModuleOp module = program.instances().front().module;
    module.walk([&](mlir::arith::AddIOp op) { add = op; });
    if (!add)
      return false;
    add.setOverflowFlags(mlir::arith::IntegerOverflowFlags::nsw);
    return true;
  });
  expectRejected("proof", [&](FinalProgram &program) {
    ac::NumericProofOp proof;
    ac::ModuleOp module = program.instances().front().module;
    module.walk([&](ac::NumericProofOp op) { proof = op; });
    if (!proof)
      return false;
    proof->removeAttr("origin");
    return true;
  });
  expectRejected("yield", [&](FinalProgram &program) {
    ac::RuleOp rule;
    ac::ModuleOp module = program.instances().front().module;
    module.walk([&](ac::RuleOp op) { rule = op; });
    if (!rule)
      return false;
    auto yield =
        mlir::cast<ac::YieldOp>(rule.getBody().front().getTerminator());
    if (rule.getBody().front().getNumArguments() != 1)
      return false;
    yield->setOperand(0, rule.getBody().front().getArgument(0));
    return true;
  });
  expectRejected("state", [&](FinalProgram &program) {
    if (program.stateCarriers().empty())
      return false;
    mlir::Operation *declaration = program.stateCarriers().front().declaration;
    if (!declaration)
      return false;
    declaration->setAttr(
        "ac.initial_value",
        mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 8), 0));
    return true;
  });
}

TEST_F(NumericNextBackendTest, NativeFinalSurvivesIndependentReparse) {
  std::string printed;
  {
    auto ready = build(*body);
    ASSERT_TRUE(mlir::succeeded(ready));
    auto unit = ready->hardware();
    ASSERT_TRUE(unit);
    llvm::raw_string_ostream stream(printed);
    unit.print(stream, mlir::OpPrintingFlags().enableDebugInfo());
  }
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(printed, &fresh);
  ASSERT_TRUE(reparsed);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*reparsed)));
  for (llvm::StringRef change :
       {"owner", "domain", "reset", "input", "proof", "check", "stage",
        "input-id-slot", "input-id-origin"}) {
    SCOPED_TRACE(change.str());
    auto copy = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>((*reparsed)->clone()));
    mlir::Builder b(&fresh);
    if (change == "stage")
      (*copy)->setAttr("ac.stage", b.getStringAttr("source"));
    if (change.starts_with("input-id-")) {
      ac::RuleOp rule;
      copy->walk([&](ac::RuleOp op) { rule = op; });
      auto values = rule.getBody().front().getOps<ac::ValueBindingOp>();
      ac::ValueBindingOp input = *values.begin();
      auto replacement = input.getIdAttr();
      if (change == "input-id-slot")
        replacement =
            replaceField(b, replacement, "slot", b.getI32IntegerAttr(9));
      else {
        auto other = *std::next(values.begin(), 2);
        replacement = replaceField(b, replacement, "origin",
                                   other.getIdAttr().get("origin"));
      }
      input->setAttr("id", replacement);
      for (auto proof : rule.getBody().front().getOps<ac::NumericProofOp>())
        if (proof.getMode() == "low_bits")
          proof->setAttr("input_ids", b.getArrayAttr({replacement}));
    }
    copy->walk([&](mlir::Operation *op) {
      if (change == "owner" && mlir::isa<ac::ModuleOp>(op))
        op->removeAttr("ac.source_owner");
      if (change == "domain" && mlir::isa<ac::RegOp>(op))
        op->removeAttr("ac.logical_type");
      if (change == "reset" && mlir::isa<ac::RegOp>(op))
        op->setAttr("ac.initial_value", b.getI16IntegerAttr(999));
      if (change == "input" && mlir::isa<ac::RuleOp>(op))
        op->setAttr("ac.input_bindings", b.getArrayAttr({}));
      if (change == "proof" && mlir::isa<ac::NumericProofOp>(op))
        op->removeAttr("origin");
      if (change == "check" && mlir::isa<ac::SourceExpectOp>(op))
        op->removeAttr("ac.check_id");
    });
    EXPECT_TRUE(mlir::failed(mlir::verify(*copy)));
  }
}

} // namespace
} // namespace acir::compiler
