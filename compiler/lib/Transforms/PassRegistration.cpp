#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir {
// The source-stage transforms and the package-verification passes register
// here. The verification pass factories stay registered at this stack state:
// their GFIRValidation/HWIRValidation target split and the conversion passes
// arrive with pr/validation and pr/conversion.
void registerACIRPasses() {
  registerPass([] { return createRouteTransactionsPass(); });
  registerPass([] { return createNormalizeSourceControlFlowPass(); });
  registerPass([] { return createExtractSourceInterfacePass(); });
  registerPass([] { return createSimplifyRecordWiresPass(); });
  registerPass([] { return createAnalyzeSourceMethodSummariesPass(); });
  registerPass([] { return createAnalyzeTransactionStateSSAPass(); });
  registerPass([] { return createLowerTransactionsPass(); });
  registerPass([] { return createVerifyRoutedPass(); });
  registerPass([] { return createVerifyHardwarePass(); });
}
} // namespace acir
