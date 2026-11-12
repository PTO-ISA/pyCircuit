#include "pycircuit/Conversion/GFIRToHWIR.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir {
void registerGFIRToHWIRPasses() {
  registerPass([] { return createLowerTransactionsPass(); });
}
} // namespace acir
