#include "BackendPublication.h"
#include "Compiler/FinalProgram.h"
#include "Compiler/SourceUnit.h"
#include "ComposedExecution.h"
#include "ComposedFixture.h"
#include "ComposedLifecycle.h"
#include "IdleExecution.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

using acir::testing::Publication;
using acir::testing::publishAtomically;

namespace {

constexpr llvm::StringLiteral kApi = "pycircuit-w10-backend-closure-v1";

llvm::cl::opt<bool> api("api", llvm::cl::init(false));
llvm::cl::opt<bool> apiJson("api-json", llvm::cl::init(false));
llvm::cl::opt<std::string> fixture("fixture", llvm::cl::init(""));
llvm::cl::opt<std::string> backend("backend", llvm::cl::init(""));
llvm::cl::opt<std::string> mutation("mutation", llvm::cl::init(""));
llvm::cl::opt<std::string> cppOutput("cpp-out", llvm::cl::init(""));
llvm::cl::opt<std::string> verilogOutput("verilog-out", llvm::cl::init(""));
llvm::cl::opt<std::string> manifestOutput("manifest-out", llvm::cl::init(""));
llvm::cl::opt<std::string> resultOutput("result-json", llvm::cl::init(""));
llvm::cl::opt<uint64_t> maxTicks("max-ticks", llvm::cl::init(5));
llvm::cl::opt<bool> resetRerun("reset-rerun", llvm::cl::init(false));

class TemporaryDirectory {
public:
  TemporaryDirectory() {
    valid_ = !llvm::sys::fs::createUniqueDirectory("acir-backend", path_);
  }
  ~TemporaryDirectory() {
    if (valid_)
      llvm::sys::fs::remove_directories(path_);
  }

  bool valid() const { return valid_; }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path_);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }

private:
  llvm::SmallString<256> path_;
  bool valid_ = false;
};

bool writeText(llvm::StringRef path, llvm::StringRef text) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  if (error)
    return false;
  output << text;
  return true;
}

bool run(llvm::StringRef program, llvm::ArrayRef<std::string> owned,
         llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt,
                                   redirects) == 0;
}

bool captureSource(llvm::StringRef source, llvm::StringRef sourceRoot,
                   llvm::StringRef output, llvm::StringRef log) {
  static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
  std::array<std::string, 7> owned = {
      ACIR_BACKEND_PYTHON, "-c",
      script.str(),        ACIR_BACKEND_REPO_ROOT,
      source.str(),        sourceRoot.str(),
      output.str()};
  llvm::SmallVector<llvm::StringRef> arguments;
  arguments.reserve(owned.size());
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(ACIR_BACKEND_PYTHON, arguments, std::nullopt,
                                   redirects) == 0;
}

struct FixtureProgram {
  acir::compiler::SourceUnitArtifacts source;
  acir::compiler::FinalProgram program;
};

mlir::FailureOr<FixtureProgram>
buildMinimalFixture(mlir::MLIRContext &context,
                    acir::ac::detail::EmitError emitError, bool idle = false) {
  TemporaryDirectory temporary;
  if (!temporary.valid())
    return emitError() << "cannot create backend fixture directory";
  const std::string sourceRoot = temporary.child("src");
  if (llvm::sys::fs::create_directories(sourceRoot))
    return emitError() << "cannot create backend fixture source root";
  const std::string sourcePath = sourceRoot + "/main.py";
  const std::string capturePath = temporary.child("capture.mlir");
  llvm::StringRef sourceText = R"py(from pycircuit import module, rule, log
@module
def Main():
    source: bool = True
    sink: bool = False
    state: bool = False
    @rule
    def transfer():
        nonlocal state, sink
        assert source, "source is set"
        log("info", "source", source)
        state = source
        sink = state
        return
    transfer()
)py";
  if (idle)
    sourceText = "from pycircuit import system\n@system\ndef Idle():\n"
                 "    state: bool = False\n";
  if (!writeText(sourcePath, sourceText) ||
      !captureSource(sourcePath, sourceRoot, capturePath,
                     temporary.child("capture.log")))
    return emitError() << "cannot capture minimal-final fixture";

  auto transport = mlir::parseSourceFile<mlir::ModuleOp>(capturePath, &context);
  if (!transport)
    return emitError() << "cannot parse minimal-final source transport";
  auto registry = acir::compiler::SourceHeaderRegistry::create({}, emitError);
  if (mlir::failed(registry))
    return mlir::failure();
  mlir::Builder builder(&context);
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("w10")),
      builder.getNamedAttr("path", builder.getStringAttr("main.py")),
  });
  auto source = acir::compiler::compilePythonSourceUnit(*transport, owner,
                                                        *registry, emitError);
  if (mlir::failed(source))
    return mlir::failure();
  acir::compiler::SourceLinkUnit unit{*source->body, *source->interface};
  auto analysis = acir::compiler::buildFinalProgram(
      llvm::ArrayRef<acir::compiler::SourceLinkUnit>(unit), emitError);
  if (mlir::failed(analysis))
    return mlir::failure();
  auto program =
      acir::compiler::materializeFinalProgram(std::move(*analysis), emitError);
  if (mlir::failed(program))
    return mlir::failure();
  return FixtureProgram{std::move(*source), std::move(*program)};
}

bool executeGenerated(llvm::StringRef cpp, llvm::StringRef verilog,
                      TemporaryDirectory &temporary) {
  const std::string header = temporary.child("FinalModel.generated.h");
  const std::string cppDriver = temporary.child("driver.cpp");
  const std::string cppBinary = temporary.child("cpp-dut");
  const std::string rtl = temporary.child("FinalModel.generated.sv");
  const std::string rtlDriver = temporary.child("driver.sv");
  const std::string rtlBinary = temporary.child("rtl-dut.vvp");
  if (!writeText(header, cpp) || !writeText(rtl, verilog) ||
      !writeText(cppDriver, R"cpp(#include "FinalModel.generated.h"
int main() { FinalSystem s; s.Build(); s.Reset();
  if (s.Step() != gfsim::SimStepResult::Running) return 1;
  s.Reset(); return s.Step() == gfsim::SimStepResult::Running ? 0 : 2; }
)cpp") ||
      !writeText(rtlDriver, R"sv(module tb;
logic clk = 0; logic reset = 1; FinalModel dut(.clk(clk), .reset(reset));
initial begin #1 clk=1; #1 clk=0; reset=0; #1 clk=1; #1; $finish; end
endmodule
)sv"))
    return false;
  const std::string include =
      std::string(ACIR_BACKEND_REPO_ROOT) + "/simulator/gfsim/include";
  return run(ACIR_BACKEND_CXX,
             {ACIR_BACKEND_CXX, "-std=c++20", "-I", include, cppDriver, "-o",
              cppBinary},
             temporary.child("cpp-build.log")) &&
         run(cppBinary, {cppBinary}, temporary.child("cpp-run.log")) &&
         run(ACIR_BACKEND_IVERILOG,
             {ACIR_BACKEND_IVERILOG, "-g2012", "-s", "tb", "-o", rtlBinary, rtl,
              rtlDriver},
             temporary.child("rtl-build.log")) &&
         run(ACIR_BACKEND_VVP, {ACIR_BACKEND_VVP, rtlBinary},
             temporary.child("rtl-run.log"));
}

mlir::Operation *cloneSourceCarrier(mlir::ModuleOp source,
                                    llvm::StringRef mutation) {
  mlir::Operation *result = nullptr;
  source->walk([&](mlir::Operation *operation) {
    if (result)
      return;
    bool matches = (mutation == "residual-source-read" &&
                    mlir::isa<acir::ac::SourceReadOp>(operation)) ||
                   (mutation == "residual-source-use" &&
                    mlir::isa<acir::ac::SourceUseOp>(operation)) ||
                   (mutation == "residual-source-observe" &&
                    mlir::isa<acir::ac::SourceObserveOp>(operation));
    if (matches)
      result = operation->clone();
  });
  return result;
}

mlir::LogicalResult applyMutation(FixtureProgram &fixtureProgram,
                                  llvm::StringRef name,
                                  acir::ac::detail::EmitError emitError) {
  using namespace acir::compiler;
  FinalProgram &program = fixtureProgram.program;
  auto &modules = const_cast<ModuleGraph &>(program.modules());
  auto &proposals = const_cast<ProposalGraph &>(program.proposals());
  auto &checks = const_cast<CheckGraph &>(program.checks());

  if (name.starts_with("residual-source-")) {
    mlir::Operation *carrier =
        cloneSourceCarrier(*fixtureProgram.source.body, name);
    if (!carrier || modules.views.empty())
      return emitError() << "minimal-final lacks requested source carrier";
    acir::ac::RuleOp rule;
    modules.views.front()->module->walk([&](acir::ac::RuleOp candidate) {
      if (!rule)
        rule = candidate;
    });
    if (!rule) {
      carrier->destroy();
      return emitError() << "minimal-final lacks a rule for source mutation";
    }
    mlir::Block &body = rule.getBody().front();
    body.getOperations().insert(body.getTerminator()->getIterator(), carrier);
    return mlir::success();
  }
  if (name == "forged-final-stage") {
    modules.views.front()->module->setAttr(
        "ac.stage", mlir::StringAttr::get(
                        program.modules().root->module.getContext(), "final"));
    return mlir::success();
  }
  if (name == "forged-proof") {
    acir::ac::RuleOp rule;
    modules.views.front()->module->walk([&](acir::ac::RuleOp candidate) {
      if (!rule)
        rule = candidate;
    });
    if (!rule)
      return emitError() << "minimal-final lacks a rule for proof mutation";
    mlir::OpBuilder builder(rule.getBody().front().getTerminator());
    mlir::OperationState state(builder.getUnknownLoc(), "ac.numeric.proof");
    builder.create(state);
    return mlir::success();
  }
  if (name == "same-type-target-swap") {
    if (proposals.states.size() < 2)
      return emitError() << "minimal-final lacks two state targets";
    proposals.states[0].commit.stateID = proposals.states[1].stateID;
    return mlir::success();
  }
  if (name == "permit-disconnected") {
    proposals.globalPermitAlways = !proposals.globalPermitAlways;
    return mlir::success();
  }
  if (name == "permit-inverted") {
    if (proposals.states.empty())
      return emitError() << "minimal-final lacks a commit permit";
    proposals.states.front().commit.permitAlways =
        !proposals.states.front().commit.permitAlways;
    return mlir::success();
  }
  if (name == "commit-enable-bypass") {
    for (StateProposals &state : proposals.states) {
      if (state.commit.kind != CommitPairKind::Forward)
        continue;
      state.commit.enable = state.commit.data;
      return mlir::success();
    }
    return emitError() << "minimal-final lacks a Forward commit";
  }
  if (name == "error-from-gated-enable") {
    if (checks.bindings.empty())
      return emitError() << "minimal-final lacks an error check";
    for (StateProposals &state : proposals.states) {
      if (state.commit.kind != CommitPairKind::Forward)
        continue;
      checks.bindings.front().condition = state.commit.enable;
      return mlir::success();
    }
    return emitError() << "minimal-final lacks a gated enable";
  }
  return emitError() << "unknown backend closure mutation '" << name << "'";
}

std::string inputIdentity(const acir::compiler::FinalProgram &program) {
  std::string canonical;
  llvm::raw_string_ostream output(canonical);
  llvm::DenseSet<mlir::Operation *> definitions;
  for (const acir::compiler::InstanceView *view : program.modules().views) {
    if (view && view->module && definitions.insert(view->module).second) {
      acir::ac::ModuleOp module = view->module;
      module.print(output);
    }
  }
  for (const acir::compiler::StateProposals &state : program.proposals().states)
    output << state.stateID;
  for (const acir::compiler::CheckBinding &check : program.checks().bindings)
    output << check.checkID;
  for (const acir::compiler::ObservationBinding &observation :
       program.observations().bindings)
    output << observation.observationID;
  output.flush();

  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (unsigned char byte : canonical) {
    hash ^= byte;
    hash *= UINT64_C(1099511628211);
  }
  llvm::SmallString<16> hex;
  llvm::raw_svector_ostream hexOutput(hex);
  hexOutput.write_hex(hash);
  return hex.str().str();
}

} // namespace

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "verified FinalProgram backend harness\n");
  if (api) {
    llvm::outs() << kApi << '\n';
    return 0;
  }
  if (apiJson) {
    llvm::outs() << "{\"api\":\"" << kApi
                 << "\",\"input\":\"verified-final-graph\","
                    "\"backends\":[\"cpp\",\"verilog\"],"
                    "\"publication\":\"atomic\"}\n";
    return 0;
  }

  if (fixture == "zero-rule-system" || fixture == "two-systems") {
    bool lifecycle = fixture == "two-systems";
    if (resultOutput.empty() ||
        (lifecycle ? backend != "both"
                   : (backend != "cpp" && backend != "verilog")) ||
        !mutation.empty() || (lifecycle ? !resetRerun : resetRerun))
      return 2;
    mlir::DialectRegistry dialects;
    dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    mlir::MLIRContext context(dialects);
    context.loadAllAvailableDialects();
    auto error = [&] {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    if (lifecycle) {
      auto result = executeComposedLifecycle(context, maxTicks, error);
      if (mlir::failed(result))
        return 1;
      llvm::SmallVector<Publication> outputs{
          {resultOutput, std::move(*result), {}, {}, false}};
      return publishAtomically(outputs) ? 0 : 1;
    }
    auto idle = buildMinimalFixture(context, error, true);
    if (mlir::failed(idle))
      return 1;
    auto result = executeIdleFixture(idle->program, backend, error);
    if (mlir::failed(result))
      return 1;
    llvm::SmallVector<Publication> publications{
        {resultOutput, std::move(*result), {}, {}, false}};
    return publishAtomically(publications) ? 0 : 1;
  }

  if ((fixture == "single-module" || fixture == "two-level-system") &&
      backend == "both" &&
      ((!cppOutput.empty() && !verilogOutput.empty()) ||
       !resultOutput.empty()) &&
      !resetRerun && mutation.empty()) {
    mlir::DialectRegistry dialects;
    dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    mlir::MLIRContext context(dialects);
    context.loadAllAvailableDialects();
    auto error = [&] {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    auto program =
        buildComposedFixture(context, fixture == "two-level-system", error);
    if (mlir::failed(program))
      return 1;
    if (!resultOutput.empty()) {
      auto result = executeComposedPair(*program, inputIdentity(*program),
                                        maxTicks, error);
      if (mlir::failed(result))
        return 1;
      llvm::SmallVector<Publication> publications{
          {resultOutput, std::move(*result), {}, {}, false}};
      return publishAtomically(publications) ? 0 : 1;
    }
    auto cpp = acir::compiler::emitFinalCpp(*program, error);
    auto rtl = acir::compiler::emitFinalVerilog(*program, error);
    if (mlir::failed(cpp) || mlir::failed(rtl))
      return 1;
    llvm::SmallVector<Publication> publications{
        {cppOutput, std::move(*cpp), {}, {}, false},
        {verilogOutput, std::move(*rtl), {}, {}, false}};
    return publishAtomically(publications) ? 0 : 1;
  }

  constexpr std::array<llvm::StringLiteral, 6> resultFixtures = {
      "single-module",    "two-level-system",      "two-systems",
      "zero-rule-system", "schedule-permutations", "source-reorder"};
  if (llvm::is_contained(resultFixtures, fixture)) {
    llvm::errs() << "actual " << fixture << " backend execution is unavailable";
    if (fixture == "zero-rule-system")
      llvm::errs() << ": FinalProgram rejects units without residual source "
                      "semantics";
    else if (fixture == "two-systems")
      llvm::errs() << ": runner termination and four backend/system runs are "
                      "not implemented";
    llvm::errs() << "; no result was published\n";
    return 1;
  }
  if (fixture != "minimal-final" || backend != "both" || cppOutput.empty() ||
      verilogOutput.empty() || !resultOutput.empty() || resetRerun) {
    llvm::errs() << "minimal-final requires --backend both, --cpp-out and "
                    "--verilog-out\n";
    return 2;
  }

  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };

  auto fixtureProgram = buildMinimalFixture(context, emitError);
  if (mlir::failed(fixtureProgram))
    return 1;
  if (!mutation.empty() &&
      mlir::failed(applyMutation(*fixtureProgram, mutation, emitError)))
    return 1;
  if (mlir::failed(acir::compiler::verifyFinalProgram(fixtureProgram->program,
                                                      emitError))) {
    llvm::errs() << "final verification rejected\n";
    return 1;
  }

  auto cpp = acir::compiler::emitFinalCpp(fixtureProgram->program, emitError);
  auto verilog =
      acir::compiler::emitFinalVerilog(fixtureProgram->program, emitError);
  if (mlir::failed(cpp) || mlir::failed(verilog))
    return 1;
  TemporaryDirectory execution;
  if (!execution.valid() || !executeGenerated(*cpp, *verilog, execution)) {
    llvm::errs() << "generated backend execution failed\n";
    return 1;
  }
  const std::string identity = inputIdentity(fixtureProgram->program);
  std::string manifest =
      "{\n  \"input_identity\": {\"cpp\": \"" + identity +
      "\", \"verilog\": \"" + identity +
      "\"},\n  \"cpp\": {\"bytes\": " + std::to_string(cpp->size()) +
      "},\n  \"verilog\": {\"bytes\": " + std::to_string(verilog->size()) +
      "}\n}\n";
  llvm::SmallVector<Publication> publications = {
      {cppOutput, std::move(*cpp), {}, {}, false},
      {verilogOutput, std::move(*verilog), {}, {}, false},
  };
  if (!manifestOutput.empty())
    publications.push_back(
        {manifestOutput, std::move(manifest), {}, {}, false});
  if (!publishAtomically(publications)) {
    for (const Publication &publication : publications)
      if (!publication.temporary.empty())
        llvm::sys::fs::remove(publication.temporary);
    llvm::errs() << "atomic backend publication failed\n";
    return 1;
  }
  return 0;
}
