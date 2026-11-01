#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/ValueUpdates.h"
#include "pycircuit/Transforms/Passes.h"
using namespace mlir;
namespace acir {
namespace {
class NormalizeValueUpdatesPass
    : public PassWrapper<NormalizeValueUpdatesPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(NormalizeValueUpdatesPass)
  StringRef getArgument() const final { return "ac-normalize-value-updates"; }
  StringRef getDescription() const final {
    return "Normalize immutable field and bit updates on a lowering copy";
  }
  void runOnOperation() final {
    if (failed(ac::normalizeValueUpdates(getOperation())))
      signalPassFailure();
  }
};
} // namespace
std::unique_ptr<mlir::Pass> createNormalizeValueUpdatesPass() {
  return std::make_unique<NormalizeValueUpdatesPass>();
}
} // namespace acir
