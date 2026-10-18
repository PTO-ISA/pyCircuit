#ifndef ACIR_TRANSFORMS_PASSES_H
#define ACIR_TRANSFORMS_PASSES_H
#include <memory>
namespace mlir {
class Pass;
}
namespace acir {
std::unique_ptr<mlir::Pass> createVerifyHardwarePass();
std::unique_ptr<mlir::Pass> createNormalizeSourceControlFlowPass();
std::unique_ptr<mlir::Pass> createExtractSourceInterfacePass();
std::unique_ptr<mlir::Pass> createSimplifyRecordWiresPass();
std::unique_ptr<mlir::Pass> createAnalyzeSourceMethodSummariesPass();
std::unique_ptr<mlir::Pass> createAnalyzeTransactionStateSSAPass();
std::unique_ptr<mlir::Pass> createLowerTransactionsPass();
void registerACIRPasses();
} // namespace acir
#endif
