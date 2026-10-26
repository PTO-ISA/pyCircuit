#ifndef PYCIRCUIT_SOURCE_ENTRY_BOUNDARY_H
#define PYCIRCUIT_SOURCE_ENTRY_BOUNDARY_H
#include "mlir/IR/BuiltinOps.h"

namespace acir::compiler {
/// Normalize only an applicable authenticated source-link package. Success
/// with an empty OwningOpRef means no change: the caller must retain the exact
/// original operations. An applicable result is a detached, independently
/// proved candidate; failure never mutates the input.
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
normalizeEntryBoundary(mlir::ModuleOp original);
mlir::LogicalResult verifyEntryBoundaryNormalization(mlir::ModuleOp original,
                                                     mlir::ModuleOp candidate);
}
#endif
