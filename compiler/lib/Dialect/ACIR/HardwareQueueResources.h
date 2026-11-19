#ifndef PYCIRCUIT_HARDWARE_QUEUE_RESOURCES_H
#define PYCIRCUIT_HARDWARE_QUEUE_RESOURCES_H

#include "HardwareSampledAdmission.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace acir::ac::detail {
/// A bare Queue predicate can replace the path select only at a root access
/// with a unique same-owner proposal whose retained raw path is literal one.
mlir::FailureOr<mlir::Value> proveUnconditionalQueuePath(
    const SampledAdmissionView &view, const SampledResourceView &resource,
    llvm::function_ref<mlir::FailureOr<bool>(mlir::Value)> isOne,
    SourceCheckBudget &budget);

/// Parse-free physical routing closure. This proves only sanctioned transport
/// edges; full resource, admission-prefix and source-inventory proofs remain
/// separate. In particular this API never calls parseSampledAdmission.
mlir::FailureOr<llvm::DenseSet<mlir::OpOperand *>>
getVerifiedQueueResourceUses(const HardwareAnalysis &analysis,
                             SourceCheckBudget &budget);
mlir::FailureOr<llvm::DenseSet<mlir::OpOperand *>>
getVerifiedQueueResourceUses(const HardwareAnalysis &analysis);
// Parse-free coverage of private Memory path/data pairs by actual pin SSA.
mlir::FailureOr<uint64_t> getMemoryIncomingPortCount(
    ModuleOp module, const HardwareAnalysis &analysis,
    const HardwareBindings &bindings, SourceCheckBudget &budget);
mlir::LogicalResult verifyQueueResourceWiring(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    SourceCheckBudget &budget);
}
#endif
