#include "ComposedExecution.h"
#include "Compiler/FinalEmitCppHierarchy.h"
#include "ComposedObservations.h"
#include "GeneratedStorageInventory.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include <array>
#include <set>
#include <sstream>
#include <tuple>

using namespace mlir;
namespace {
bool write(llvm::StringRef path, llvm::StringRef text) {
  std::error_code error;
  llvm::raw_fd_ostream out(path, error);
  if (error)
    return false;
  out << text;
  out.close();
  return !out.has_error();
}
bool run(llvm::ArrayRef<std::string> owned, llvm::StringRef log,
         acir::ac::detail::EmitError error) {
  llvm::SmallVector<llvm::StringRef> args(owned.begin(), owned.end());
  std::array<std::optional<llvm::StringRef>, 3> redirects{std::nullopt, log,
                                                          log};
  if (llvm::sys::ExecuteAndWait(args.front(), args, std::nullopt, redirects) ==
      0)
    return true;
  auto output = llvm::MemoryBuffer::getFile(log);
  error() << "generated composed backend failed: "
          << (output ? (*output)->getBuffer().take_back(6000)
                     : llvm::StringRef("missing log"));
  return false;
}
FailureOr<unsigned> outgoingIndex(const acir::compiler::FinalProgram &program,
                                  acir::ac::detail::EmitError error) {
  const auto &root = program.instances()[program.rootInstanceOrdinal()];
  for (auto [local, ordinal] : llvm::enumerate(root.ownedStateOrdinals)) {
    auto state = program.proposals().states[ordinal].stateID;
    for (const auto &carrier : program.stateCarriers()) {
      auto reg = dyn_cast_or_null<acir::ac::RegOp>(carrier.declaration);
      if (carrier.stateID == state && carrier.view == root.view && reg &&
          reg.getName() == "outgoing")
        return unsigned(local);
    }
  }
  return error() << "source fixture has no owned outgoing state";
}

std::string cppDriver(const acir::compiler::FinalProgram &program,
                      unsigned outgoing, uint64_t limit, unsigned runs) {
  std::string text = R"cpp(#include <iostream>
#include <vector>
#include <tuple>
#include <span>
#include <algorithm>
#include <limits>
#include <string>
#include <cstdint>
#include "gfsim/SimSystem.h"
#include "gfsim/SimExecutor.h"
#define private public
#include "model.h"
#undef private
void observation(char tag, const gfsim::ObservationDescriptor &d,
                 uint64_t epoch, gfsim::SlotValue value) {
  std::cout << tag << " " << d.stableOrdinal << " " << epoch << " "
            << unsigned(value.kind) << " " << value.bits << " "
            << d.ownerKey << " " << d.registrationKey << " " << d.siteKey << "\n";
}
int main() {
  FinalSystem model;
  __REPORTS__
  gfsim::SimExecutor executor(model,model.Observations(),reports);
  const char config[] = __CONFIG__;
  if(!executor.created() || executor.ConfigureJson(reinterpret_cast<const uint8_t*>(config),sizeof(config)-1)!=AGENTIC_MODEL_STATUS_V1_OK) return 1;
  for(unsigned run=0;run<__RUNS__;++run){
    if(executor.Reset()!=AGENTIC_MODEL_STATUS_V1_OK) return 1;
    unsigned calls=0; uint64_t commits=0;
    AgenticModelStepResultV1 step{sizeof(AgenticModelStepResultV1),0,0,0,0};
    if(model.state()!=gfsim::SimSystemState::Ready) return 1;
    std::cout << "BEGIN " << run << "\n";
    while(model.cycle()<UINT64_C(__LIMIT__)) {
      std::cout << "T " << model.cycle() << " " << model.root_.q__OUT___ .Read() << "\n";
      ++calls;
      if(executor.Step(&step)!=AGENTIC_MODEL_STATUS_V1_OK ||
         (step.state!=AGENTIC_MODEL_STEP_V1_RUNNING && step.state!=AGENTIC_MODEL_STEP_V1_TERMINATED)) return 2;
      ++commits;
      for(const auto &e:model.Observations().Events()) observation('E',e.descriptor,e.epoch,e.value);
      for(const auto &g:model.Observations().Gauges()) {
        if(g.lastUpdate!=model.cycle()) continue;
        bool found=false;
        for(const auto &d:model.Observations().Descriptors()) {
          if(d.stableOrdinal!=g.stableOrdinal) continue;
          if(found) return 3;
          found=true; observation('G',d,g.lastUpdate,g.value);
        }
        if(!found) return 4;
      }
    }
    AgenticModelBufferV1 statistics{};
    if(executor.StatisticsJson(&statistics)!=AGENTIC_MODEL_STATUS_V1_OK) return 5;
    std::cout << "STAT "; std::cout.write(reinterpret_cast<const char*>(statistics.data),statistics.size); std::cout << "\n";
    std::cout << "END " << model.cycle() << " " << calls << " " << commits << " " << step.state << "\n";
  }
}
)cpp";
  for (const auto &[key, value] :
       std::array<std::pair<std::string, uint64_t>, 3>{
           {{"__RUNS__", runs}, {"__LIMIT__", limit}, {"__OUT__", outgoing}}})
    text.replace(text.find(key), key.size(), std::to_string(value));
  std::string reports;
  llvm::raw_string_ostream reportOut(reports);
  size_t count = 0;
  for (const auto &observation : program.observations().bindings)
    count += observation.kind.getValue() == "report";
  reportOut << "std::array<gfsim::ReportGaugeDescriptor," << count
            << "> reports{{";
  for (const auto &observation : program.observations().bindings) {
    if (observation.kind.getValue() != "report")
      continue;
    std::string owner;
    llvm::raw_string_ostream ownerOut(owner);
    ownerOut << observation.ownerRef;
    auto name = observation.spec.getAs<mlir::StringAttr>("name").getValue();
    reportOut << "{" << observation.stableOrdinal << ",std::string("
              << acir::compiler::cppStringLiteral(owner) << "," << owner.size()
              << "),std::string(" << acir::compiler::cppStringLiteral(name)
              << "," << name.size() << ")},";
  }
  reportOut << "}};";
  text.replace(text.find("__REPORTS__"), 11, reports);
  std::string config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":" +
      std::to_string(limit) +
      ",\"schema\":\"agentic-model-config\",\"version\":\"1\"}";
  text.replace(text.find("__CONFIG__"), 10,
               acir::compiler::cppStringLiteral(config));
  return text;
}

std::string rtlDriver(const acir::compiler::FinalProgram &program,
                      unsigned outgoing, uint64_t limit, unsigned runs) {
  const auto &root = program.instances()[program.rootInstanceOrdinal()];
  std::ostringstream out;
  out << "module tb; reg clk=0; reg reset=1; integer run; integer epoch; "
         "integer commits;\n"
         "FinalModel dut(.clk(clk),.reset(reset));\n";
  for (size_t local = 0; local < root.observationOrdinals.size(); ++local)
    out << "reg p" << local << "; reg [63:0] v" << local << ";\n";
  out << "initial begin\nfor(run=0;run<" << runs
      << ";run=run+1) begin\n"
         "commits=0; reset=1; #1; clk=1; #1; clk=0; reset=0; #1;\n"
         "$display(\"BEGIN %0d\",run);\n"
      << "for(epoch=0;epoch<" << limit << ";epoch=epoch+1) begin\n"
      << "$display(\"T %0d %0d\",epoch,dut.q" << outgoing
      << ");\n"
         "if(dut.global_permit !== 1'b1) $fatal(1,\"source check "
         "rejected\");\n";
  for (size_t local = 0; local < root.observationOrdinals.size(); ++local)
    out << "p" << local << "=dut.root_.obs_path_" << local << "; v" << local
        << "=dut.root_.obs_value_" << local << ";\n";
  out << "clk=1; #1; commits=commits+1; clk=0; #1;\n";
  for (auto [local, ordinal] : llvm::enumerate(root.observationOrdinals)) {
    const auto &observation = program.observations().bindings[ordinal];
    const char *tag = observation.kind.getValue() == "report" ? "G" : "E";
    unsigned registration = 0;
    auto rootModule = root.module;
    for (auto rule : rootModule.getBody().front().getOps<acir::ac::RuleOp>()) {
      if (rule == observation.rule)
        break;
      ++registration;
    }
    out << "if(p" << local << ") $display(\"" << tag << " "
        << observation.stableOrdinal << " %0d 2 %0d " << root.ordinal << " "
        << registration << " " << observation.requiredIndex << "\",epoch+1,v"
        << local << ");\n";
  }
  out << "end\n$display(\"END %0d "
         "%0d %0d 2\",epoch,epoch,commits);\nend\n$finish;\nend\nendmodule\n";
  return out.str();
}

FailureOr<llvm::json::Array>
measurements(const acir::compiler::FinalProgram &program, llvm::StringRef text,
             llvm::StringRef rtlTrailer, unsigned expectedRuns, uint64_t limit,
             acir::ac::detail::EmitError error) {
  llvm::SmallVector<llvm::StringRef> lines;
  text.split(lines, '\n', -1, false);
  llvm::json::Array result, trace, events, gauges;
  std::vector<std::tuple<uint64_t, uint64_t, llvm::json::Object>> observations;
  std::set<std::pair<uint64_t, uint64_t>> seen;
  bool active = false;
  llvm::json::Value statistics(nullptr);
  bool hasStatistics = false;
  uint64_t run = 0, nextEpoch = 0;
  for (auto line : lines) {
    std::istringstream input(line.str());
    std::string tag, extra;
    input >> tag;
    if (tag == "BEGIN") {
      if (active || !(input >> run) || run != result.size() || input >> extra)
        return error() << "invalid generated BEGIN record";
      active = true;
      nextEpoch = 0;
      seen.clear();
      observations.clear();
      statistics = nullptr;
      hasStatistics = false;
      trace = {};
      events = {};
      gauges = {};
    } else if (tag == "T") {
      uint64_t epoch, value;
      if (!active || !(input >> epoch >> value) || epoch != nextEpoch++ ||
          input >> extra)
        return error() << "invalid generated trace record";
      trace.push_back(value);
    } else if (tag == "E" || tag == "G") {
      uint64_t ordinal, epoch, kind, value, owner, registration, site;
      if (!active ||
          !(input >> ordinal >> epoch >> kind >> value >> owner >>
            registration >> site) ||
          epoch != nextEpoch || input >> extra ||
          !seen.emplace(epoch, ordinal).second)
        return error() << "invalid or duplicated generated observation record";
      auto canonical = composedObservation(program, tag, ordinal, epoch, kind,
                                           std::to_string(value), owner,
                                           registration, site, error);
      if (failed(canonical))
        return failure();
      observations.emplace_back(epoch, ordinal, std::move(*canonical));
      llvm::json::Object record{{"ordinal", ordinal},
                                {"epoch", epoch},
                                {"value_kind", kind},
                                {"value", std::to_string(value)}};
      (tag == "E" ? events : gauges).push_back(std::move(record));
    } else if (tag == "STAT") {
      auto parsed = llvm::json::parse(line.drop_front(5));
      if (!parsed) {
        llvm::consumeError(parsed.takeError());
        return error() << "invalid executor statistics JSON";
      }
      if (!active || hasStatistics || !parsed->getAsArray())
        return error() << "duplicated or misplaced executor statistics";
      statistics = std::move(*parsed);
      hasStatistics = true;
    } else if (tag == "END") {
      uint64_t epoch, calls, commits, state;
      if (!active || !(input >> epoch >> calls >> commits >> state) ||
          epoch != limit || calls != limit || commits != epoch || state != 2 ||
          nextEpoch != limit || input >> extra)
        return error() << "invalid generated terminal record";
      llvm::sort(observations, [](const auto &a, const auto &b) {
        return std::tie(std::get<0>(a), std::get<1>(a)) <
               std::tie(std::get<0>(b), std::get<1>(b));
      });
      llvm::json::Array canonical;
      for (auto &observation : observations)
        canonical.push_back(std::move(std::get<2>(observation)));
      if (!hasStatistics) {
        if (rtlTrailer.empty())
          return error() << "C++ executor did not report statistics";
        auto snapshot = composedRtlStatistics(program, commits, gauges, error);
        if (failed(snapshot))
          return failure();
        statistics = llvm::json::Value(std::move(*snapshot));
      }
      llvm::json::Object terminal{{"kind", "result"},
                                  {"status", "TERMINATED"},
                                  {"epoch_time", std::to_string(commits)},
                                  {"statistics", std::move(statistics)},
                                  {"error", nullptr}};
      result.push_back(
          llvm::json::Object{{"trace", std::move(trace)},
                             {"events", std::move(events)},
                             {"gauges", std::move(gauges)},
                             {"commit", commits},
                             {"step_calls", calls},
                             {"observations", std::move(canonical)},
                             {"terminal", std::move(terminal)}});
      active = false;
    } else if (!rtlTrailer.empty() && line.starts_with(rtlTrailer) &&
               line.contains("$finish called at") && !active &&
               result.size() == expectedRuns) {
      // Icarus's normal tool-owned termination trailer is separate from DUT
      // records.
    } else
      return error() << "unrecognized generated output: " << line;
  }
  if (active || result.size() != expectedRuns)
    return error() << "generated run inventory is incomplete";
  return std::move(result);
}

} // namespace

FailureOr<llvm::json::Object>
executeComposedFixture(const acir::compiler::FinalProgram &program,
                       llvm::StringRef backend, uint64_t maxTicks,
                       bool resetRerun, acir::ac::detail::EmitError error) {
  using namespace acir::compiler;
  if ((backend != "cpp" && backend != "verilog") || !maxTicks ||
      maxTicks > 1000 || failed(verifyFinalProgram(program, error)))
    return error() << "invalid bounded composed execution request";
  auto outgoing = outgoingIndex(program, error);
  if (failed(outgoing))
    return failure();
  const auto &root = program.instances()[program.rootInstanceOrdinal()];
  for (const auto &instance : program.instances())
    if (instance.ordinal != root.ordinal &&
        !instance.observationOrdinals.empty())
      return error() << "fixture measurement requires root-owned observations";
  llvm::SmallString<256> temporary;
  if (llvm::sys::fs::createUniqueDirectory("acir-composed-run", temporary))
    return error() << "cannot create backend execution workspace";
  auto cleanup =
      llvm::scope_exit([&] { llvm::sys::fs::remove_directories(temporary); });
  const std::string dir = temporary.str().str();
  const std::string log = dir + "/run.log";
  unsigned runs = resetRerun ? 2 : 1;
  uint64_t physicalCount = 0;
  if (backend == "cpp") {
    auto model = emitFinalCpp(program, error);
    if (failed(model))
      return failure();
    auto measured = generatedStorageInventory(*model, true, error);
    if (failed(measured))
      return failure();
    physicalCount = *measured;
    if (physicalCount != program.stateCarriers().size())
      return error() << "fixture C++ storage declarations disagree with "
                        "physical inventory";
    if (failed(model) || !write(dir + "/model.h", *model) ||
        !write(dir + "/main.cpp",
               cppDriver(program, *outgoing, maxTicks, runs)) ||
        !run({ACIR_BACKEND_CXX, "-std=c++20", "-I",
              std::string(ACIR_BACKEND_REPO_ROOT) + "/simulator/gfsim/include",
              dir + "/main.cpp",
              std::string(ACIR_BACKEND_REPO_ROOT) +
                  "/simulator/gfsim/sim_executor.cpp",
              std::string(ACIR_BACKEND_REPO_ROOT) +
                  "/simulator/gfsim/model_input.cpp",
              "-o", dir + "/dut"},
             dir + "/build.log", error) ||
        !run({dir + "/dut"}, log, error))
      return failure();
  } else {
    auto model = emitFinalVerilog(program, error);
    if (failed(model))
      return failure();
    auto measured = generatedStorageInventory(*model, false, error);
    if (failed(measured))
      return failure();
    physicalCount = *measured;
    if (physicalCount != program.stateCarriers().size())
      return error() << "fixture RTL storage declarations disagree with "
                        "physical inventory";
    if (failed(model) || !write(dir + "/model.sv", *model) ||
        !write(dir + "/tb.sv", rtlDriver(program, *outgoing, maxTicks, runs)) ||
        !run({ACIR_BACKEND_IVERILOG, "-g2012", "-s", "tb", "-o", dir + "/dut",
              dir + "/model.sv", dir + "/tb.sv"},
             dir + "/build.log", error) ||
        !run({ACIR_BACKEND_VVP, dir + "/dut"}, log, error))
      return failure();
  }
  auto output = llvm::MemoryBuffer::getFile(log);
  if (!output)
    return error() << "missing composed execution output";
  auto records =
      measurements(program, (*output)->getBuffer(),
                   backend == "verilog" ? llvm::StringRef(dir + "/tb.sv:")
                                        : llvm::StringRef(),
                   runs, maxTicks, error);
  if (failed(records))
    return failure();
  return llvm::json::Object{{"runs", std::move(*records)},
                            {"register_count", physicalCount}};
}

mlir::FailureOr<std::string>
executeComposedPair(const acir::compiler::FinalProgram &program,
                    llvm::StringRef identity, uint64_t maxTicks,
                    acir::ac::detail::EmitError error) {
  auto cpp = executeComposedFixture(program, "cpp", maxTicks, true, error);
  auto rtl = executeComposedFixture(program, "verilog", maxTicks, true, error);
  if (mlir::failed(cpp) || mlir::failed(rtl))
    return mlir::failure();
  auto trace = [&](llvm::json::Object &data) -> llvm::json::Array * {
    auto runs = data.getArray("runs");
    if (!runs || runs->size() != 2 || (*runs)[0] != (*runs)[1])
      return nullptr;
    auto first = (*runs)[0].getAsObject();
    return first ? first->getArray("trace") : nullptr;
  };
  if ((*cpp)["runs"] != (*rtl)["runs"] ||
      (*cpp)["register_count"] != (*rtl)["register_count"])
    return error() << "C++ and RTL composed measurements disagree";
  for (auto *backend : {&*cpp, &*rtl})
    for (const auto &run : *backend->getArray("runs"))
      if (!run.getAsObject() ||
          failed(verifyComposedCompletion(program, *run.getAsObject(), error)))
        return failure();
  auto cppTrace = trace(*cpp), rtlTrace = trace(*rtl);
  if (!cppTrace || !rtlTrace)
    return error() << "reset/rerun changed generated backend observations";
  llvm::json::Object result{
      {"input_identity",
       llvm::json::Object{{"cpp", identity}, {"verilog", identity}}},
      {"trace", llvm::json::Object{{"cpp", llvm::json::Array(*cppTrace)},
                                   {"verilog", llvm::json::Array(*rtlTrace)}}},
      {"register_count",
       llvm::json::Object{{"cpp", (*cpp)["register_count"]},
                          {"verilog", (*rtl)["register_count"]}}},
      {"measurements", llvm::json::Object{{"cpp", std::move(*cpp)},
                                          {"verilog", std::move(*rtl)}}}};
  std::string text;
  llvm::raw_string_ostream out(text);
  out << llvm::json::Value(std::move(result)) << '\n';
  return text;
}
