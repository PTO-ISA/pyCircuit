#ifndef PYCIRCUIT_ENTRY_BOUNDARY_H
#define PYCIRCUIT_ENTRY_BOUNDARY_H

#include "HardwareSourceChecks.h"

namespace acir::ac::detail {
inline constexpr llvm::StringLiteral entryBoundarySymbol = "pycircuit.__link_entry__";
/// Attribute callbacks check only the closed grammar and placement. Semantic
/// binding is performed after the owning caller's bounded structural admission.
mlir::LogicalResult verifyEntryBoundaryAttribute(mlir::Operation *operation,
                                                mlir::Attribute attribute);
mlir::LogicalResult verifyEntryBoundary(
    const HardwareAnalysis &analysis, bool normalizedInput,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
mlir::FailureOr<bool> needsEntryBoundary(
    const HardwareAnalysis &analysis,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
}
#endif
