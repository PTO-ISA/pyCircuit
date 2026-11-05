#ifndef ACIR_TRANSFORMS_PASSES_H
#define ACIR_TRANSFORMS_PASSES_H
#include <memory>
namespace mlir {
class Pass;
class OpPassManager;
}
namespace acir {
std::unique_ptr<mlir::Pass> createVerifyACIRPass();
std::unique_ptr<mlir::Pass> createInlineHelpersPass();
std::unique_ptr<mlir::Pass> createLowerValueBoundariesPass();
std::unique_ptr<mlir::Pass> createLowerQueriesPass();
std::unique_ptr<mlir::Pass> createFinalizeSourcePass();
void buildACIRToSourcePipeline(mlir::OpPassManager &pipeline);
std::unique_ptr<mlir::Pass> createVerifyHardwarePass();
std::unique_ptr<mlir::Pass> createNormalizeSourceControlFlowPass();
std::unique_ptr<mlir::Pass> createExtractSourceInterfacePass();
std::unique_ptr<mlir::Pass> createSimplifyRecordWiresPass();
std::unique_ptr<mlir::Pass> createAnalyzeSourceMethodSummariesPass();
std::unique_ptr<mlir::Pass> createAnalyzeTransactionStateSSAPass();
std::unique_ptr<mlir::Pass> createRouteTransactionsPass();
std::unique_ptr<mlir::Pass> createVerifyRoutedPass();
std::unique_ptr<mlir::Pass> createLowerTransactionsPass();
void registerACIRPasses();
} // namespace acir
#endif
