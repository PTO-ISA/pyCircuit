#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/CodeGen/QueueGraphPyc.h"
#include "acir/InitAllDialects.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <string>
#include <system_error>
#include <vector>

namespace {

enum class EmitMode { None, Cpp, CppBundle, Verilog };

struct Options {
  std::string input;
  std::string output;
  EmitMode mode = EmitMode::None;
};

llvm::Error accError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument), message);
}

int fail(llvm::Error error) {
  llvm::errs() << "ACLOWER-QUEUE-CXX: acc: " << llvm::toString(std::move(error))
               << '\n';
  return EXIT_FAILURE;
}

void printHelp(llvm::StringRef program) {
  llvm::outs() << "usage: " << program
               << " -c INPUT.ac EMIT_MODE -o OUTPUT\n"
                  "\n"
                  "  -c INPUT.ac          verified ACIR input\n"
                  "  -emit-cpp            emit one concatenated C++ file\n"
                  "  -emit-cpp-bundle     emit the deterministic model bundle\n"
                  "  -emit-verilog        emit one Verilog file through pycc\n"
                  "  -o OUTPUT            output file or bundle directory\n";
}

llvm::Expected<Options> parseOptions(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    llvm::StringRef argument(argv[index]);
    if (argument == "-h" || argument == "--help") {
      printHelp(argv[0]);
      std::exit(EXIT_SUCCESS);
    }
    if (argument == "-c" || argument == "-o") {
      if (index + 1 == argc)
        return accError(argument + " requires a value");
      llvm::StringRef value(argv[++index]);
      if (value.empty() || value.starts_with("-"))
        return accError(argument + " requires a value");
      std::string &destination =
          argument == "-c" ? options.input : options.output;
      if (!destination.empty())
        return accError(argument + " may be specified only once");
      destination = value.str();
      continue;
    }
    if (argument == "-emit-cpp" || argument == "-emit-cpp-bundle" ||
        argument == "-emit-verilog") {
      EmitMode requested = EmitMode::Verilog;
      if (argument == "-emit-cpp")
        requested = EmitMode::Cpp;
      else if (argument == "-emit-cpp-bundle")
        requested = EmitMode::CppBundle;
      if (options.mode != EmitMode::None)
        return accError("exactly one emit mode is required");
      options.mode = requested;
      continue;
    }
    return accError("unknown argument '" + argument + "'");
  }

  if (options.input.empty())
    return accError("-c INPUT.ac is required");
  if (options.output.empty())
    return accError("-o OUTPUT is required");
  if (options.mode == EmitMode::None)
    return accError("exactly one emit mode is required");
  return options;
}

llvm::Expected<llvm::SmallString<256>> prepareParent(llvm::StringRef output) {
  llvm::SmallString<256> parent(output);
  llvm::sys::path::remove_filename(parent);
  if (parent.empty())
    parent = ".";
  if (std::error_code error = llvm::sys::fs::make_absolute(parent))
    return llvm::createStringError(error, "cannot resolve output parent");
  if (std::error_code error = llvm::sys::fs::create_directories(parent))
    return llvm::createStringError(error, "cannot create output parent");
  return parent;
}

llvm::Error requireAbsent(llvm::StringRef output) {
  if (llvm::sys::fs::exists(output))
    return llvm::createStringError(
        std::make_error_code(std::errc::file_exists),
        "output already exists; refusing a partial replacement");
  return llvm::Error::success();
}

llvm::Error writeCppAtomically(llvm::StringRef output, llvm::StringRef source) {
  if (auto error = requireAbsent(output))
    return error;
  auto parent = prepareParent(output);
  if (!parent)
    return parent.takeError();

  llvm::SmallString<256> stagingModel(*parent);
  llvm::sys::path::append(stagingModel, ".acc-%%%%%%.cpp.tmp");
  llvm::SmallString<256> staging;
  int descriptor = -1;
  if (std::error_code error =
          llvm::sys::fs::createUniqueFile(stagingModel, descriptor, staging))
    return llvm::createStringError(error, "cannot create C++ staging file");

  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() {
      if (!path.empty())
        llvm::sys::fs::remove(path);
    }
  } cleanup{staging};

  {
    llvm::raw_fd_ostream stream(descriptor, true);
    stream << source;
    stream.flush();
    if (stream.has_error())
      return llvm::createStringError(stream.error(),
                                     "cannot finish C++ staging file");
  }
  if (auto error = requireAbsent(output))
    return error;
  if (std::error_code error = llvm::sys::fs::rename(staging, output))
    return llvm::createStringError(error, "cannot publish generated C++");
  cleanup.path.clear();
  return llvm::Error::success();
}

llvm::Expected<llvm::SmallString<256>>
findSiblingPycc(llvm::StringRef program) {
  llvm::ErrorOr<std::string> resolved = llvm::sys::findProgramByName(program);
  llvm::SmallString<256> executable = resolved ? *resolved : program;
  llvm::SmallString<256> realExecutable;
  if (std::error_code error =
          llvm::sys::fs::real_path(executable, realExecutable))
    return llvm::createStringError(error, "cannot resolve acc executable");
  llvm::sys::path::remove_filename(realExecutable);
#if defined(_WIN32)
  llvm::sys::path::append(realExecutable, "pycc.exe");
#else
  llvm::sys::path::append(realExecutable, "pycc");
#endif
  if (!llvm::sys::fs::can_execute(realExecutable))
    return accError("sibling pycc executable is unavailable");
  return realExecutable;
}

llvm::Error writeVerilogAtomically(llvm::StringRef program,
                                   llvm::StringRef output,
                                   llvm::StringRef pycSource) {
  if (auto error = requireAbsent(output))
    return error;
  auto parent = prepareParent(output);
  if (!parent)
    return parent.takeError();
  auto pycc = findSiblingPycc(program);
  if (!pycc)
    return pycc.takeError();

  struct Cleanup {
    llvm::SmallString<256> pyc;
    llvm::SmallString<256> verilog;
    llvm::SmallString<256> verilogStats;
    ~Cleanup() {
      if (!pyc.empty())
        llvm::sys::fs::remove(pyc);
      if (!verilog.empty())
        llvm::sys::fs::remove(verilog);
      if (!verilogStats.empty())
        llvm::sys::fs::remove(verilogStats);
    }
  } cleanup;

  llvm::SmallString<256> pycModel(*parent);
  llvm::sys::path::append(pycModel, ".acc-%%%%%%.pyc.tmp");
  int pycDescriptor = -1;
  if (std::error_code error =
          llvm::sys::fs::createUniqueFile(pycModel, pycDescriptor, cleanup.pyc))
    return llvm::createStringError(error, "cannot create PYC staging file");
  {
    llvm::raw_fd_ostream stream(pycDescriptor, true);
    stream << pycSource;
    stream.flush();
    if (stream.has_error())
      return llvm::createStringError(stream.error(),
                                     "cannot finish PYC staging file");
  }

  llvm::SmallString<256> verilogModel(*parent);
  llvm::sys::path::append(verilogModel, ".acc-%%%%%%.v.tmp");
  int verilogDescriptor = -1;
  if (std::error_code error = llvm::sys::fs::createUniqueFile(
          verilogModel, verilogDescriptor, cleanup.verilog))
    return llvm::createStringError(error, "cannot create Verilog staging file");
  cleanup.verilogStats = cleanup.verilog;
  cleanup.verilogStats += ".stats.json";
  if (std::error_code error = llvm::sys::fs::closeFile(verilogDescriptor))
    return llvm::createStringError(error, "cannot close Verilog staging file");

  std::vector<std::string> ownedArguments{
      pycc->str().str(),
      cleanup.pyc.str().str(),
      "-verilog",
      cleanup.verilog.str().str(),
      "--hierarchy-policy=strict",
      "--inline-policy=off",
  };
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  if (llvm::sys::ExecuteAndWait(arguments.front(), arguments) != 0)
    return accError("pycc Verilog emission failed");

  if (auto error = requireAbsent(output))
    return error;
  if (std::error_code error = llvm::sys::fs::rename(cleanup.verilog, output))
    return llvm::createStringError(error, "cannot publish generated Verilog");
  cleanup.verilog.clear();
  return llvm::Error::success();
}

llvm::Error writeBundleAtomically(
    llvm::StringRef output,
    const std::vector<acir::codegen::QueueGraphGeneratedFile> &files) {
  if (auto error = requireAbsent(output))
    return error;
  auto parent = prepareParent(output);
  if (!parent)
    return parent.takeError();

  llvm::SmallString<256> stagingPrefix(*parent);
  llvm::sys::path::append(stagingPrefix, ".acc-bundle");
  llvm::SmallString<256> staging;
  if (std::error_code error =
          llvm::sys::fs::createUniqueDirectory(stagingPrefix, staging))
    return llvm::createStringError(error,
                                   "cannot create bundle staging directory");

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

  if (auto error = requireAbsent(output))
    return error;
  if (std::error_code error = llvm::sys::fs::rename(staging, output))
    return llvm::createStringError(error, "cannot publish generated bundle");
  cleanup.path.clear();
  return llvm::Error::success();
}

} // namespace

int main(int argc, char **argv) {
  auto options = parseOptions(argc, argv);
  if (!options)
    return fail(options.takeError());

  mlir::DialectRegistry registry;
  acir::registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(options->input, &context);
  if (!module)
    return fail(accError("verified ACIR parsing failed"));

  auto plan = acir::codegen::buildQueueGraphPlan(*module);
  if (!plan)
    return fail(plan.takeError());

  if (options->mode == EmitMode::Verilog) {
    auto pyc = acir::codegen::generateQueueGraphPyc(*plan);
    if (!pyc)
      return fail(pyc.takeError());
    if (auto error = writeVerilogAtomically(argv[0], options->output, *pyc))
      return fail(std::move(error));
    return EXIT_SUCCESS;
  }

  if (options->mode == EmitMode::Cpp) {
    auto source = acir::codegen::generateQueueGraphCpp(*plan);
    if (!source)
      return fail(source.takeError());
    if (auto error = writeCppAtomically(options->output, *source))
      return fail(std::move(error));
    return EXIT_SUCCESS;
  }

  auto bundle = acir::codegen::generateQueueGraphModelBundle(
      *plan, {.sdkProductVersion = ACC_SDK_PRODUCT_VERSION,
              .sdkSourceRevision = ACC_SDK_SOURCE_REVISION});
  if (!bundle)
    return fail(bundle.takeError());
  if (auto error = writeBundleAtomically(options->output, *bundle))
    return fail(std::move(error));
  return EXIT_SUCCESS;
}
