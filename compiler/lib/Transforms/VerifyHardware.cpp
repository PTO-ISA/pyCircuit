#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir {
namespace {

class VerifyHardwarePass
    : public PassWrapper<VerifyHardwarePass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyHardwarePass)
  StringRef getArgument() const final { return "ac-verify-hardware"; }
  StringRef getDescription() const final {
    return "Verify the closed module/rule/instance/bits hardware graph";
  }
  void runOnOperation() final {
    if (failed(getAnalysis<ac::HardwareAnalysis>().verify()))
      signalPassFailure();
    else
      markAllAnalysesPreserved();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createVerifyHardwarePass() {
  return std::make_unique<VerifyHardwarePass>();
}
void registerACIRPasses() {
  PassRegistration<VerifyHardwarePass>();
  registerPass([] { return createExtractSourceInterfacePass(); });
  registerPass([] { return createSimplifyRecordWiresPass(); });
}

} // namespace acir
