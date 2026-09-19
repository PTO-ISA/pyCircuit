#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/InitAllDialects.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <string>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

/// Move the staged bundle directory into its final location.
///
/// llvm::sys::fs::rename opens the source without FILE_FLAG_BACKUP_SEMANTICS,
/// which Windows requires in order to open a directory, so it cannot move a
/// directory there and fails with permission denied. MoveFileExW moves
/// directories, and the caller has already established that the destination
/// does not exist.
std::error_code publishDirectory(llvm::StringRef staged, llvm::StringRef root) {
#if defined(_WIN32)
  llvm::SmallVector<wchar_t, 0> source;
  llvm::SmallVector<wchar_t, 0> target;
  if (std::error_code error = llvm::sys::windows::UTF8ToUTF16(staged, source))
    return error;
  if (std::error_code error = llvm::sys::windows::UTF8ToUTF16(root, target))
    return error;
  source.push_back(L'\0');
  target.push_back(L'\0');
  // A scanner or indexer can still hold a handle on a file written moments
  // ago, which blocks the move; retry briefly before reporting failure.
  for (unsigned attempt = 0; attempt != 20; ++attempt) {
    if (::MoveFileExW(source.data(), target.data(), MOVEFILE_REPLACE_EXISTING))
      return std::error_code();
    ::Sleep(50);
  }
  return std::error_code(static_cast<int>(::GetLastError()),
                         std::system_category());
#else
  return llvm::sys::fs::rename(staged, root);
#endif
}

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
  if (std::error_code error = publishDirectory(staging, root))
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
  llvm::outs() << "emitted model bundle v1 (" << bundle->size() << " files)\n";
  return EXIT_SUCCESS;
}
