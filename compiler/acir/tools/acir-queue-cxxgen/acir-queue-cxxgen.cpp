#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/InitAllDialects.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <string>
#include <system_error>

namespace {

llvm::cl::opt<std::string> inputFile(llvm::cl::Positional, llvm::cl::Required,
                                     llvm::cl::desc("<frozen-acir>"));
llvm::cl::opt<std::string>
    outputRoot("output-root", llvm::cl::desc("emit the fixed model v1 bundle"),
               llvm::cl::value_desc("directory"));
llvm::cl::opt<std::string> sdkProductVersion(
    "sdk-product-version", llvm::cl::desc("generated ABI product identity"),
    llvm::cl::value_desc("version"));
llvm::cl::opt<std::string> sdkSourceRevision(
    "sdk-source-revision", llvm::cl::desc("generated ABI source identity"),
    llvm::cl::value_desc("revision"));

llvm::Error writeBundle(
    llvm::StringRef root,
    const std::vector<acir::codegen::QueueGraphGeneratedFile> &files) {
  llvm::SmallString<256> staging;
  llvm::SmallString<256> parent(root);
  llvm::sys::path::remove_filename(parent);
  if (parent.empty())
    parent = ".";
  if (std::error_code error = llvm::sys::fs::make_absolute(parent))
    return llvm::createStringError(error, "cannot resolve bundle parent");
  if (std::error_code error = llvm::sys::fs::create_directories(parent))
    return llvm::createStringError(error, "cannot create bundle parent");
  llvm::SmallString<256> stagingPrefix(parent);
  llvm::sys::path::append(stagingPrefix, ".acir-queue-cxxgen");
  if (std::error_code error =
          llvm::sys::fs::createUniqueDirectory(stagingPrefix, staging))
    return llvm::createStringError(error, "cannot create bundle staging root");
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() {
      if (!path.empty())
        llvm::sys::fs::remove_directories(path);
    }
  } cleanup{staging};

  for (const auto &file : files) {
    llvm::SmallString<256> path(staging);
    llvm::sys::path::append(path, file.relativePath);
    llvm::SmallString<256> directory(path);
    llvm::sys::path::remove_filename(directory);
    if (std::error_code error = llvm::sys::fs::create_directories(directory))
      return llvm::createStringError(error,
                                     "cannot create generated directory");
    std::error_code error;
    llvm::raw_fd_ostream stream(path, error);
    if (error)
      return llvm::createStringError(error, "cannot create generated file");
    stream << file.content;
    stream.close();
    if (stream.has_error())
      return llvm::createStringError(stream.error(),
                                     "cannot finish generated file");
  }

  if (llvm::sys::fs::exists(root))
    return llvm::createStringError(
        std::make_error_code(std::errc::file_exists),
        "output root already exists; refusing a partial replacement");
  if (std::error_code error = llvm::sys::fs::rename(staging, root))
    return llvm::createStringError(error, "cannot publish generated bundle");
  cleanup.path.clear();
  return llvm::Error::success();
}

} // namespace

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(
      argc, argv, "Generate typed Queue-wired gfsim C++ from frozen ACIR\n");
  mlir::DialectRegistry registry;
  acir::registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(inputFile, &context);
  if (!module) {
    llvm::errs() << "ACLOWER-QUEUE-CXX: frozen ACIR parsing failed\n";
    return EXIT_FAILURE;
  }
  auto plan = acir::codegen::buildQueueGraphPlan(*module);
  if (!plan) {
    llvm::errs() << llvm::toString(plan.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  if (outputRoot.empty()) {
    if (!sdkProductVersion.empty() || !sdkSourceRevision.empty()) {
      llvm::errs() << "ACLOWER-QUEUE-CXX: SDK identity options require "
                      "--output-root\n";
      return EXIT_FAILURE;
    }
    auto source = acir::codegen::generateQueueGraphCpp(*plan);
    if (!source) {
      llvm::errs() << llvm::toString(source.takeError()) << '\n';
      return EXIT_FAILURE;
    }
    llvm::outs() << *source;
    return EXIT_SUCCESS;
  }

  auto bundle = acir::codegen::generateQueueGraphModelBundle(
      *plan, {.sdkProductVersion = sdkProductVersion,
              .sdkSourceRevision = sdkSourceRevision});
  if (!bundle) {
    llvm::errs() << llvm::toString(bundle.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  if (llvm::Error error = writeBundle(outputRoot, *bundle)) {
    llvm::errs() << "ACLOWER-QUEUE-CXX: " << llvm::toString(std::move(error))
                 << '\n';
    return EXIT_FAILURE;
  }
  llvm::outs() << "emitted model bundle v1 (3 files)\n";
  return EXIT_SUCCESS;
}
