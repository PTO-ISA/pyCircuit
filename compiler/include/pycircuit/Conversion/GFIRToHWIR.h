#ifndef PYCIRCUIT_CONVERSION_GFIRTOHWIR_H
#define PYCIRCUIT_CONVERSION_GFIRTOHWIR_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Support/LogicalResult.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <memory>

namespace mlir {
class Pass;
}

namespace acir::ac::detail {
class SourceCheckBudget;
}

namespace acir::ac {
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
materializeHardwareCDC(mlir::ModuleOp logical,
                       HardwareSourceCheckLimits limits = {});
mlir::LogicalResult
verifyHardwareCDCMaterialization(mlir::ModuleOp logical,
                                 mlir::ModuleOp expanded,
                                 HardwareSourceCheckLimits limits = {});
} // namespace acir::ac

namespace acir::detail {
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> materializeTransactionResetMasks(
    mlir::ModuleOp routed, ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyTransactionResetMaskMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate,
    ac::detail::SourceCheckBudget &budget);
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> materializeHardwareState(
    mlir::ModuleOp routed, ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyHardwareStateMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate,
    ac::detail::SourceCheckBudget &budget);
/// Shared reset normalization, CDC expansion and state refinement with each
/// independent correspondence proof, used by passes and saved-anchor reload.
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> refineRoutedHardware(
    mlir::ModuleOp routed, ac::HardwareSourceCheckLimits limits = {});
} // namespace acir::detail

namespace acir {
std::unique_ptr<mlir::Pass> createLowerTransactionsPass();
void registerGFIRToHWIRPasses();
} // namespace acir
#endif
