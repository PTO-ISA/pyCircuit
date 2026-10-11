#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

// Private machine-readable status consumed by _native_verify.py. The public
// pycircuit CLI still reports its normal exit code and publication diagnostic.
constexpr int kSourceUnitOwnerMismatchExitCode = 3;

struct Options {
  std::vector<std::string> bodies;
  std::vector<std::string> headers;
  std::optional<std::string> top;
  std::optional<std::string> design;
  std::optional<std::string> target;
  std::optional<std::string> output;
  std::optional<std::string> glueOutput;
  std::optional<std::string> entryOwnerOutput;
  std::optional<std::string> unitOwnerOutput;
  bool verifyOnly = false;
};

// Minimal strict JSON string escaping for the private report channels. The
// values come from verified declarations, so the control-character branch is
// defensive rather than expected.
std::string jsonString(llvm::StringRef value) {
  static const char *hexDigits = "0123456789abcdef";
  std::string result = "\"";
  for (char character : value) {
    unsigned char byte = static_cast<unsigned char>(character);
    if (character == '"') {
      result += "\\\"";
    } else if (character == '\\') {
      result += "\\\\";
    } else if (character == '\n') {
      result += "\\n";
    } else if (character == '\r') {
      result += "\\r";
    } else if (character == '\t') {
      result += "\\t";
    } else if (byte < 0x20) {
      result += "\\u00";
      result += hexDigits[(byte >> 4) & 0xF];
      result += hexDigits[byte & 0xF];
    } else {
      result += character;
    }
  }
  result += "\"";
  return result;
}

bool setOnce(std::optional<std::string> &slot, llvm::StringRef value,
             llvm::StringRef option) {
  if (slot) {
    llvm::errs() << "error: duplicate " << option << '\n';
    return false;
  }
  slot = value.str();
  return true;
}

bool parseOptions(int argc, char **argv, Options &options) {
  for (int i = 1; i < argc; ++i) {
    llvm::StringRef arg(argv[i]);
    if (arg != "--body" && arg != "--header" && arg != "--top" &&
        arg != "--design" && arg != "--target" && arg != "--output" &&
        arg != "--glue-output" &&
        arg != "--entry-owner-out" && arg != "--unit-owner-out" &&
        arg != "--verify-only") {
      llvm::errs() << "error: unknown option '" << arg << "'\n";
      return false;
    }
    if (arg == "--verify-only") {
      if (options.verifyOnly) {
        llvm::errs() << "error: duplicate --verify-only\n";
        return false;
      }
      options.verifyOnly = true;
      continue;
    }
    if (++i == argc || llvm::StringRef(argv[i]).starts_with("--")) {
      llvm::errs() << "error: missing value for " << arg << '\n';
      return false;
    }
    llvm::StringRef value(argv[i]);
    if (arg == "--body")
      options.bodies.push_back(value.str());
    else if (arg == "--header")
      options.headers.push_back(value.str());
    else if (arg == "--top") {
      if (!setOnce(options.top, value, arg))
        return false;
    } else if (arg == "--design") {
      if (!setOnce(options.design, value, arg))
        return false;
    } else if (arg == "--target") {
      if (!setOnce(options.target, value, arg))
        return false;
    } else if (arg == "--output" && !setOnce(options.output, value, arg)) {
      return false;
    } else if (arg == "--glue-output" &&
               !setOnce(options.glueOutput, value, arg)) {
      return false;
    } else if (arg == "--entry-owner-out" &&
               !setOnce(options.entryOwnerOutput, value, arg)) {
      return false;
    } else if (arg == "--unit-owner-out" &&
               !setOnce(options.unitOwnerOutput, value, arg)) {
      return false;
    }
  }

  if (options.verifyOnly) {
    const bool programShape = options.design.has_value();
    const bool unitShape = !options.bodies.empty() || !options.headers.empty();
    if (programShape == unitShape) {
      llvm::errs()
          << "error: --verify-only requires either --design or exactly "
             "one --body/--header pair\n";
      return false;
    }
    if (options.target || options.output || options.glueOutput ||
        options.top) {
      llvm::errs() << "error: --verify-only takes no --target, --output, "
                      "--glue-output or --top\n";
      return false;
    }
    if (unitShape &&
        (options.bodies.size() != 1 || options.headers.size() != 1)) {
      llvm::errs()
          << "error: --verify-only verifies exactly one --body/--header "
             "pair\n";
      return false;
    }
    if (programShape && options.unitOwnerOutput) {
      llvm::errs() << "error: --unit-owner-out requires --verify-only with one "
                      "--body/--header pair\n";
      return false;
    }
    if (unitShape && options.entryOwnerOutput) {
      llvm::errs()
          << "error: --entry-owner-out reports a linked root owner and "
             "requires link or program verification\n";
      return false;
    }
    return true;
  }
  if (options.unitOwnerOutput) {
    llvm::errs() << "error: --unit-owner-out requires --verify-only with one "
                    "--body/--header pair\n";
    return false;
  }

  const bool hasLink = !options.bodies.empty() || !options.headers.empty() ||
                       options.top.has_value();
  const bool hasEmit = options.design.has_value();
  if (!options.target || !options.output || hasLink == hasEmit) {
    llvm::errs() << "error: select exactly one complete link or emit mode; "
                    "--target and --output are required\n";
    return false;
  }
  if (hasLink) {
    if (options.bodies.empty() ||
        options.bodies.size() != options.headers.size() || !options.top ||
        options.design || *options.target != "final") {
      llvm::errs() << "error: link mode requires matching nonempty --body and "
                      "--header pairs, --top, and --target final\n";
      return false;
    }
  } else if (!options.bodies.empty() || !options.headers.empty() ||
             options.top ||
             (*options.target != "cpp" && *options.target != "verilog")) {
    llvm::errs() << "error: emit mode requires --design and --target cpp or "
                    "verilog\n";
    return false;
  }
  if (options.entryOwnerOutput && !hasLink) {
    llvm::errs()
        << "error: --entry-owner-out reports the linked root owner and "
           "requires link mode\n";
    return false;
  }
  if (options.entryOwnerOutput &&
      *options.entryOwnerOutput == *options.output) {
    llvm::errs() << "error: --entry-owner-out must differ from --output\n";
    return false;
  }
  if (options.glueOutput &&
      (hasLink || !options.design || *options.target != "verilog")) {
    llvm::errs() << "error: --glue-output separates the Verilog runtime-glue "
                    "role and requires emit mode with --target verilog\n";
    return false;
  }
  if (options.glueOutput && *options.glueOutput == *options.output) {
    llvm::errs() << "error: --glue-output must differ from --output\n";
    return false;
  }
  return true;
}

mlir::OwningOpRef<mlir::ModuleOp> readModule(llvm::StringRef path,
                                             mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

// Render one verified source owner as the closed `{package, path}` object.
bool jsonOwnerObject(mlir::DictionaryAttr owner, std::string &out) {
  auto package = owner.getAs<mlir::StringAttr>("package");
  auto sourcePath = owner.getAs<mlir::StringAttr>("path");
  if (!package || !sourcePath)
    return false;
  out = "{\"package\":" + jsonString(package.getValue()) +
        ",\"path\":" + jsonString(sourcePath.getValue()) + "}";
  return true;
}

bool writeReport(llvm::StringRef path, llvm::StringRef text) {
  int descriptor = -1;
  std::error_code error = llvm::sys::fs::openFileForWrite(
      path, descriptor, llvm::sys::fs::CD_CreateNew, llvm::sys::fs::OF_None);
  if (error) {
    llvm::errs() << "error: cannot create report '" << path
                 << "': " << error.message() << '\n';
    return false;
  }
  llvm::raw_fd_ostream output(descriptor, true);
  output << text;
  output.flush();
  if (output.has_error()) {
    llvm::errs() << "error: failed writing report '" << path << "'\n";
    output.close();
    llvm::sys::fs::remove(path);
    return false;
  }
  output.close();
  return true;
}

// Report the linked root's source owner and canonical definition so the driver
// publishes the program under the owner the linker actually selected instead of
// inferring it from module names.
bool writeEntryOwner(llvm::StringRef path, mlir::DictionaryAttr owner,
                     llvm::StringRef definition) {
  std::string source;
  if (!jsonOwnerObject(owner, source)) {
    llvm::errs() << "error: selected root source owner is not a package/path "
                    "pair\n";
    return false;
  }
  std::string canonical = "@\"" + definition.str() + "\"";
  std::string text = "{\"source\":" + source +
                     ",\"definition\":" + jsonString(canonical) + "}\n";
  return writeReport(path, text);
}

// Report both internal owners of a verified source unit so the caller can
// compare each against the receipt and the expected publication owner.
bool writeUnitOwners(llvm::StringRef path, mlir::DictionaryAttr body,
                     mlir::DictionaryAttr header) {
  std::string bodyOwner;
  std::string headerOwner;
  if (!jsonOwnerObject(body, bodyOwner) ||
      !jsonOwnerObject(header, headerOwner)) {
    llvm::errs() << "error: unit body or header owner is not a package/path "
                    "pair\n";
    return false;
  }
  std::string text =
      "{\"body\":" + bodyOwner + ",\"header\":" + headerOwner + "}\n";
  return writeReport(path, text);
}

bool publishNoClobber(llvm::StringRef path, llvm::StringRef contents) {
  int descriptor = -1;
  std::error_code error = llvm::sys::fs::openFileForWrite(
      path, descriptor, llvm::sys::fs::CD_CreateNew, llvm::sys::fs::OF_None);
  if (error) {
    llvm::errs() << "error: cannot create output '" << path
                 << "' without replacing an existing path: " << error.message()
                 << '\n';
    return false;
  }
  llvm::raw_fd_ostream output(descriptor, true);
  output << contents;
  output.flush();
  if (output.has_error()) {
    llvm::errs() << "error: failed writing output '" << path << "'\n";
    output.close();
    llvm::sys::fs::remove(path);
    return false;
  }
  output.close();
  if (output.has_error()) {
    llvm::errs() << "error: failed closing output '" << path << "'\n";
    llvm::sys::fs::remove(path);
    return false;
  }
  return true;
}

// Publish the two role files of one Verilog emission. Both destinations are
// checked before either is created, and a failure on the second removes the
// first, so a rejected bundle never leaves a half-published artifact behind.
bool publishNoClobberPair(llvm::StringRef primaryPath, llvm::StringRef primary,
                          llvm::StringRef secondaryPath,
                          llvm::StringRef secondary) {
  if (llvm::sys::fs::exists(primaryPath)) {
    llvm::errs() << "error: cannot create output '" << primaryPath
                 << "' without replacing an existing path\n";
    return false;
  }
  if (llvm::sys::fs::exists(secondaryPath)) {
    llvm::errs() << "error: cannot create output '" << secondaryPath
                 << "' without replacing an existing path\n";
    return false;
  }
  if (!publishNoClobber(primaryPath, primary))
    return false;
  if (!publishNoClobber(secondaryPath, secondary)) {
    // Report a failed rollback instead of silently leaving the first role file
    // behind while claiming the bundle was rejected.
    if (std::error_code rollback = llvm::sys::fs::remove(primaryPath))
      llvm::errs() << "error: failed to roll back partially published output '"
                   << primaryPath << "': " << rollback.message() << '\n';
    return false;
  }
  return true;
}

mlir::LogicalResult runLink(const Options &options, mlir::MLIRContext &context,
                            std::string &result) {
  llvm::SmallVector<mlir::OwningOpRef<mlir::ModuleOp>> ownedBodies;
  llvm::SmallVector<mlir::OwningOpRef<mlir::ModuleOp>> ownedHeaders;
  llvm::SmallVector<acir::compiler::SourceLinkUnit> units;
  for (size_t i = 0; i < options.bodies.size(); ++i) {
    auto body = readModule(options.bodies[i], context);
    if (!body) {
      llvm::errs() << "error: cannot parse body file '" << options.bodies[i]
                   << "'\n";
      return mlir::failure();
    }
    auto header = readModule(options.headers[i], context);
    if (!header) {
      llvm::errs() << "error: cannot parse header file '" << options.headers[i]
                   << "'\n";
      return mlir::failure();
    }
    ownedBodies.push_back(std::move(body));
    ownedHeaders.push_back(std::move(header));
    units.push_back({*ownedBodies.back(), *ownedHeaders.back()});
  }

  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto program = acir::compiler::linkHardwareUnits(units, *options.top, emitError);
  if (mlir::failed(program))
    return mlir::failure();
  auto root = llvm::dyn_cast_or_null<acir::ac::ModuleOp>(
      mlir::SymbolTable::lookupSymbolIn(**program, *options.top));
  if (!root)
    return emitError() << "linked package has no selected root";
  if (options.entryOwnerOutput &&
      !writeEntryOwner(*options.entryOwnerOutput, root.getSourceOwner(), root.getSymName()))
    return mlir::failure();
  llvm::raw_string_ostream output(result);
  (*program)->print(output, mlir::OpPrintingFlags().enableDebugInfo());
  output << '\n';
  return mlir::success();
}

// Read-only verification of an already published artifact: a linked program or
// a source unit body/header pair. It emits no hardware, takes no lock, and
// reuses the same native verifier the link and emit paths use, so "valid" means
// the shared verifier accepted the artifact rather than that Python guessed.
mlir::LogicalResult runVerify(const Options &options,
                              mlir::MLIRContext &context,
                              bool &sourceUnitOwnerMismatch) {
  sourceUnitOwnerMismatch = false;
  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  if (options.design) {
    auto input = readModule(*options.design, context);
    if (!input) {
      llvm::errs() << "error: cannot parse artifact '" << *options.design
                   << "'\n";
      return mlir::failure();
    }
    if (mlir::failed(mlir::verify(*input)))
      return emitError() << "artifact failed native verification";
    if (mlir::failed(acir::ac::verifyHardwarePackage(*input)))
      return mlir::failure();
    auto systems = (*input).getOps<acir::ac::SystemOp>();
    auto entry = (*systems.begin()).getEntry().getAs<mlir::FlatSymbolRefAttr>("callee");
    auto root = llvm::dyn_cast_or_null<acir::ac::ModuleOp>(
        mlir::SymbolTable::lookupSymbolIn(*input, entry.getValue()));
    if (!root)
      return emitError() << "verified artifact has no selected module";
    if (options.entryOwnerOutput &&
        !writeEntryOwner(*options.entryOwnerOutput, root.getSourceOwner(), root.getSymName()))
      return mlir::failure();
    return mlir::success();
  }

  auto body = readModule(options.bodies.front(), context);
  if (!body) {
    llvm::errs() << "error: cannot parse body file '" << options.bodies.front()
                 << "'\n";
    return mlir::failure();
  }
  auto header = readModule(options.headers.front(), context);
  if (!header) {
    llvm::errs() << "error: cannot parse header file '"
                 << options.headers.front() << "'\n";
    return mlir::failure();
  }
  auto kind = (*body)->getAttrOfType<mlir::StringAttr>("ac.unit_kind");
  auto stage = (*body)->getAttrOfType<mlir::StringAttr>("ac.stage");
  if (!kind ||
      (kind.getValue() != "implementation" &&
       kind.getValue() != "declarations") ||
      !stage || stage.getValue() != "source")
    return emitError() << "unit body stage/unit_kind is invalid";
  if (mlir::failed(acir::compiler::verifyIntrinsicSourceUnitOwnerPair(
          *body, *header, sourceUnitOwnerMismatch, emitError)))
    return emitError() << "unit body/interface owner preflight failed";
  if (mlir::failed(mlir::verify(*body)))
    return emitError() << "unit body failed native verification";
  if (mlir::failed(mlir::verify(*header)))
    return emitError() << "unit header failed native verification";
  if (mlir::failed(acir::compiler::verifyIntrinsicSourceUnitPair(*body, *header,
                                                                 emitError)))
    return emitError() << "unit body/interface integrity verification failed";
  if (options.unitOwnerOutput) {
    auto bodyOwner =
        (*body)->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
    auto headerOwner =
        (*header)->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
    if (!bodyOwner || !headerOwner)
      return emitError() << "unit body or header records no source owner";
    if (!writeUnitOwners(*options.unitOwnerOutput, bodyOwner, headerOwner))
      return mlir::failure();
  }
  return mlir::success();
}

} // namespace

int main(int argc, char **argv) {
  Options options;
  if (!parseOptions(argc, argv, options))
    return 2;

  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();

  std::string result;
  std::string glueResult;
  bool sourceUnitOwnerMismatch = false;
  mlir::LogicalResult status =
      options.verifyOnly
          ? runVerify(options, context, sourceUnitOwnerMismatch)
          : runLink(options, context, result);
  if (mlir::failed(status) && sourceUnitOwnerMismatch)
    return kSourceUnitOwnerMismatchExitCode;
  if (mlir::failed(status))
    return 1;
  if (options.verifyOnly)
    return 0;
  if (options.glueOutput)
    return publishNoClobberPair(*options.output, result, *options.glueOutput,
                                glueResult)
               ? 0
               : 1;
  return publishNoClobber(*options.output, result) ? 0 : 1;
}
