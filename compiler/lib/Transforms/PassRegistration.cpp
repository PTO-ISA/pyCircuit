#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir {
// The source-stage transforms register here. The package-verification passes
// register through GFIRValidation/HWIRValidation (registerGFIRValidationPasses
// and registerHWIRValidationPasses, Dialect/ACIR/GFIRPackageValidation.h and
// HWIRPackageValidation.h); the conversion passes register through their own
// targets with pr/conversion.
void registerACIRPasses() {
  registerPass([] { return createRouteTransactionsPass(); });
  registerPass([] { return createNormalizeSourceControlFlowPass(); });
  registerPass([] { return createExtractSourceInterfacePass(); });
  registerPass([] { return createSimplifyRecordWiresPass(); });
  registerPass([] { return createAnalyzeSourceMethodSummariesPass(); });
  registerPass([] { return createAnalyzeTransactionStateSSAPass(); });
  registerPass([] { return createLowerTransactionsPass(); });
}
} // namespace acir
