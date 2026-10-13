#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;

namespace acir {
namespace {

class AnalyzeTransactionStateSSAPass
    : public PassWrapper<AnalyzeTransactionStateSSAPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AnalyzeTransactionStateSSAPass)
  StringRef getArgument() const final {
    return "ac-analyze-transaction-state-ssa";
  }
  StringRef getDescription() const final {
    return "Analyze straight-line transaction-local state versions and guards";
  }
  void runOnOperation() final {
    if (!getAnalysis<ac::SourceTransactionStateAnalysis>().isValid()) {
      signalPassFailure();
      return;
    }
    markAllAnalysesPreserved();
  }
};

} // namespace

std::unique_ptr<Pass> createAnalyzeTransactionStateSSAPass() {
  return std::make_unique<AnalyzeTransactionStateSSAPass>();
}

} // namespace acir
