#include "acir/CodeGen/EmitCxx.h"
#include "acir/Conversion/ACIRToACSim/ACIRToACSim.h"
#include "acir/InitAllDialects.h"
#include "acir/Transforms/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#ifndef ACIR_HOST_CXX
#define ACIR_HOST_CXX "c++"
#endif
#ifndef ACIR_GFSIM_INCLUDE
#define ACIR_GFSIM_INCLUDE "."
#endif
#ifndef ACIR_GFSIM_LIBRARY
#define ACIR_GFSIM_LIBRARY "libgfsim.a"
#endif

using namespace mlir;

namespace {

constexpr llvm::StringLiteral kOutputMarker = "agentic-circuit-output-v1\n";

std::optional<std::filesystem::path>
prepareOutputPath(llvm::StringRef requested) {
  namespace fs = std::filesystem;
  if (requested.empty())
    return std::nullopt;
  std::error_code ec;
  fs::path output = fs::absolute(requested.str(), ec).lexically_normal();
  if (ec || output.empty() || output == output.root_path() ||
      output.filename().empty())
    return std::nullopt;
  fs::create_directories(output.parent_path(), ec);
  if (ec)
    return std::nullopt;
  fs::path parent = fs::weakly_canonical(output.parent_path(), ec);
  if (ec)
    return std::nullopt;
  output = parent / output.filename();
  fs::file_status status = fs::symlink_status(output, ec);
  if (ec && ec != std::errc::no_such_file_or_directory)
    return std::nullopt;
  ec.clear();
  if (fs::is_symlink(status))
    return std::nullopt;
  if (fs::exists(status)) {
    if (!fs::is_directory(status))
      return std::nullopt;
    const bool empty = fs::is_empty(output, ec);
    if (ec)
      return std::nullopt;
    if (!empty) {
      std::ifstream marker(output / ".agentic-circuit-output");
      std::string value((std::istreambuf_iterator<char>(marker)), {});
      if (!marker || value != kOutputMarker.str())
        return std::nullopt;
    }
  }
  return output;
}

std::optional<std::filesystem::path>
createOwnedSibling(const std::filesystem::path &finalDir,
                   llvm::StringRef role) {
  llvm::SmallString<256> created;
  const std::string prefix = (finalDir.parent_path() /
                              (finalDir.filename().string() + "." + role.str()))
                                 .string();
  if (llvm::sys::fs::createUniqueDirectory(prefix, created))
    return std::nullopt;
  return std::filesystem::path(created.str().str());
}

} // namespace

int main(int argc, char **argv) {
  llvm::InitLLVM init(argc, argv);
  llvm::cl::opt<std::string> inputFilename(
      llvm::cl::Positional, llvm::cl::desc("<input file>"), llvm::cl::Required);
  llvm::cl::opt<std::string> outputDir(
      "output-dir", llvm::cl::desc("Published simulator directory"),
      llvm::cl::value_desc("dir"), llvm::cl::Required);
  llvm::cl::opt<std::string> profile("profile", llvm::cl::desc("Build profile"),
                                     llvm::cl::init("fast"));
  llvm::cl::opt<std::string> target("target",
                                    llvm::cl::desc("Toolchain target"),
                                    llvm::cl::init("x86_64-linux-gnu"));
  llvm::cl::opt<std::string> cxxCompiler("cxx",
                                         llvm::cl::desc("Host C++ compiler"),
                                         llvm::cl::init(ACIR_HOST_CXX));
  llvm::cl::ParseCommandLineOptions(
      argc, argv, "Verify, normalize, lower, emit, and compile ACIR\n");

  DialectRegistry registry;
  acir::registerAllDialects(registry);
  MLIRContext context(registry);
  context.loadAllAvailableDialects();

  std::string error;
  auto file = openInputFile(inputFilename, &error);
  if (!file) {
    llvm::errs() << error << '\n';
    return 1;
  }
  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(file), llvm::SMLoc());
  auto module = parseSourceFile<ModuleOp>(sourceMgr, &context);
  if (!module)
    return 1;

  {
    PassManager canonicalize(&context);
    canonicalize.addPass(acir::createVerifyACIRFilePass());
    acir::addRuleLoweringPipeline(canonicalize);
    canonicalize.addPass(acir::createNormalizeACIRFilePass());
    canonicalize.addPass(acir::createFreezeTopologyPass());
    if (failed(canonicalize.run(module.get())))
      return 1;
  }

  acir::ACIRToACSimPassOptions lowerOptions;
  lowerOptions.profile = profile;
  lowerOptions.target = target;
  {
    PassManager lower(&context);
    lower.addPass(acir::createACIRToACSimPass(lowerOptions));
    if (failed(lower.run(module.get())))
      return 1;
  }

  namespace fs = std::filesystem;
  auto finalDirOr = prepareOutputPath(outputDir);
  if (!finalDirOr) {
    llvm::errs() << "acir-build: unsafe or unowned output directory\n";
    return 1;
  }
  fs::path finalDir = *finalDirOr;
  auto workOr = createOwnedSibling(finalDir, "work");
  if (!workOr) {
    llvm::errs() << "acir-build: cannot create owned work directory\n";
    return 1;
  }
  fs::path work = *workOr;
  std::error_code ec;

  acir::codegen::EmitCxxOptions emitOptions;
  emitOptions.outputDir = work.string();
  emitOptions.profile = profile;
  emitOptions.toolchainTarget = target;
  if (failed(acir::codegen::emitCxxFile(module.get(), emitOptions))) {
    fs::remove_all(work, ec);
    return 1;
  }

  llvm::SmallString<128> simPath;
  simPath = work.string();
  llvm::sys::path::append(simPath, "sim");
  llvm::SmallString<128> modelCpp;
  modelCpp = work.string();
  llvm::sys::path::append(modelCpp, "src", "generated", "model.cpp");
  llvm::SmallString<128> mainCpp;
  mainCpp = work.string();
  llvm::sys::path::append(mainCpp, "src", "generated", "main.cpp");
  llvm::SmallString<128> includeDir;
  includeDir = work.string();
  llvm::sys::path::append(includeDir, "include");

  llvm::SmallVector<llvm::StringRef, 16> args = {
      cxxCompiler,
      "-std=c++20",
      "-I",
      ACIR_GFSIM_INCLUDE,
      "-I",
      includeDir.c_str(),
      modelCpp.c_str(),
      mainCpp.c_str(),
      ACIR_GFSIM_LIBRARY,
      "-o",
      simPath.c_str(),
  };
  std::string compileError;
  int compile = llvm::sys::ExecuteAndWait(cxxCompiler, args, {}, {}, 120, 0,
                                          &compileError);
  if (compile != 0) {
    llvm::errs() << "acir-build: host compile failed: " << compileError << '\n';
    fs::remove_all(work, ec);
    return 1;
  }

  std::optional<fs::path> backupRoot;
  if (fs::exists(finalDir)) {
    backupRoot = createOwnedSibling(finalDir, "backup");
    if (!backupRoot) {
      fs::remove_all(work, ec);
      llvm::errs() << "acir-build: cannot create owned backup directory\n";
      return 1;
    }
    fs::rename(finalDir, *backupRoot / "previous", ec);
    if (ec) {
      fs::remove_all(work, ec);
      fs::remove_all(*backupRoot, ec);
      llvm::errs() << "acir-build: cannot preserve previous output\n";
      return 1;
    }
  }
  fs::rename(work, finalDir, ec);
  if (ec) {
    if (backupRoot && fs::exists(*backupRoot / "previous")) {
      std::error_code restore;
      fs::rename(*backupRoot / "previous", finalDir, restore);
      if (!restore) {
        fs::remove_all(*backupRoot, restore);
      } else {
        llvm::errs() << "acir-build: cannot restore previous output from "
                     << (*backupRoot / "previous").string() << ": "
                     << restore.message() << '\n';
      }
    }
    fs::remove_all(work, ec);
    llvm::errs() << "acir-build: cannot publish " << finalDir.string() << '\n';
    return 1;
  }
  if (backupRoot)
    fs::remove_all(*backupRoot, ec);
  return 0;
}
