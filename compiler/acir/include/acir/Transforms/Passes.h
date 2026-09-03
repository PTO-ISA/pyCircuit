#ifndef ACIR_TRANSFORMS_PASSES_H
#define ACIR_TRANSFORMS_PASSES_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Support/LogicalResult.h"

#include <memory>

namespace acir {

std::unique_ptr<mlir::Pass> createNormalizeACIRFilePass();
std::unique_ptr<mlir::Pass> createVerifyACIRFilePass();
std::unique_ptr<mlir::Pass> createLowerProcessStatePass();

#define GEN_PASS_DECL_VERIFYMODELPASS
#define GEN_PASS_DECL_LOWERPROCESSSTATEPASS
#define GEN_PASS_DECL_CANONICALIZEMODELPASS
#define GEN_PASS_DECL_FREEZETOPOLOGYPASS
#include "acir/Transforms/Passes.h.inc"

/// Shared implementation used by ac-canonicalize-model and the atomic freeze
/// pass. It is idempotent and never depends on host pointer order.
mlir::LogicalResult canonicalizeModel(mlir::ModuleOp model);

#define GEN_PASS_REGISTRATION
#include "acir/Transforms/Passes.h.inc"

} // namespace acir

#endif // ACIR_TRANSFORMS_PASSES_H
