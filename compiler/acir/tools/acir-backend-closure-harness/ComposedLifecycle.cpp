#include "ComposedLifecycle.h"
#include "ComposedExecution.h"
#include "ComposedFixture.h"
#include "ComposedObservations.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
namespace {
FailureOr<llvm::json::Object>
terminalRun(const acir::compiler::FinalProgram &program,
            llvm::json::Object &run, acir::ac::detail::EmitError error) {
  if (failed(verifyComposedCompletion(program, run, error)))
    return failure();
  auto terminal = run.getObject("terminal");
  auto observations = run.getArray("observations");
  auto trace = run.getArray("trace");
  if (!terminal || !observations || !trace || !run.getInteger("commit") ||
      !run.getInteger("step_calls"))
    return error() << "lifecycle measurement is incomplete";
  auto statistics = terminal->getArray("statistics");
  std::optional<int64_t> completed;
  if (statistics)
    for (const auto &value : *statistics) {
      auto row = value.getAsObject();
      if (row && row->getString("name") == llvm::StringRef("completed"))
        completed = row->getInteger("value");
    }
  if (!completed)
    return error() << "lifecycle completion gauge is missing";
  llvm::json::Array records(*observations);
  records.push_back(llvm::json::Object(*terminal));
  return llvm::json::Object{{"trace", llvm::json::Array(*trace)},
                            {"commit", *run.getInteger("commit")},
                            {"state", *terminal->getString("status")},
                            {"completed", *completed},
                            {"step_calls", *run.getInteger("step_calls")},
                            {"records", std::move(records)}};
}
} // namespace
FailureOr<std::string>
executeComposedLifecycle(MLIRContext &context, uint64_t maxTicks,
                         acir::ac::detail::EmitError error) {
  llvm::json::Object cpp, rtl;
  for (bool pipeline : {false, true}) {
    auto program = buildComposedFixture(context, pipeline, error);
    if (failed(program))
      return failure();
    const char *name = pipeline ? "two-level-system" : "single-module";
    llvm::json::Object pair;
    for (llvm::StringRef backend : {"cpp", "verilog"}) {
      auto result =
          executeComposedFixture(*program, backend, maxTicks, true, error);
      if (failed(result))
        return failure();
      auto runs = result->getArray("runs");
      if (!runs || runs->size() != 2 || (*runs)[0] != (*runs)[1])
        return error() << "lifecycle reset replay changed";
      auto first = terminalRun(*program, *(*runs)[0].getAsObject(), error);
      auto second = terminalRun(*program, *(*runs)[1].getAsObject(), error);
      if (failed(first) || failed(second))
        return failure();
      pair[backend] = llvm::json::Object{{"first_run", std::move(*first)},
                                         {"rerun", std::move(*second)}};
    }
    if (pair["cpp"] != pair["verilog"])
      return error() << "lifecycle backends disagree";
    cpp[name] = std::move(pair["cpp"]);
    rtl[name] = std::move(pair["verilog"]);
  }
  std::string text;
  llvm::raw_string_ostream out(text);
  out << llvm::json::Value(llvm::json::Object{{"cpp", std::move(cpp)},
                                              {"verilog", std::move(rtl)}})
      << '\n';
  return text;
}
