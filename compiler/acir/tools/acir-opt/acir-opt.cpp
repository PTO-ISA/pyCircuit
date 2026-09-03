#include "BindingOptions.h"
#include "acir/CodeGen/EmitCxx.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/GraphRegion.h"
#include "acir/InitAllDialects.h"
#include "acir/InitAllPasses.h"
#include "acir/Transforms/ResolveBindings.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassInstrumentation.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

#include <exception>
#include <memory>
#include <string>

namespace {

enum class CanonicalScanResult { Canonical, GenericOperation, MalformedEscape };

CanonicalScanResult scanCanonicalAssembly(llvm::StringRef input) {
  mlir::MLIRContext stringContext;
  mlir::ScopedDiagnosticHandler suppressStringDiagnostics(
      &stringContext, [](mlir::Diagnostic &) { return mlir::success(); });
  for (size_t index = 0; index < input.size();) {
    if (input.substr(index).starts_with("//")) {
      index = input.find('\n', index);
      if (index == llvm::StringRef::npos)
        return CanonicalScanResult::Canonical;
      continue;
    }
    if (input.substr(index).starts_with("/*")) {
      unsigned depth = 1;
      index += 2;
      while (index < input.size() && depth) {
        if (input.substr(index).starts_with("/*")) {
          ++depth;
          index += 2;
        } else if (input.substr(index).starts_with("*/")) {
          --depth;
          index += 2;
        } else {
          ++index;
        }
      }
      continue;
    }
    if (input[index] != '"') {
      ++index;
      continue;
    }
    size_t start = index++;
    bool escaped = false;
    while (index < input.size()) {
      char value = input[index++];
      if (!escaped && value == '"')
        break;
      escaped = !escaped && value == '\\';
      if (value != '\\')
        escaped = false;
    }
    llvm::StringRef token = input.slice(start, index);
    size_t next = index;
    while (next < input.size() && llvm::isSpace(input[next]))
      ++next;
    if (next >= input.size() || input[next] != '(')
      continue;
    auto parsed = mlir::parseAttribute(token, &stringContext);
    auto spelling = mlir::dyn_cast_or_null<mlir::StringAttr>(parsed);
    if (!spelling)
      return CanonicalScanResult::MalformedEscape;
    llvm::StringRef value = spelling.getValue();
    if (value.starts_with("ac.") || value.starts_with("acsim."))
      return CanonicalScanResult::GenericOperation;
  }
  return CanonicalScanResult::Canonical;
}

#ifdef ACIR_INTERNAL_TEST_TOOL
llvm::cl::opt<unsigned> testRawDepth(
    "acir-test-raw-depth", llvm::cl::Hidden, llvm::cl::init(0),
    llvm::cl::desc("materialize hostile nested IR after shallow parsing"));
llvm::cl::opt<bool> testRawMalformed("acir-test-raw-malformed",
                                     llvm::cl::Hidden, llvm::cl::init(false));
llvm::cl::opt<bool> testPassTrace("acir-test-pass-trace", llvm::cl::Hidden,
                                  llvm::cl::init(false));

class MaterializeRawDepthPass final
    : public mlir::PassWrapper<MaterializeRawDepthPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  llvm::StringRef getArgument() const final {
    return "acir-test-materialize-raw-depth";
  }
  void runOnOperation() final {
    mlir::ModuleOp parent = getOperation();
    for (unsigned index = 0; index < testRawDepth; ++index) {
      auto nested = mlir::ModuleOp::create(parent.getLoc());
      parent.getBody()->push_back(nested);
      parent = nested;
    }
    if (testRawMalformed) {
      mlir::OperationState state(parent.getLoc(), "scf.yield");
      parent.getBody()->push_back(mlir::Operation::create(state));
    }
  }
};

class TestPassTrace final : public mlir::PassInstrumentation {
public:
  void runBeforePass(mlir::Pass *pass, mlir::Operation *) final {
    llvm::errs() << "enter:" << pass->getArgument() << '\n';
  }
  void runAfterPass(mlir::Pass *pass, mlir::Operation *) final {
    llvm::errs() << "complete:" << pass->getArgument() << '\n';
  }
  void runAfterPassFailed(mlir::Pass *pass, mlir::Operation *) final {
    llvm::errs() << "fail:" << pass->getArgument() << '\n';
  }
};
#endif

int runDriver(int argc, char **argv) {
  mlir::DialectRegistry registry;
  acir::registerAllDialects(registry);
#ifdef ACIR_INTERNAL_TEST_TOOL
  registry.addExtension(
      +[](mlir::MLIRContext *context, acir::ac::ACIRDialect *) {
        auto &providers = acir::ac::getStructuralProviderRegistry(context);
        for (llvm::StringRef name :
             {"A", "B", "Consumer", "Empty", "Ext", "Leaf", "Producer", "Top"})
          providers.registerExternal(name);
        providers.registerGenerator("Gen");
      });
#endif
  acir::registerAllPasses();

  auto [inputFilename, outputFilename] = mlir::registerAndParseCLIOptions(
      argc, argv, "Agentic Circuit optimizer driver\n", registry);
  auto bindingOptions = acir::opt::loadBindingCommandLineOptions();
  if (!bindingOptions) {
    llvm::errs() << "error: " << llvm::toString(bindingOptions.takeError())
                 << '\n';
    return EXIT_FAILURE;
  }
  auto loweringOptions = acir::opt::loadLoweringCommandLineOptions();
  if (!loweringOptions) {
    llvm::errs() << "error: " << llvm::toString(loweringOptions.takeError())
                 << '\n';
    return EXIT_FAILURE;
  }
  mlir::MlirOptMainConfig config =
      mlir::MlirOptMainConfig::createFromCLOptions();
  mlir::MlirOptMainConfig commandLineConfig = config;
  config.allowUnregisteredDialects(false)
      .useExplicitModule(true)
      .setPassPipelineSetupFn([commandLineConfig,
                               bindingOptions = std::move(*bindingOptions),
                               loweringOptions = std::move(*loweringOptions)](
                                  mlir::PassManager &passManager) {
#ifdef ACIR_INTERNAL_TEST_TOOL
        if (testPassTrace)
          passManager.addInstrumentation(std::make_unique<TestPassTrace>());
        if (testRawDepth)
          passManager.addPass(std::make_unique<MaterializeRawDepthPass>());
#endif
        passManager.addPass(acir::createNormalizeACIRFilePass());
        passManager.addPass(acir::createVerifyACIRFilePass());
        if (mlir::failed(commandLineConfig.setupPassPipeline(passManager)))
          return mlir::failure();
        if (bindingOptions)
          passManager.addPass(acir::createResolveBindingsPass(*bindingOptions));
        auto emitOptions = []() {
          acir::codegen::EmitCxxOptions options;
          options.outputDir = acir::codegen::emitCxxOutputDir();
          options.profile = acir::opt::selectedBindingProfile();
          options.toolchainTarget = acir::opt::selectedBindingTarget();
          if (options.profile.empty())
            options.profile = "fast";
          if (options.toolchainTarget.empty())
            options.toolchainTarget = "unspecified";
          return options;
        };
        if (loweringOptions) {
          // Atomic whole-model lowering publishes canonical ACSim, so the
          // trailing ACIR whole-model gate does not apply to its output.
          passManager.addPass(acir::createACIRToACSimPass(*loweringOptions));
          if (acir::codegen::emitCxxRequested())
            passManager.addPass(
                acir::codegen::createEmitCxxPass(emitOptions()));
          if (acir::codegen::checkCxxContractRequested())
            passManager.addPass(acir::codegen::createCheckCxxContractPass());
          return mlir::success();
        }
        // --acsim-check-cxx-contract without --ac-lower-to-acsim is check-only:
        // it reads DIR from --acsim-output-dir and must not try to emit from
        // frozen ACIR (or any non-ACSim input).
        if (acir::codegen::checkCxxContractRequested()) {
          passManager.addPass(acir::codegen::createCheckCxxContractPass());
          return mlir::success();
        }
        if (acir::codegen::emitCxxRequested()) {
          passManager.addPass(acir::codegen::createEmitCxxPass(emitOptions()));
          return mlir::success();
        }
        // The final whole-model gate makes a persisted freeze digest effective
        // across every user-supplied pipeline: any topology mutation after
        // ac-freeze-topology is diagnosed before output is committed.
        passManager.addPass(acir::createVerifyModelPass());
        return mlir::success();
      });

  std::string errorMessage;
  std::unique_ptr<llvm::MemoryBuffer> input =
      mlir::openInputFile(inputFilename, &errorMessage);
  if (!input) {
    llvm::errs() << errorMessage << '\n';
    return EXIT_FAILURE;
  }

#ifndef ACIR_INTERNAL_TEST_TOOL
  llvm::StringRef contents = input->getBuffer();
  bool isBytecode = contents.size() >= 4 &&
                    contents.take_front(4) == llvm::StringRef("ML\xefR", 4);
  if (!isBytecode) {
    switch (scanCanonicalAssembly(contents)) {
    case CanonicalScanResult::Canonical:
      break;
    case CanonicalScanResult::GenericOperation:
      llvm::errs()
          << "error: generic ACIR operation spelling is internal-only; "
             "use canonical ACIR assembly\n";
      return EXIT_FAILURE;
    case CanonicalScanResult::MalformedEscape:
      llvm::errs() << "error: malformed quoted operation name escape\n";
      return EXIT_FAILURE;
    }
  }
#endif

  std::unique_ptr<llvm::ToolOutputFile> output =
      mlir::openOutputFile(outputFilename, &errorMessage);
  if (!output) {
    llvm::errs() << errorMessage << '\n';
    return EXIT_FAILURE;
  }

  mlir::LogicalResult result =
      mlir::MlirOptMain(output->os(), std::move(input), registry, config);
  if (mlir::succeeded(result))
    output->keep();
  return mlir::asMainReturnCode(result);
}

} // namespace

int main(int argc, char **argv) {
  try {
    return runDriver(argc, argv);
  } catch (const std::exception &error) {
    llvm::errs() << "error: unhandled exception: " << error.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    llvm::errs() << "error: unhandled unknown exception\n";
    return EXIT_FAILURE;
  }
}
