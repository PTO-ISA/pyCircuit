#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/CodeGen/QueueGraphPyc.h"
#include "acir/InitAllDialects.h"
#include "acir/Transforms/Passes.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace {

enum class EmitMode { None, Verify, Cpp, CppBundle, Verilog };

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
                  "  -c INPUT.ac          linked AC package directory; a single\n"
                  "                       non-hierarchical AC unit is also accepted\n"
                  "  -verify              verify and link without backend output\n"
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
    if (argument == "-verify" || argument == "-emit-cpp" ||
        argument == "-emit-cpp-bundle" ||
        argument == "-emit-verilog") {
      EmitMode requested = EmitMode::Verilog;
      if (argument == "-verify")
        requested = EmitMode::Verify;
      else if (argument == "-emit-cpp")
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
  if (options.mode != EmitMode::Verify && options.output.empty())
    return accError("-o OUTPUT is required");
  if (options.mode == EmitMode::Verify && !options.output.empty())
    return accError("-verify does not accept -o");
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

llvm::Expected<mlir::OwningOpRef<mlir::ModuleOp>>
parseSingleUnit(llvm::StringRef path, mlir::MLIRContext &context,
                bool verifyAfterParse) {
  mlir::ParserConfig config(&context, verifyAfterParse);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(path, config);
  if (!module)
    return accError(llvm::Twine("AC unit parsing failed: ") + path);
  return module;
}

llvm::Expected<mlir::OwningOpRef<mlir::ModuleOp>>
loadAcInput(llvm::StringRef input, mlir::MLIRContext &context) {
  if (!llvm::sys::fs::is_directory(input)) {
    auto module = parseSingleUnit(input, context, true);
    if (!module)
      return module.takeError();
    if (auto unitKind = (*module)->getOperation()->getAttrOfType<mlir::StringAttr>(
            "ac.unit_kind");
        unitKind && unitKind.getValue() == "source")
      return accError(
          "structured input requires a directory-backed AC package; "
          "a standalone source-owned module is not a complete design");
    unsigned definitions = 0;
    bool hasSystem = false;
    std::optional<std::string> sourceFile;
    bool mixedSources = false;
    (*module)->walk([&](mlir::Operation *operation) {
      llvm::StringRef name = operation->getName().getStringRef();
      hasSystem |= name == "ac.system";
      if (name != "ac.module")
        return;
      ++definitions;
      auto source = operation->getAttrOfType<mlir::StringAttr>("ac.source_file");
      if (!source || source.getValue().empty()) {
        mixedSources = true;
        return;
      }
      if (!sourceFile)
        sourceFile = source.getValue().str();
      else if (*sourceFile != source.getValue())
        mixedSources = true;
    });
    if (definitions > 1 && (hasSystem || mixedSources))
      return accError(
          "structured input requires a directory-backed AC package; "
          "a standalone AC source unit may contain multiple definitions only "
          "when they share one ac.source_file");
    return module;
  }

  std::error_code error;
  llvm::SmallVector<std::string> files;
  for (llvm::sys::fs::recursive_directory_iterator iterator(input, error), end;
       iterator != end && !error; iterator.increment(error)) {
    llvm::StringRef path = iterator->path();
    llvm::sys::fs::file_status status;
    if (std::error_code statusError = llvm::sys::fs::status(path, status, false))
      return llvm::createStringError(statusError,
                                     "cannot inspect AC package unit");
    if (llvm::sys::fs::is_symlink_file(status))
      return accError("AC package must not contain symlinks");
    if (llvm::sys::fs::is_directory(status))
      continue;
    if (!llvm::sys::fs::is_regular_file(status) ||
        llvm::sys::path::extension(path) != ".ac")
      return accError("AC package may contain only .ac files and directories");
    files.push_back(path.str());
  }
  if (error)
    return llvm::createStringError(error, "cannot enumerate AC package");
  llvm::sort(files);

  llvm::SmallString<256> corePath(input);
  llvm::sys::path::append(corePath, "core.ac");
  if (!llvm::is_contained(files, corePath.str().str()))
    return accError("AC package requires core.ac");

  auto core = parseSingleUnit(corePath, context, false);
  if (!core)
    return core.takeError();
  auto coreKind = (*core)->getOperation()->getAttrOfType<mlir::StringAttr>(
      "ac.unit_kind");
  if (!coreKind || coreKind.getValue() != "core")
    return accError("core.ac requires ac.unit_kind = \"core\"");
  std::set<std::string> sourceOwners;
  std::set<std::string> interfaceOwners;
  struct ModuleHeader {
    acir::ac::ModuleFamilySchemaAttr schema;
    acir::ac::SourceOwnerAttr source;
  };
  std::map<std::string, ModuleHeader> moduleHeaders;
  std::set<std::string> sourceDefinitions;
  acir::ac::TypeScopeOp linkedTypeScope;
  for (const std::string &path : files) {
    if (path == corePath)
      continue;
    auto unit = parseSingleUnit(path, context, false);
    if (!unit)
      return unit.takeError();
    auto kind = (*unit)->getOperation()->getAttrOfType<mlir::StringAttr>(
        "ac.unit_kind");
    if (!kind)
      return accError("non-core AC unit requires ac.unit_kind");
    if (kind.getValue() == "interface") {
      auto owner = (*unit)->getOperation()->getAttrOfType<mlir::StringAttr>(
          "ac.unit_source");
      if (!owner || owner.getValue().empty())
        return accError("interface AC unit requires ac.unit_source");
      auto interfaceKind =
          (*unit)->getOperation()->getAttrOfType<mlir::StringAttr>(
              "ac.interface_kind");
      if (!interfaceKind || interfaceKind.getValue().empty())
        return accError("interface AC unit requires ac.interface_kind");
      std::string interfaceIdentity =
          (interfaceKind.getValue() + ":" + owner.getValue()).str();
      if (!interfaceOwners.insert(interfaceIdentity).second)
        return accError(
            "Python interface source is published by more than one AC unit");
      if (interfaceKind.getValue() == "source") {
        acir::ac::TypeScopeOp sourceTypeScope;
        llvm::SmallVector<mlir::Operation *> imports;
        for (mlir::Operation &operation : (*unit)->getBody()->getOperations()) {
          if (auto candidate = mlir::dyn_cast<acir::ac::TypeScopeOp>(operation)) {
            if (sourceTypeScope)
              return accError(
                  "source interface unit contains multiple type scopes");
            sourceTypeScope = candidate;
            for (mlir::Operation &definition : candidate.getBody().front()) {
              auto source = definition.getAttrOfType<mlir::StringAttr>(
                  "ac.source_file");
              if (!source || source != owner)
                return accError(
                    "source interface type is owned by another Python file");
            }
            continue;
          }
          auto import = mlir::dyn_cast<acir::ac::ModuleImportOp>(operation);
          if (!import)
            return accError(
                "source interface unit may contain only one type scope and module imports");
          auto source = import.getSource();
          if (!source || source.getImplementation() != owner)
            return accError(
                "source interface module is owned by another Python file");
          ModuleHeader header{import.getSchema(), source};
          if (!moduleHeaders
                   .emplace(import.getSymName().str(), std::move(header))
                   .second)
            return accError("module definition is published by more than one "
                            "interface header");
          imports.push_back(&operation);
        }
        if (sourceTypeScope) {
          if (!linkedTypeScope) {
            (*core)->getBody()->getOperations().splice(
                (*core)->getBody()->begin(), (*unit)->getBody()->getOperations(),
                sourceTypeScope->getIterator());
            linkedTypeScope = sourceTypeScope;
          } else {
            linkedTypeScope.getBody().front().getOperations().splice(
                linkedTypeScope.getBody().front().end(),
                sourceTypeScope.getBody().front().getOperations());
          }
        }
        for (mlir::Operation *import : imports)
          (*core)->getBody()->getOperations().splice(
              (*core)->getBody()->begin(), (*unit)->getBody()->getOperations(),
              import->getIterator());
      } else if (interfaceKind.getValue() == "layouts") {
        acir::ac::TypeScopeOp sourceTypeScope;
        for (mlir::Operation &operation : (*unit)->getBody()->getOperations()) {
          auto candidate = mlir::dyn_cast<acir::ac::TypeScopeOp>(operation);
          if (!candidate || sourceTypeScope)
            return accError(
                "type interface AC unit requires exactly one ac.type_scope");
          sourceTypeScope = candidate;
        }
        for (mlir::Operation &definition : sourceTypeScope.getBody().front()) {
          auto source = definition.getAttrOfType<mlir::StringAttr>(
              "ac.source_file");
        }
        if (!linkedTypeScope) {
          (*core)->getBody()->getOperations().splice(
              (*core)->getBody()->begin(), (*unit)->getBody()->getOperations(),
              sourceTypeScope->getIterator());
          linkedTypeScope = sourceTypeScope;
        } else {
          if (auto layout = sourceTypeScope->getAttr("dlti.dl_spec"))
            linkedTypeScope->setAttr("dlti.dl_spec", layout);
          linkedTypeScope.getBody().front().getOperations().splice(
              linkedTypeScope.getBody().front().end(),
              sourceTypeScope.getBody().front().getOperations());
        }
      } else {
        return accError("interface AC unit has unknown ac.interface_kind");
      }
      continue;
    } else if (kind.getValue() == "source") {
      auto owner = (*unit)->getOperation()->getAttrOfType<mlir::StringAttr>(
          "ac.unit_source");
      if (!owner || owner.getValue().empty())
        return accError("source AC unit requires ac.unit_source");
      if (!sourceOwners.insert(owner.getValue().str()).second)
        return accError("Python source is published by more than one AC unit");
      unsigned definitions = 0;
      for (mlir::Operation &operation : (*unit)->getBody()->getOperations()) {
        llvm::StringRef operationName = operation.getName().getStringRef();
        if (operationName != "ac.module" && operationName != "func.func")
          return accError(
              "source AC unit may contain only modules and pure helpers");
        if (operationName == "ac.module") {
          ++definitions;
          auto definition = mlir::cast<acir::ac::ModuleOp>(operation);
          if (!sourceDefinitions.insert(definition.getSymName().str()).second)
            return accError("module definition is published by more than one "
                            "source AC unit");
        }
        auto source = mlir::dyn_cast<acir::ac::ModuleOp>(operation)
                          ? mlir::cast<acir::ac::ModuleOp>(operation)
                                .getSource()
                                .getImplementation()
                          : operation.getAttrOfType<mlir::StringAttr>(
                                "ac.source_file");
        if (!source || source != owner)
          return accError(
              "source AC unit contains a definition owned by another Python file");
      }
      if (!definitions)
        return accError("source AC unit must contain at least one definition");
    } else {
      return accError("non-core AC unit has unknown ac.unit_kind");
    }
    (*core)->getBody()->getOperations().splice(
        (*core)->getBody()->begin(), (*unit)->getBody()->getOperations());
  }
  std::map<std::string, acir::ac::ModuleOp> linkedDefinitions;
  for (acir::ac::ModuleOp definition : (*core)->getOps<acir::ac::ModuleOp>())
    linkedDefinitions.emplace(definition.getSymName().str(), definition);
  for (const std::string &symbol : sourceDefinitions) {
    auto definition = linkedDefinitions.find(symbol);
    if (definition == linkedDefinitions.end())
      return accError("source AC unit definition disappeared before linking");
    auto header = moduleHeaders.find(symbol);
    if (header == moduleHeaders.end())
      return accError("source AC unit requires one matching module interface "
                      "header for '" +
                      symbol + "'");
    auto actualSource = definition->second.getSource();
    if (header->second.schema != definition->second.getSchema())
      return accError("module interface header signature mismatch for '" +
                      symbol + "'");
    if (!actualSource || actualSource != header->second.source)
      return accError("module interface header source mismatch for '" + symbol +
                      "'");
  }
  for (const auto &[symbol, header] : moduleHeaders)
    if (!sourceDefinitions.contains(symbol))
      return accError("module interface header has no matching source "
                      "definition for '" +
                      symbol + "'");
  for (acir::ac::ModuleImportOp import : llvm::make_early_inc_range(
           (*core)->getOps<acir::ac::ModuleImportOp>())) {
    auto found = linkedDefinitions.find(import.getSymName().str());
    if (found == linkedDefinitions.end())
      return accError("unresolved module import '" + import.getSymName() + "'");
    acir::ac::ModuleOp definition = found->second;
    if (import.getSchema() != definition.getSchema())
      return accError("module import signature mismatch for '" +
                      import.getSymName() + "'");
    auto expectedSource = import.getSource();
    auto actualSource = definition.getSource();
    if (!expectedSource || expectedSource != actualSource)
      return accError("module import source mismatch for '" +
                      import.getSymName() + "'");
    import.erase();
  }
  if (mlir::failed(mlir::verify(**core)))
    return accError("linked AC package verification failed");
  return core;
}

llvm::Error lowerLinkedPackage(mlir::ModuleOp module) {
  mlir::PassManager manager(module.getContext());
  acir::addRuleLoweringPipeline(manager);
  manager.addPass(acir::createNormalizeACIRFilePass());
  manager.addPass(acir::createFreezeTopologyPass());
  if (mlir::failed(manager.run(module)))
    return accError("linked High ACIR lowering failed");
  if (mlir::failed(mlir::verify(module)))
    return accError("lowered AC package verification failed");
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
  {
    // `llvm::sys::fs::file_t` is a HANDLE on Windows, so close through the
    // stream wrapper instead of the POSIX-shaped closeFile overload.
    llvm::raw_fd_ostream reserve(verilogDescriptor, /*shouldClose=*/true);
    reserve.flush();
    if (reserve.has_error())
      return llvm::createStringError(reserve.error(),
                                     "cannot close Verilog staging file");
  }

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
  auto module = loadAcInput(options->input, context);
  if (!module)
    return fail(module.takeError());
  if (auto error = lowerLinkedPackage(**module))
    return fail(std::move(error));
  if (options->mode == EmitMode::Verify)
    return EXIT_SUCCESS;

  auto plan = acir::codegen::buildQueueGraphPlan(**module);
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

  auto bundle = acir::codegen::generateQueueGraphModelBundle(*plan);
  if (!bundle)
    return fail(bundle.takeError());
  if (auto error = writeBundleAtomically(options->output, *bundle))
    return fail(std::move(error));
  return EXIT_SUCCESS;
}
