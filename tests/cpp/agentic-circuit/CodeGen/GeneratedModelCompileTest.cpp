#include "TestToolchain.h"
#include "acir/CodeGen/Generator.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace acir::codegen {
namespace {

constexpr llvm::StringLiteral kFingerprint =
    "sha256:0000000000000000000000000000000000000000000000000000000000000000";

ModelPlan makeMinimalRunnablePlan() {
  ModelPlan plan;
  plan.modelSymbol = "minimal";
  plan.rootSymbol = "Top";
  plan.contractEpoch = "0.4";
  plan.frozenAcirFingerprint = kFingerprint.str();
  plan.bindingLockFingerprint = kFingerprint.str();
  plan.providerFingerprint = kFingerprint.str();
  plan.profileFingerprint = kFingerprint.str();
  plan.toolchainFingerprint = kFingerprint.str();
  plan.schemaSetFingerprint = kFingerprint.str();
  plan.timeDomains.push_back({"core", 2, 0, 1});
  ModulePlan module{.symbol = "Top",
                    .className = "Top_s0000000000000000",
                    .specializationFingerprint = kFingerprint.str()};
  ProcessPlan process{.symbol = "scalar",
                      .className = "scalar_s0000000000000000",
                      .specializationFingerprint = kFingerprint.str(),
                      .entryPc = "entry",
                      .fairnessWork = 4};
  PcStatePlan state{.ordinal = 0, .name = "entry"};
  state.operations.push_back(ConstantPlan{"left", "i32", 7});
  state.operations.push_back(ConstantPlan{"right", "i32", 5});
  state.operations.push_back(
      ArithmeticPlan{"arith.addi", {"left", "right"}, {"sum"}, {"i32"}, {}});
  state.terminator = TerminatePlan{"success"};
  PcBlockPlan entryBlock{.ordinal = 0};
  entryBlock.operations.push_back(ConstantPlan{"condition", "i1", true});
  entryBlock.operations.push_back(ConstantPlan{"left", "i32", 7});
  entryBlock.operations.push_back(ConstantPlan{"right", "i32", 5});
  entryBlock.terminator =
      ConditionalBranchPlan{"condition", 1, {"left"}, 2, {"right"}};
  PcBlockPlan trueBlock{.ordinal = 1,
                        .arguments = {{"selected_true", "i32"}},
                        .terminator = TerminatePlan{"success"}};
  trueBlock.operations.push_back(
      ArithmeticPlan{"arith.addi",
                     {"selected_true", "selected_true"},
                     {"true_sum"},
                     {"i32"},
                     {}});
  PcBlockPlan falseBlock{.ordinal = 2,
                         .arguments = {{"selected_false", "i32"}},
                         .terminator = TerminatePlan{"success"}};
  falseBlock.operations.push_back(
      ArithmeticPlan{"arith.addi",
                     {"selected_false", "selected_false"},
                     {"false_sum"},
                     {"i32"},
                     {}});
  state.blocks = {std::move(entryBlock), std::move(trueBlock),
                  std::move(falseBlock)};
  process.states.push_back(std::move(state));
  module.processes.push_back(std::move(process));
  plan.modules.push_back(std::move(module));
  plan.constructionOrder = {"Top.scalar"};
  plan.destructionOrder = {"Top.scalar"};
  plan.runtimeObjects.push_back({.objectId = 0,
                                 .activationId = 0,
                                 .targetSymbol = "Top::scalar",
                                 .hierarchyPath = "Top.scalar",
                                 .objectKind = RuntimeObjectKind::Process,
                                 .workThunk = "scalar_work",
                                 .xferThunk = "scalar_xfer",
                                 .resetThunk = "scalar_reset",
                                 .validateThunk = "scalar_validate"});
  return plan;
}

llvm::Error writeBundle(llvm::StringRef root, const SourceBundle &bundle) {
  for (const GeneratedFile &file : bundle.files) {
    llvm::SmallString<256> path(root);
    llvm::sys::path::append(path, file.relativePath);
    llvm::SmallString<256> parent(path);
    llvm::sys::path::remove_filename(parent);
    if (std::error_code error = llvm::sys::fs::create_directories(parent))
      return llvm::createStringError(error,
                                     "cannot create generated directory");
    std::error_code error;
    llvm::raw_fd_ostream output(path, error);
    if (error)
      return llvm::createStringError(error, "cannot create generated file");
    output << file.content;
  }
  return llvm::Error::success();
}

std::string readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  if (!buffer)
    return {};
  return buffer.get()->getBuffer().str();
}

TEST(GeneratedModelCompileTest,
     MinimalBundleCompilesLinksAndPrintsFingerprint) {
  auto bundle = generateModelSources(makeMinimalRunnablePlan());
  if (!bundle) {
    ADD_FAILURE() << llvm::toString(bundle.takeError());
    return;
  }

  llvm::SmallString<256> temporaryRoot;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acir-generated-model",
                                                    temporaryRoot));
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() { llvm::sys::fs::remove_directories(path); }
  } cleanup{temporaryRoot};
  if (llvm::Error error = writeBundle(temporaryRoot, *bundle)) {
    ADD_FAILURE() << llvm::toString(std::move(error));
    return;
  }

  llvm::SmallString<256> executable(temporaryRoot);
  llvm::sys::path::append(executable, "generated-model");
  llvm::SmallString<256> compileLog(temporaryRoot);
  llvm::sys::path::append(compileLog, "compile.log");
  llvm::SmallString<256> generatedInclude(temporaryRoot);
  llvm::sys::path::append(generatedInclude, "include");

  std::vector<std::string> ownedArguments = {
      ACIR_TEST_CXX_COMPILER, "-std=c++20", "-I" + generatedInclude.str().str(),
      "-I" ACIR_TEST_SOURCE_DIR "/simulator/gfsim/include"};
  for (const std::string &include : test::llvmIncludeDirectories())
    ownedArguments.push_back("-I" + include);
  if (llvm::StringRef(ACIR_TEST_RTTI_FLAG).size())
    ownedArguments.push_back(ACIR_TEST_RTTI_FLAG);
  if (llvm::StringRef(ACIR_TEST_SANITIZER_FLAG).size())
    ownedArguments.push_back(ACIR_TEST_SANITIZER_FLAG);
  for (const GeneratedFile &file : bundle->files) {
    if (!llvm::StringRef(file.relativePath).ends_with(".cpp"))
      continue;
    llvm::SmallString<256> path(temporaryRoot);
    llvm::sys::path::append(path, file.relativePath);
    ownedArguments.push_back(path.str().str());
  }
  ownedArguments.push_back(ACIR_TEST_BINARY_DIR "/gfsim/libgfsim.a");
  ownedArguments.push_back(ACIR_TEST_BINARY_DIR
                           "/lib/Bindings/libACIRBindings.a");
  for (const std::string &flag : test::llvmLinkerFlags())
    ownedArguments.push_back(flag);
  ownedArguments.push_back("-o");
  ownedArguments.push_back(executable.str().str());
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, compileLog.str(), compileLog.str()};
  const int compileStatus = llvm::sys::ExecuteAndWait(
      ACIR_TEST_CXX_COMPILER, arguments, std::nullopt, redirects);
  ASSERT_EQ(compileStatus, 0) << readFile(compileLog);

  llvm::SmallString<256> queryLog(temporaryRoot);
  llvm::sys::path::append(queryLog, "query.log");
  const std::array<llvm::StringRef, 2> queryArguments = {executable.str(),
                                                         "--build-fingerprint"};
  redirects = {std::nullopt, queryLog.str(), queryLog.str()};
  const int queryStatus = llvm::sys::ExecuteAndWait(executable, queryArguments,
                                                    std::nullopt, redirects);
  ASSERT_EQ(queryStatus, 0) << readFile(queryLog);
  EXPECT_EQ(readFile(queryLog), bundle->buildFingerprint + "\n");

  llvm::SmallString<256> runLog(temporaryRoot);
  llvm::sys::path::append(runLog, "run.log");
  const std::array<llvm::StringRef, 1> runArguments = {executable.str()};
  redirects = {std::nullopt, runLog.str(), runLog.str()};
  const int runStatus = llvm::sys::ExecuteAndWait(executable, runArguments,
                                                  std::nullopt, redirects);
  EXPECT_EQ(runStatus, 0) << readFile(runLog);
}

} // namespace
} // namespace acir::codegen
