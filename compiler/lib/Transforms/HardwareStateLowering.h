#ifndef PYCIRCUIT_HARDWARE_STATE_LOWERING_H
#define PYCIRCUIT_HARDWARE_STATE_LOWERING_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"

namespace acir::ac {
class HardwareAnalysis;
namespace detail {
class SourceCheckBudget;
}
}

namespace acir::detail {
// Shared accounting and constant-enable proof used by physical refinement and
// the retained transaction materializer.
bool prepayOperationHeader(mlir::Operation *operation,
                          ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult prepayClone(mlir::Operation *root,
                               ac::detail::SourceCheckBudget &budget);
bool knownTrue(mlir::Value value, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget, unsigned depth = 0);

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> materializeHardwareState(
    mlir::ModuleOp routed, ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyHardwareStateMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate,
    ac::detail::SourceCheckBudget &budget);
} // namespace acir::detail
#endif
