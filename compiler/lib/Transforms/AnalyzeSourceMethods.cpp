#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;

namespace acir {
namespace {

class AnalyzeSourceMethodSummariesPass
    : public PassWrapper<AnalyzeSourceMethodSummariesPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AnalyzeSourceMethodSummariesPass)
  StringRef getArgument() const final {
    return "ac-analyze-source-method-summaries";
  }
  StringRef getDescription() const final {
    return "Verify typed Method call closure and transaction result totality";
  }
  void runOnOperation() final {
    if (!getAnalysis<ac::SourceMethodSummariesAnalysis>().isValid()) {
      signalPassFailure();
      return;
    }
    markAllAnalysesPreserved();
  }
};

} // namespace

std::unique_ptr<Pass> createAnalyzeSourceMethodSummariesPass() {
  return std::make_unique<AnalyzeSourceMethodSummariesPass>();
}

} // namespace acir
