#include "Passes.h"
#include "mlir/Pass/PassRegistry.h"
void acir::registerACIRPasses() {
  mlir::registerPass([] { return createTransactionPass(); });
  mlir::registerPass([] { return createRTLPass(); });
  mlir::registerPass([] { return createResourceExpansionPass(); });
  mlir::registerPass([] { return createConditionPass(); });
  mlir::PassPipelineRegistration<> cleanup(
      "acir-cleanup",
      "Fold redundant computations and merge common expressions",
      addCleanupPasses);
  mlir::PassPipelineRegistration<> source(
      "acir-source-opt", "Normalize names and simplify structured source IR",
      addSourcePasses);
}
