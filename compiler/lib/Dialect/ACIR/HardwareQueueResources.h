#ifndef PYCIRCUIT_HARDWARE_QUEUE_RESOURCES_H
#define PYCIRCUIT_HARDWARE_QUEUE_RESOURCES_H

#include "HardwareSampledAdmission.h"
#include "llvm/ADT/DenseSet.h"

namespace acir::ac::detail {
/// Parse-free physical routing closure. This proves only sanctioned transport
/// edges; full resource, admission-prefix and source-inventory proofs remain
/// separate. In particular this API never calls parseSampledAdmission.
mlir::FailureOr<llvm::DenseSet<mlir::OpOperand *>>
getVerifiedQueueResourceUses(const HardwareAnalysis &analysis,
                             SourceCheckBudget &budget);
mlir::FailureOr<llvm::DenseSet<mlir::OpOperand *>>
getVerifiedQueueResourceUses(const HardwareAnalysis &analysis);
mlir::LogicalResult verifyQueueResourceWiring(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    SourceCheckBudget &budget);
}
#endif
