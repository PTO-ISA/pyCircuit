#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir {
// Integration entry for the five source-stage transforms. The conversion
// passes register through their own targets: registerACIRToGFIRPasses and
// registerGFIRToHWIRPasses (Conversion/ACIRToGFIR.h, Conversion/GFIRToHWIR.h),
// assembled by each tool.
void registerACIRPasses() {
  registerPass([] { return createNormalizeSourceControlFlowPass(); });
  registerPass([] { return createExtractSourceInterfacePass(); });
  registerPass([] { return createSimplifyRecordWiresPass(); });
  registerPass([] { return createAnalyzeSourceMethodSummariesPass(); });
  registerPass([] { return createAnalyzeTransactionStateSSAPass(); });
}
} // namespace acir
