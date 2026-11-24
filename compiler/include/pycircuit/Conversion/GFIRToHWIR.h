#ifndef PYCIRCUIT_CONVERSION_GFIRTOHWIR_H
#define PYCIRCUIT_CONVERSION_GFIRTOHWIR_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Support/LogicalResult.h"
#include <memory>

namespace mlir {
class Pass;
}

namespace acir::ac::detail {
class SourceCheckBudget;
}

namespace acir::detail {
/// Materialize a closed structured GFIR graph without changing the input.
/// The returned candidate has passed native and common HWIR verification.
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
materializeStructuredHardware(mlir::ModuleOp package);
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> materializeHardwareState(
    mlir::ModuleOp routed, ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyHardwareStateMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate,
    ac::detail::SourceCheckBudget &budget);
} // namespace acir::detail

namespace acir {
std::unique_ptr<mlir::Pass> createLowerStructuredGFIRPass();
std::unique_ptr<mlir::Pass> createLowerTransactionsPass();
void registerGFIRToHWIRPasses();
} // namespace acir
#endif
