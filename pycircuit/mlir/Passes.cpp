#include "Passes.h"
#include "mlir/Pass/PassRegistry.h"
void acir::registerACIRPasses() {
  mlir::registerPass([] { return createTransactionPass(); });
  mlir::registerPass([] { return createRTLPass(); });
}
