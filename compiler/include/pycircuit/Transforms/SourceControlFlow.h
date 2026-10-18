#ifndef PYCIRCUIT_TRANSFORMS_SOURCECONTROLFLOW_H
#define PYCIRCUIT_TRANSFORMS_SOURCECONTROLFLOW_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"

namespace acir {
/// Normalize transient source SCF before source verification/analysis. Failure
/// leaves the input unchanged. Success may replace all nested operations and
/// invalidate their handles; callers must finish using importer SSA maps first.
/// A package without transient control flow is left untouched.
mlir::LogicalResult normalizeSourceControlFlow(mlir::ModuleOp package);
} // namespace acir

#endif
