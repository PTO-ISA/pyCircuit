#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/InitAllDialects.h"

#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <string>

namespace {

llvm::cl::opt<std::string> inputFile(llvm::cl::Positional, llvm::cl::Required,
                                     llvm::cl::desc("<frozen-acir>"));

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
  auto source = acir::codegen::generateQueueGraphCpp(*plan);
  if (!source) {
    llvm::errs() << llvm::toString(source.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  llvm::outs() << *source;
  return EXIT_SUCCESS;
}
