#pragma once
#include "mlir/Pass/Pass.h"
namespace mlir {
class OpPassManager;
}
namespace acir {
int compileMain(int argc, char **argv);
std::unique_ptr<mlir::Pass> createTransactionPass();
std::unique_ptr<mlir::Pass> createRTLPass();
std::unique_ptr<mlir::Pass> createResourceExpansionPass();
std::unique_ptr<mlir::Pass> createConditionPass();
std::unique_ptr<mlir::Pass> createFieldProjectionPass();
void addCleanupPasses(mlir::OpPassManager &pm);
void addSourcePasses(mlir::OpPassManager &pm);
void registerACIRPasses();
} // namespace acir
