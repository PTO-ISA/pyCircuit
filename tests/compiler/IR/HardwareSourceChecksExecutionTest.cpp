// Existing GTest framework owns this native-development execution gate.
// Prepare real linked inputs, emit in-process, then verify the seven DUTs with
// independent Python oracles. Public checked emission and RTL remain separate.
#include "Compiler/HardwareEmitCommon.h"
#include "Compiler/HardwareEmitCppChecks.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

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
    write(output, "native-emission.json", llvm::formatv("{0:2}\n", llvm::json::Value(
        llvm::json::Object{{"case", caseName}, {"route", "in-process GTest"},
                           {"status", "success"}})).str());
  } catch (const std::exception &error) {
    llvm::errs() << "native check GTest emission: " << error.what() << '\n';
    return false;
  }
  return true;
}

bool runPython(llvm::StringRef phase, const std::string &evidence) {
  auto script = (std::filesystem::path(PYCIRCUIT_TEST_REPO_ROOT) /
      "tests/compiler/lit/Source/Inputs/source-check-execution.py").string();
  llvm::SmallVector<std::string> command{
      PYCIRCUIT_TEST_PYTHON, script, "--phase", phase.str(),
      "--repo", PYCIRCUIT_TEST_REPO_ROOT,
      "--source-compiler", PYCIRCUIT_TEST_SOURCE_COMPILER,
      "--linker", PYCIRCUIT_TEST_LINKER,
      "--cxx", PYCIRCUIT_TEST_CXX, "--scratch", evidence};
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const auto &argument : command) arguments.push_back(argument);
  auto out = evidence + "/" + phase.str() + ".stdout";
  auto err = evidence + "/" + phase.str() + ".stderr";
  llvm::SmallVector<std::optional<llvm::StringRef>> redirects{
      std::nullopt, llvm::StringRef(out), llvm::StringRef(err)};
  std::string error;
  int status = llvm::sys::ExecuteAndWait(command.front(), arguments,
      std::nullopt, redirects, 900, 0, &error);
  if (status != 0) {
    ADD_FAILURE() << phase.str() << " failed, exit " << status << ": " << error
                  << "; evidence " << evidence;
    if (auto log = llvm::MemoryBuffer::getFile(err))
      llvm::errs() << (*log)->getBuffer();
  }
  return status == 0;
}
} // namespace

TEST(HardwareSourceChecksExecution, NativeLifecycle) {
  llvm::SmallString<256> directory;
  auto prefix = (std::filesystem::path(PYCIRCUIT_TEST_BINARY_DIR) /
                 "source-check-execution").string();
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
        std::filesystem::path(root->str()) / ("generated-" + name->str())));
  }
  ASSERT_TRUE(runPython("verify", evidence));
}
