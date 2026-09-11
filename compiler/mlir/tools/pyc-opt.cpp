#include "pyc/Dialect/PYC/PYCDialect.h"
#include "pyc/Transforms/Passes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/Extensions/InlinerExtension.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>

using namespace mlir;

namespace {

int runDriver(int argc, char **argv) {
  DialectRegistry registry;
  registry.insert<pyc::PYCDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect, mlir::scf::SCFDialect>();
  mlir::func::registerInlinerExtension(registry);
  registerAllPasses();

  auto [inputFilename, outputFilename] = mlir::registerAndParseCLIOptions(
      argc, argv, "pyCircuit optimizer driver\n", registry);
  mlir::MlirOptMainConfig config =
      mlir::MlirOptMainConfig::createFromCLOptions();

  std::string errorMessage;
  std::unique_ptr<llvm::MemoryBuffer> input =
      mlir::openInputFile(inputFilename, &errorMessage);
  if (!input) {
    llvm::errs() << errorMessage << '\n';
    return EXIT_FAILURE;
  }
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
