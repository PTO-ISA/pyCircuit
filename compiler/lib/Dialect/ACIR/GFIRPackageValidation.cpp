#include "pycircuit/Dialect/ACIR/GFIRPackageValidation.h"
#include "HardwareSourceCheckPreflight.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir::ac {
LogicalResult verifyRoutedPackage(mlir::ModuleOp package,
                                  HardwareSourceCheckLimits limits) {
  if (!package)
    return failure();
  HardwareAnalysis analysis(package);
  return success(succeeded(detail::buildSourceCheckPlan(
      analysis, limits, detail::PackageStagePolicy::routed())));
}
} // namespace acir::ac

namespace acir {
namespace {

class VerifyRoutedPass
    : public PassWrapper<VerifyRoutedPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyRoutedPass)
  StringRef getArgument() const final { return "ac-verify-routed"; }
  StringRef getDescription() const final {
    return "Verify the routed logical-state ACIR anchor without source objects";
  }
  void runOnOperation() final {
    if (failed(ac::verifyRoutedPackage(getOperation())))
      signalPassFailure();
    else
      markAllAnalysesPreserved();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createVerifyRoutedPass() {
  return std::make_unique<VerifyRoutedPass>();
}

void registerGFIRValidationPasses() {
  PassRegistration<VerifyRoutedPass>();
}
} // namespace acir
