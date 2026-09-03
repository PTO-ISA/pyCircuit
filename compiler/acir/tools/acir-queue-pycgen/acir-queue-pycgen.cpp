#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/CodeGen/QueueGraphPyc.h"
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
      argc, argv, "Generate canonical PYC IR from frozen Queue ACIR\n");
  mlir::DialectRegistry registry;
  acir::registerAllDialects(registry);
  mlir::MLIRContext context(registry);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(inputFile, &context);
  if (!module) {
    llvm::errs() << "ACLOWER-PYC: frozen ACIR parsing failed\n";
    return EXIT_FAILURE;
  }
  auto plan = acir::codegen::buildQueueGraphPlan(*module);
  if (!plan) {
    llvm::errs() << llvm::toString(plan.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  auto pyc = acir::codegen::generateQueueGraphPyc(*plan);
  if (!pyc) {
    llvm::errs() << llvm::toString(pyc.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  llvm::outs() << *pyc;
  return EXIT_SUCCESS;
}
