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

int main(int argc, char **argv) {
  llvm::InitLLVM init(argc, argv);
  llvm::cl::opt<std::string> inputFilename(llvm::cl::Positional,
                                           llvm::cl::desc("<input file>"),
                                           llvm::cl::Required);
  llvm::cl::opt<std::string> outputDir(
      "output-dir", llvm::cl::desc("Published simulator directory"),
      llvm::cl::value_desc("dir"), llvm::cl::Required);
  llvm::cl::opt<std::string> profile(
      "profile", llvm::cl::desc("Build profile"), llvm::cl::init("fast"));
  llvm::cl::opt<std::string> target(
      "target", llvm::cl::desc("Toolchain target"),
      llvm::cl::init("x86_64-linux-gnu"));
  llvm::cl::opt<std::string> cxxCompiler(
      "cxx", llvm::cl::desc("Host C++ compiler"), llvm::cl::init(ACIR_HOST_CXX));
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "Freeze, lower, emit, and compile ACIR\n");

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
    PassManager freeze(&context);
    freeze.enableVerifier(false);
    freeze.addPass(acir::createFreezeTopologyPass());
    if (failed(freeze.run(module.get())))
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
  fs::path finalDir = outputDir.getValue();
  fs::path work = fs::path(outputDir.getValue() + ".work");
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work);

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

  fs::path backup = fs::path(finalDir.string() + ".prev");
  if (fs::exists(finalDir)) {
    fs::remove_all(backup, ec);
    fs::rename(finalDir, backup, ec);
  }
  fs::rename(work, finalDir, ec);
  if (ec) {
    if (fs::exists(backup))
      fs::rename(backup, finalDir, ec);
    llvm::errs() << "acir-build: cannot publish " << finalDir.string() << '\n';
    return 1;
  }
  fs::remove_all(backup, ec);
  return 0;
}
