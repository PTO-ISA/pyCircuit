#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalPreparation.h"
#include "pycircuit/Transforms/Passes.h"

using namespace mlir;
namespace acir {
namespace {
class InlinePureHelpersPass
    : public PassWrapper<InlinePureHelpersPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(InlinePureHelpersPass)
  StringRef getArgument() const final { return "ac-inline-pure-helpers"; }
  StringRef getDescription() const final {
    return "Expand verified pure helpers in a Linked lowering copy";
  }
  void runOnOperation() final {
    uint64_t work = 0;
    ac::HardwareSourceCheckLimits limits;
    limits.maxWorkUnits = 16 * 1024 * 1024;
    auto debit = [&](uint64_t amount, Operation *site) -> LogicalResult {
      if (amount > limits.maxWorkUnits - work)
        return site->emitOpError("pure helper inlining work budget exceeded");
      work += amount;
      return success();
    };
    if (failed(ac::inlinePureHelpers(getOperation(), debit)))
      signalPassFailure();
  }
};
} // namespace
std::unique_ptr<mlir::Pass> createInlinePureHelpersPass() {
  return std::make_unique<InlinePureHelpersPass>();
}
} // namespace acir
