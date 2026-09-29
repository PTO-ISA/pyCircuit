#include "Compiler/FinalProgram.h"
#include "Compiler/ModuleGraph.h"
#include "Compiler/ScalarNumericLowering.h"
#include "Compiler/SourceLink.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

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

struct Options {
  std::vector<std::string> bodies;
  std::vector<std::string> headers;
  std::optional<std::string> top;
  std::optional<std::string> design;
  std::optional<std::string> target;
  std::optional<std::string> output;
  std::optional<std::string> glueOutput;
  std::optional<std::string> role;
};

// Which kind of root a linked or reparsed artifact selects. A @system is the
// externally driven harness that instantiates a design; a @module is the design
// itself. The framework must not let the two be conflated, so the tool requires
// the caller to state the role and checks it against the artifact.
struct RootKind {
  bool found = false;
  bool system = false;
};

RootKind selectedRootKind(mlir::ModuleOp container, llvm::StringRef top) {
  RootKind result;
  container.walk([&](mlir::Operation *op) {
    if (result.found || op->getName().getStringRef() != "ac.module")
      return;
    auto name = op->getAttrOfType<mlir::StringAttr>("sym_name");
    if (!name || name.getValue() != top)
      return;
    result.found = true;
    result.system =
        static_cast<bool>(op->getAttrOfType<mlir::StringAttr>("ac.root_kind"));
  });
  return result;
}

// Returns a diagnostic when the requested role does not match the artifact, and
// an empty string when it does.
std::string roleMismatch(const RootKind &root, llvm::StringRef top,
                         const std::optional<std::string> &role) {
  if (!root.found)
    return {};
  if (root.system && (!role || *role != "testbench"))
    return "the selected root '" + top.str() +
           "' is declared @system, which is the externally driven harness that "
           "instantiates a design; name it with --role testbench, or link a "
           "@module root to produce a design artifact";
  if (!root.system && role && *role == "testbench")
    return "the selected root '" + top.str() +
           "' is a @module design; --role testbench is only for a @system root";
  return {};
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
        arg != "--glue-output" && arg != "--role") {
      llvm::errs() << "error: unknown option '" << arg << "'\n";
      return false;
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
    } else if (arg == "--role") {
      if (value != "design" && value != "testbench") {
        llvm::errs() << "error: --role must be design or testbench\n";
        return false;
      }
      if (!setOnce(options.role, value, arg))
        return false;
    }
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
  for (auto [index, unit] : llvm::enumerate(units)) {
    auto kind = unit.body->getAttrOfType<mlir::StringAttr>("ac.unit_kind");
    if (kind && kind.getValue() == "implementation" &&
        mlir::failed(
            acir::compiler::lowerExactInputAddTransactional(unit.body)))
      return emitError() << "source unit[" << index
                         << "] exact-input numeric lowering failed";
    if (mlir::failed(mlir::verify(unit.body)) ||
        mlir::failed(mlir::verify(unit.header)))
      return emitError() << "source unit[" << index
                         << "] failed native verification";
  }

  // A @system root is an externally driven harness, not the design. Require the
  // caller to say which one it is producing so the two cannot be conflated.
  RootKind linkRoot;
  for (const mlir::OwningOpRef<mlir::ModuleOp> &body : ownedBodies) {
    RootKind candidate = selectedRootKind(*body, *options.top);
    if (candidate.found) {
      linkRoot = candidate;
      break;
    }
  }
  if (std::string mismatch = roleMismatch(linkRoot, *options.top, options.role);
      !mismatch.empty())
    return emitError() << mismatch;

  auto analysis = acir::compiler::buildFinalProgram(units, emitError);
  if (mlir::failed(analysis))
    return mlir::failure();
  const auto *root = analysis->modules().root;
  if (!root || !root->definition || root->definition.getValue() != *options.top)
    return emitError()
           << "--top does not match the selected source graph root '"
           << (root && root->definition ? root->definition.getValue()
                                        : llvm::StringRef("<missing>"))
           << "'";
  auto program =
      acir::compiler::materializeFinalProgram(std::move(*analysis), emitError);
  if (mlir::failed(program))
    return mlir::failure();
  // Reconstructing from a clone here guarantees only that the shared final IR
  // can be rebuilt by the emit path; it is not a promise that every backend can
  // emit every shape. Backend capability limits (for example a multi-value
  // observation) stay enforced by each emitter when that backend actually runs,
  // and link must not call the emitters to widen this guarantee.
  mlir::OwningOpRef<mlir::ModuleOp> emittedView(
      llvm::cast<mlir::ModuleOp>(program->hardware()->clone()));
  if (mlir::failed(acir::compiler::buildFinalProgramFromHardware(*emittedView,
                                                                 emitError)))
    return emitError() << "linked design is not reconstructible by the emit "
                          "path; this source shape is not supported yet";
  llvm::raw_string_ostream output(result);
  program->hardware().print(output, mlir::OpPrintingFlags().enableDebugInfo());
  output << '\n';
  return mlir::success();
}

mlir::LogicalResult runEmit(const Options &options, mlir::MLIRContext &context,
                            std::string &result, std::string &glueResult) {
  auto input = readModule(*options.design, context);
  if (!input) {
    llvm::errs() << "error: cannot parse final design '" << *options.design
                 << "'\n";
    return mlir::failure();
  }
  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  {
    // The artifact records which kind of root it selected, so the role is
    // checked again here rather than trusted from the link invocation. Any
    // ac.module carrying ac.root_kind is a @system root.
    RootKind emitRoot;
    std::string systemName;
    (*input).walk([&](mlir::Operation *op) {
      if (emitRoot.system || op->getName().getStringRef() != "ac.module")
        return;
      auto kind = op->getAttrOfType<mlir::StringAttr>("ac.root_kind");
      if (!kind)
        return;
      emitRoot.found = true;
      emitRoot.system = true;
      if (auto name = op->getAttrOfType<mlir::StringAttr>("sym_name"))
        systemName = name.getValue().str();
    });
    if (emitRoot.system) {
      if (std::string mismatch =
              roleMismatch(emitRoot, systemName, options.role);
          !mismatch.empty())
        return emitError() << mismatch;
    }
  }
  if (mlir::failed(mlir::verify(*input)))
    return emitError() << "final design failed native verification";
  auto program =
      acir::compiler::buildFinalProgramFromHardware(*input, emitError);
  if (mlir::failed(program))
    return mlir::failure();
  if (*options.target == "cpp") {
    auto emitted = acir::compiler::emitFinalCpp(*program, emitError);
    if (mlir::failed(emitted))
      return mlir::failure();
    result = std::move(*emitted);
    return mlir::success();
  }
  if (!options.glueOutput) {
    auto emitted = acir::compiler::emitFinalVerilog(*program, emitError);
    if (mlir::failed(emitted))
      return mlir::failure();
    result = std::move(*emitted);
    return mlir::success();
  }
  // Separate the two C3 generated-file roles: --output carries the hardware
  // `rtl` artifact and --glue-output carries the `runtime-glue` wrapper.
  auto parts = acir::compiler::emitFinalVerilogParts(*program, emitError);
  if (mlir::failed(parts))
    return mlir::failure();
  result = std::move(parts->rtl);
  glueResult = std::move(parts->runtimeGlue);
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
  mlir::LogicalResult status =
      options.design ? runEmit(options, context, result, glueResult)
                     : runLink(options, context, result);
  if (mlir::failed(status))
    return 1;
  if (options.glueOutput)
    return publishNoClobberPair(*options.output, result, *options.glueOutput,
                                glueResult)
               ? 0
               : 1;
  return publishNoClobber(*options.output, result) ? 0 : 1;
}
