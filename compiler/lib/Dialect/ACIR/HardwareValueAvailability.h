#ifndef PYCIRCUIT_HARDWARE_VALUE_AVAILABILITY_H
#define PYCIRCUIT_HARDWARE_VALUE_AVAILABILITY_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseSet.h"

namespace acir::ac::detail {
// Prove that selecting a row cannot change the existing availability plane.
// This follows exact capture ordinals and uniform interface/storage producers;
// row-dependent constructors/maps keep the selected-row proof path.
mlir::FailureOr<bool> hasUniformTableAvailability(
    mlir::Value value, const HardwareAnalysis &analysis,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit = {});

// Re-derive availability from payload SSA and physical instance wiring. Source
// replay separately authenticates which reached source uses require admission.
mlir::LogicalResult
verifyHardwareValueAvailability(
    const HardwareAnalysis &analysis,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit = {});
// These edges transport availability exclusively to checked companion ports
// or require operands. A shared data use disqualifies the complete route.
mlir::FailureOr<llvm::DenseSet<mlir::OpOperand *>>
getVerifiedValueAvailabilityUses(const HardwareAnalysis &analysis);
} // namespace acir::ac::detail
#endif
