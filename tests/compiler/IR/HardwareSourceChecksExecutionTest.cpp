// Existing GTest framework owns this native-development execution gate.
// Prepare real linked inputs, emit in-process, then verify the retained DUTs
// with independent oracles. Public checked emission and RTL remain separate;
// the local observation case verifies common native IR semantics directly.
#include "Compiler/HardwareEmitCommon.h"
#include "Compiler/HardwareEmitCppChecks.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mlir;
namespace ac = acir::ac;
namespace {
llvm::json::Value json(Attribute value) {
  if (auto dictionary = dyn_cast<DictionaryAttr>(value)) {
    llvm::json::Object result;
    for (auto entry : dictionary)
      result[entry.getName().strref()] = json(entry.getValue());
    return result;
  }
  if (auto array = dyn_cast<ArrayAttr>(value)) {
    llvm::json::Array result;
    for (auto entry : array)
      result.push_back(json(entry));
    return result;
  }
  if (auto boolean = dyn_cast<BoolAttr>(value))
    return boolean.getValue();
  if (auto integer = dyn_cast<ac::MathIntAttr>(value))
    return integer.getCanonicalValue();
  if (auto integer = dyn_cast<IntegerAttr>(value)) {
    if (integer.getValue().isNegative() ||
        integer.getValue().getActiveBits() > 64)
      throw std::runtime_error("test metadata integer is not logical u64");
    return integer.getValue().getZExtValue();
  }
  if (auto string = dyn_cast<StringAttr>(value))
    return string.getValue();
  if (auto symbol = dyn_cast<FlatSymbolRefAttr>(value))
    return symbol.getValue();
  throw std::runtime_error("unexpected test metadata attribute");
}
void write(const std::filesystem::path &root, const std::string &name,
           const std::string &text) {
  auto relative = std::filesystem::path(name);
  if (relative.empty() || relative.is_absolute())
    throw std::runtime_error("invalid generated relative path");
  for (const auto &component : relative)
    if (component == "..")
      throw std::runtime_error("generated path escapes output directory");
  auto path = root / relative;
  std::filesystem::create_directories(path.parent_path());
  std::ofstream stream(path, std::ios::binary);
  stream.exceptions(std::ios::badbit | std::ios::failbit);
  stream << text;
}
bool emitNative(const std::string &caseName, const std::string &final,
                const std::filesystem::path &output) {
  try {
    MLIRContext context;
    context.loadDialect<ac::ACIRDialect>();
    // Owning preparation performs budget-first validation; do not run local
    // association verification ahead of that admission boundary at parse time.
    auto package = parseSourceFile<ModuleOp>(
        final, ParserConfig(&context, /*verifyAfterParse=*/false));
    if (!package)
      return false;
    ac::HardwareAnalysis analysis(*package);
    acir::compiler::HardwareEmitContext emitter(*package, analysis);
    if (failed(emitter.prepareNativeChecks()))
      return false;
    const auto &plan = emitter.sourceChecks();
    auto parts = acir::compiler::emitPreparedCppSourceParts(emitter);
    if (failed(parts))
      return false;
    llvm::json::Array files, checks, owners, commits;
    for (const auto &owner : plan.owners) {
      llvm::json::Array path;
      for (const auto &step : owner.path) {
        llvm::json::Array coordinates;
        for (uint64_t coordinate : step.coordinates)
          coordinates.push_back(coordinate);
        path.push_back(llvm::json::Object{
            {"name", step.allocation->getAttrOfType<StringAttr>("instance_name")
                         .getValue()},
            {"occurrence", json(step.allocation->getAttr("occurrence"))},
            {"coordinates", std::move(coordinates)}});
      }
      owners.push_back(llvm::json::Object{
          {"definition",
           owner.definition->getAttrOfType<StringAttr>("sym_name").getValue()},
          {"ordinal", owner.ordinal},
          {"path", std::move(path)}});
    }
    for (const auto &check : plan.checks) {
      const auto &binding = plan.bindings[check.bindingIndex];
      llvm::json::Object reset{
          {"physical",
           check.reset.kind == ac::HardwareCheckResetKind::PhysicalReset}};
      if (check.reset.ownerOccurrence)
        reset["owner"] = *check.reset.ownerOccurrence;
      if (check.reset.inputOrdinal)
        reset["input"] = *check.reset.inputOrdinal;
      checks.push_back(llvm::json::Object{{"owner", check.ownerOccurrence},
                                          {"ordinal", check.ordinal},
                                          {"id", json(binding.checkID)},
                                          {"location", json(binding.location)},
                                          {"kind", binding.kind.getValue()},
                                          {"reset", std::move(reset)}});
    }
    for (const auto &endpoint : plan.commits)
      commits.push_back(
          llvm::json::Object{{"owner", endpoint.ownerOccurrence},
                             {"kind", endpoint.primitiveKind.getValue()}});
    files.push_back("pycircuit_support.hpp");
    files.push_back("pycircuit_system.hpp");
    for (const auto &group : parts->sourceGroups) {
      files.push_back(group.headerPath);
      if (!group.sourcePath.empty())
        files.push_back(group.sourcePath);
    }
    llvm::json::Object manifest{
        {"scope",
         "test-only prepared native emitter; no public or RTL acceptance"},
        {"root_cpp_name", parts->rootCppName},
        {"files", std::move(files)},
        {"owners", std::move(owners)},
        {"checks", std::move(checks)},
        {"commits", std::move(commits)}};
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
      throw std::runtime_error("test output directory must be absent or empty");
    write(output, "pycircuit_support.hpp", parts->supportHeader);
    write(output, "pycircuit_system.hpp", parts->systemHeader);
    for (const auto &group : parts->sourceGroups) {
      write(output, group.headerPath, group.header);
      if (!group.sourcePath.empty())
        write(output, group.sourcePath, group.source);
    }
    write(
        output, "native-checks.json",
        llvm::formatv("{0:2}\n", llvm::json::Value(std::move(manifest))).str());
    write(output, "native-emission.json",
          llvm::formatv("{0:2}\n", llvm::json::Value(llvm::json::Object{
                                       {"case", caseName},
                                       {"route", "in-process GTest"},
                                       {"status", "success"}}))
              .str());
  } catch (const std::exception &error) {
    llvm::errs() << "native check GTest emission: " << error.what() << '\n';
    return false;
  }
  return true;
}

bool runPython(llvm::StringRef phase, const std::string &evidence) {
  auto script = (std::filesystem::path(PYCIRCUIT_TEST_REPO_ROOT) /
                 "tests/compiler/lit/Source/Inputs/source-check-execution.py")
                    .string();
  llvm::SmallVector<std::string> command{PYCIRCUIT_TEST_PYTHON,
                                         script,
                                         "--phase",
                                         phase.str(),
                                         "--repo",
                                         PYCIRCUIT_TEST_REPO_ROOT,
                                         "--source-compiler",
                                         PYCIRCUIT_TEST_SOURCE_COMPILER,
                                         "--linker",
                                         PYCIRCUIT_TEST_LINKER,
                                         "--cxx",
                                         PYCIRCUIT_TEST_CXX,
                                         "--scratch",
                                         evidence};
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const auto &argument : command)
    arguments.push_back(argument);
  auto out = evidence + "/" + phase.str() + ".stdout";
  auto err = evidence + "/" + phase.str() + ".stderr";
  llvm::SmallVector<std::optional<llvm::StringRef>> redirects{
      std::nullopt, llvm::StringRef(out), llvm::StringRef(err)};
  std::string error;
  int status = llvm::sys::ExecuteAndWait(
      command.front(), arguments, std::nullopt, redirects, 900, 0, &error);
  if (status != 0) {
    ADD_FAILURE() << phase.str() << " failed, exit " << status << ": " << error
                  << "; evidence " << evidence;
    if (auto log = llvm::MemoryBuffer::getFile(err))
      llvm::errs() << (*log)->getBuffer();
  }
  return status == 0;
}

std::string noisyObservationFinal() {
  return R"mlir(
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Noisy, ast_path = []}, expansion = []}, location = {path = "noisy.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#owner = {package = "", path = "noisy.py"}
#span = {path = "noisy.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#registration = {site = {definition = @Noisy, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}
#log_site = {site = {definition = @Noisy, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#report_site = {site = {definition = @Noisy, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
module attributes {ac.stage = "final", ac.source_units = [{package = "", path = "noisy.py"}]} {
  "ac.module"() ({
  ^bb0(%ok: !b1):
    "ac.observe"(%ok, %ok) {kind = "log", spec = {event = "noisy", items = [{kind = "value", ordinal = 0 : i32}], level = "info"}, ac.location = #span, ac.observation_id = {registration = #registration, site = #log_site, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}} : (!b1, !b1) -> ()
    "ac.observe"(%ok, %ok) {kind = "report", spec = {name = "noisy_gauge"}, ac.location = #span, ac.observation_id = {registration = #registration, site = #report_site, ordinal = 1 : i64, invocation = [], owner = {instance_path = []}}} : (!b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Noisy", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1) -> (), input_names = ["ok"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @Noisy, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)mlir";
}

bool runNoisyObservationNative(const std::filesystem::path &output) {
  auto exposePrivateLines = [](const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    std::string original((std::istreambuf_iterator<char>(input)), {});
    std::string exposed;
    for (std::size_t begin = 0; begin < original.size();) {
      auto end = original.find('\n', begin);
      auto length =
          end == std::string::npos ? original.size() - begin : end - begin + 1;
      auto line = original.substr(begin, length);
      auto first = line.find_first_not_of(" \t");
      auto last = line.find_last_not_of(" \t\r\n");
      if (first != std::string::npos && last != std::string::npos &&
          line.substr(first, last - first + 1) == "private:")
        line.replace(first, std::string("private:").size(), "public:");
      exposed += line;
      if (end == std::string::npos)
        break;
      begin = end + 1;
    }
    std::ofstream written(path, std::ios::binary | std::ios::trunc);
    written << exposed;
  };
  auto includeRoot = output / "include/gfsim";
  std::filesystem::create_directories(includeRoot);
  auto sourceHeader = std::filesystem::path(PYCIRCUIT_TEST_REPO_ROOT) /
                      "include/gfsim/ObservationSlot.h";
  std::ifstream input(sourceHeader, std::ios::binary);
  std::string header((std::istreambuf_iterator<char>(input)), {});
  std::string exposed;
  for (std::size_t offset = 0;;) {
    auto found = header.find("private:", offset);
    if (found == std::string::npos) {
      exposed.append(header, offset, std::string::npos);
      break;
    }
    exposed.append(header, offset, found - offset);
    exposed += "public: ";
    offset = found + std::string("private:").size();
  }
  write(output / "include", "gfsim/ObservationSlot.h", exposed);
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(output))
    if (entry.path().extension() == ".hpp")
      exposePrivateLines(entry.path());
  const std::string driver = R"cpp(
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>

void require(bool condition, unsigned testCase, const char *message) {
  if (!condition) {
    std::cerr << "noisy observation case " << testCase << ": " << message << '\n';
    std::abort();
  }
}
auto sample(bool valueBit, bool knownBit, bool zBit) {
  gfsim::Bits<1> value{valueBit ? 1u : 0u}, known{knownBit ? 1u : 0u},
      zMask{zBit ? 1u : 0u};
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(
      gfsim::FourState<1>::fromMasks(value, known, zMask));
}
int main() {
  unsigned testCase = 0;
  struct Pattern { bool value, known, z, active; };
  for (auto pattern : std::array<Pattern, 6>{{
           {false, true, false, false}, {true, true, false, true},
           {false, false, false, false}, {true, false, false, false},
           {false, false, true, false}, {true, false, true, false}}}) {
    pyc_dut dut(1);
    pyc_dut::Inputs inputs;
    inputs.ok = sample(pattern.value, pattern.known, pattern.z);
    dut.drive(inputs);
    dut.system().Build();
    dut.system().Reset();
    dut.root_->Work();
    auto &observations = dut.observations();
    require(observations.slots_.size() == 2, testCase, "slot count");
    for (const auto &slot : observations.slots_)
      require(slot.pending && slot.path == pattern.active, testCase,
              "pending canonical path");
    dut.root_->DiscardNext();
    observations.DiscardNext();
    dut.drive(inputs);
    require(dut.system().Step() == gfsim::SimStepResult::Running, testCase,
            "successful step");
    require(observations.Gauges().size() == 1, testCase, "gauge count");
    if (pattern.active) {
      require(observations.Events().size() == 1, testCase, "active event count");
      require(observations.Events()[0].value == gfsim::SlotValue::Bool(true),
              testCase, "active bool event");
      require(observations.Gauges()[0].value == gfsim::SlotValue::Unsigned(1),
              testCase, "active unsigned gauge");
      require(observations.Gauges()[0].lastUpdate == 1, testCase,
              "active gauge epoch");
    } else {
      require(observations.Events().empty(), testCase, "inactive events");
      require(observations.Gauges()[0].value == gfsim::SlotValue::Unsigned(0),
              testCase, "inactive gauge value");
      require(observations.Gauges()[0].lastUpdate == 0, testCase,
              "inactive gauge epoch");
    }
    ++testCase;
  }
}
)cpp";
  write(output, "driver.cpp", driver);

  std::filesystem::path runtimeRoot =
      std::filesystem::path(PYCIRCUIT_TEST_SOURCE_COMPILER)
          .parent_path()
          .parent_path();
  auto runtime = runtimeRoot / "runtime/libpyc6_runtime.a";
  if (!std::filesystem::is_regular_file(runtime))
    runtime = runtimeRoot / "lib/libpyc6_runtime.a";
  if (!std::filesystem::is_regular_file(runtime))
    return false;
  std::vector<std::string> command{
      PYCIRCUIT_TEST_CXX,
      "-std=c++20",
      "-pthread",
      "-I" + (output / "include").string(),
      "-I" + (std::filesystem::path(PYCIRCUIT_TEST_REPO_ROOT) / "include")
                 .string(),
      "-I" + output.string(),
      (output / "driver.cpp").string()};
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(output))
    if (entry.path().extension() == ".cpp" &&
        entry.path().filename() != "driver.cpp")
      command.push_back(entry.path().string());
  auto executable = output / "noisy-observation";
  command.push_back(runtime.string());
  command.push_back("-o");
  command.push_back(executable.string());
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const auto &argument : command)
    arguments.push_back(argument);
  std::string error;
  if (llvm::sys::ExecuteAndWait(command.front(), arguments, std::nullopt, {},
                                120, 0, &error) != 0) {
    llvm::errs() << "noisy observation compile failed: " << error << '\n';
    return false;
  }
  std::string executableText = executable.string();
  llvm::SmallVector<llvm::StringRef> run{executableText};
  return llvm::sys::ExecuteAndWait(run.front(), run, std::nullopt, {}, 60, 0,
                                   &error) == 0;
}
} // namespace

TEST(HardwareSourceChecksExecution, NativeLifecycle) {
  llvm::SmallString<256> directory;
  auto prefix = (std::filesystem::path(PYCIRCUIT_TEST_BINARY_DIR) /
                 "source-check-execution")
                    .string();
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(prefix, directory));
  std::string evidence = directory.str().str();
  llvm::outs() << "source check execution evidence: " << evidence << '\n';
  ASSERT_TRUE(runPython("prepare", evidence));
  auto buffer = llvm::MemoryBuffer::getFile(evidence + "/prepared.json");
  ASSERT_TRUE(buffer);
  auto prepared = llvm::json::parse((*buffer)->getBuffer());
  if (!prepared) {
    ADD_FAILURE() << llvm::toString(prepared.takeError());
    return;
  }
  auto *record = prepared->getAsObject();
  ASSERT_NE(record, nullptr);
  auto root = record->getString("artifact_directory");
  ASSERT_TRUE(root);
  auto *cases = record->getArray("cases");
  ASSERT_NE(cases, nullptr);
  ASSERT_EQ(cases->size(), 7u);
  for (const auto &entry : *cases) {
    auto *specification = entry.getAsObject();
    ASSERT_NE(specification, nullptr);
    auto name = specification->getString("case");
    auto final = specification->getString("final");
    ASSERT_TRUE(name);
    ASSERT_TRUE(final);
    ASSERT_TRUE(emitNative(name->str(), final->str(),
                           std::filesystem::path(root->str()) /
                               ("generated-" + name->str())));
  }
  ASSERT_TRUE(runPython("verify", evidence));
}

TEST(HardwareSourceChecksExecution,
     NativeUnknownAndHighImpedancePathsCanonicalizeInactive) {
  llvm::SmallString<256> directory;
  auto prefix = (std::filesystem::path(PYCIRCUIT_TEST_BINARY_DIR) /
                 "source-observation-noisy-path")
                    .string();
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(prefix, directory));
  auto output = std::filesystem::path(directory.str().str());
  llvm::outs() << "noisy observation evidence: " << output.string() << '\n';
  auto input =
      output.parent_path() / (output.filename().string() + "-input.mlir");
  {
    std::ofstream stream(input, std::ios::binary);
    ASSERT_TRUE(stream.good());
    stream << noisyObservationFinal();
  }
  ASSERT_TRUE(emitNative("noisy-path", input.string(), output));
  ASSERT_TRUE(runNoisyObservationNative(output));
}
