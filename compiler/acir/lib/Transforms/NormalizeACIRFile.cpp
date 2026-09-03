#include "acir/Transforms/Passes.h"

#include "Dialect/ACIR/ProcessLowerability.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRResources.h"

namespace acir {
namespace {

class NormalizeACIRFilePass final
    : public mlir::PassWrapper<NormalizeACIRFilePass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(NormalizeACIRFilePass)

  llvm::StringRef getArgument() const override { return "normalize-ac-file"; }
  llvm::StringRef getDescription() const override {
    return "Normalize deterministic Agentic Circuit declaration order";
  }

  void runOnOperation() override {
    mlir::ModuleOp module = getOperation();
    if (mlir::failed(ac::preflightRawModelStructure(module))) {
      signalPassFailure();
      return;
    }
    ac::normalizeAddressMaps(module);
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createNormalizeACIRFilePass() {
  return std::make_unique<NormalizeACIRFilePass>();
}

} // namespace acir
