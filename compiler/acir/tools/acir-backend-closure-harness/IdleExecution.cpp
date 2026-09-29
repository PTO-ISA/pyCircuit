#include "IdleExecution.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include <array>

namespace {
struct Workspace {
  llvm::SmallString<256> path;
  Workspace() {
    if (llvm::sys::fs::createUniqueDirectory("acir-idle", path))
      path.clear();
  }
  ~Workspace() {
    if (!path.empty())
      llvm::sys::fs::remove_directories(path);
  }
  std::string file(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};
bool write(llvm::StringRef name, llvm::StringRef content) {
  std::error_code error;
  llvm::raw_fd_ostream out(name, error);
  if (error)
    return false;
  out << content;
  out.close();
  return !out.has_error();
}
bool run(llvm::ArrayRef<std::string> args, llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs(args.begin(), args.end());
  std::array<std::optional<llvm::StringRef>, 3> redirects{std::nullopt, log,
                                                          log};
  return llvm::sys::ExecuteAndWait(refs.front(), refs, std::nullopt,
                                   redirects) == 0;
}
} // namespace

mlir::FailureOr<std::string>
executeIdleFixture(const acir::compiler::FinalProgram &program,
                   llvm::StringRef backend, acir::ac::detail::EmitError error) {
  using namespace acir::compiler;
  if ((backend != "cpp" && backend != "verilog") ||
      mlir::failed(verifyFinalProgram(program, error)))
    return mlir::failure();
  for (const auto &instance : program.instances())
    if (!instance.ruleOrdinals.empty())
      return error() << "idle execution requires zero rules";
  if (program.stateCarriers().size() != 1 || program.instances().size() != 1)
    return error() << "idle fixture requires its one declared register";
  Workspace w;
  if (w.path.empty())
    return error() << "cannot create idle execution workspace";
  auto output = w.file("run.log");
  if (backend == "cpp") {
    auto model = emitFinalCpp(program, error);
    if (mlir::failed(model))
      return mlir::failure();
    auto driver = w.file("main.cpp"), binary = w.file("dut");
    if (!write(w.file("model.h"), *model) || !write(driver, R"cpp(
#include "model.h"
#include <iostream>
int main(){
  FinalSystem model; model.Build(); model.Reset();
  unsigned calls=0; ++calls; auto result=model.Step();
  if(result!=gfsim::SimStepResult::Quiescent ||
     model.state()!=gfsim::SimSystemState::Ready || model.cycle()!=0) return 1;
  std::cout<<"QUIESCENT READY "<<model.cycle()<<" "<<calls<<"\n";
  model.Reset();
  return model.Step()==gfsim::SimStepResult::Quiescent && model.cycle()==0 ? 0:2;
}
)cpp") ||
        !run({ACIR_BACKEND_CXX, "-std=c++20", "-I",
              std::string(ACIR_BACKEND_REPO_ROOT) + "/simulator/gfsim/include",
              driver, "-o", binary},
             w.file("compile.log")) ||
        !run({binary}, output))
      return error() << "generated idle C++ execution failed";
  } else {
    auto model = emitFinalVerilog(program, error);
    if (mlir::failed(model))
      return mlir::failure();
    auto driver = w.file("tb.sv"), binary = w.file("dut.vvp"),
         rtl = w.file("model.sv");
    // The wrapper's step attempts do not clock a zero-rule design. Only reset
    // transfers are driven; activity comes from the verified instance
    // inventory.
    if (!write(rtl, *model) || !write(driver, R"sv(
module tb;
reg clk=0; reg reset=1; integer epoch=0; integer calls=0;
FinalModel dut(.clk(clk),.reset(reset));
initial begin
 #1; clk=1; #1; clk=0; reset=0; #1;
 calls=calls+1;
 if(dut.q0 !== 1'b0 || dut.global_permit !== 1'b1) $fatal(1,"idle state");
 $display("QUIESCENT READY %0d %0d",epoch,calls);
 reset=1; #1; clk=1; #1; clk=0; reset=0; #1;
 if(dut.q0 !== 1'b0) $fatal(1,"idle reset");
 $finish;
end
endmodule
)sv") ||
        !run({ACIR_BACKEND_IVERILOG, "-g2012", "-s", "tb", "-o", binary, rtl,
              driver},
             w.file("compile.log")) ||
        !run({ACIR_BACKEND_VVP, binary}, output))
      return error() << "generated idle RTL execution failed";
  }
  auto log = llvm::MemoryBuffer::getFile(output);
  if (!log)
    return error() << "cannot read idle backend result";
  llvm::SmallVector<llvm::StringRef> lines;
  (*log)->getBuffer().split(lines, '\n', -1, false);
  llvm::StringRef record;
  for (auto line : lines)
    if (line.starts_with("QUIESCENT READY ")) {
      if (!record.empty())
        return error() << "duplicate idle backend result";
      record = line;
    }
  llvm::SmallVector<llvm::StringRef> parts;
  record.split(parts, ' ', -1, false);
  uint64_t epoch = 0, calls = 0;
  if (parts.size() != 4 || parts[2].getAsInteger(10, epoch) ||
      parts[3].getAsInteger(10, calls) || epoch != 0 || calls != 1)
    return error() << "invalid idle backend result";
  llvm::json::Object terminal{{"kind", "result"},
                              {"status", "QUIESCENT"},
                              {"epoch_time", std::to_string(epoch)},
                              {"statistics", llvm::json::Array{}},
                              {"error", nullptr}};
  llvm::json::Object result{
      {"state", "QUIESCENT"},
      {"epoch", epoch},
      {"step_calls", calls},
      {"model_state", "READY"},
      {"records", llvm::json::Array{std::move(terminal)}}};
  std::string text;
  llvm::raw_string_ostream out(text);
  out << llvm::json::Value(std::move(result)) << '\n';
  return text;
}
