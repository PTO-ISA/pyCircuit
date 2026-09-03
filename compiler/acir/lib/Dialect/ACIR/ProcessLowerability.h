#ifndef ACIR_DIALECT_ACIR_PROCESSLOWERABILITY_H
#define ACIR_DIALECT_ACIR_PROCESSLOWERABILITY_H

#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/STLFunctionalExtras.h"

#include <cstdint>

namespace acir::ac {

struct RawModelStructureLimits {
  uint64_t maxNodes = 1U << 20;
  uint64_t maxEdges = 1U << 22;
  uint64_t maxNestedRegionDepth = 512;
};

mlir::LogicalResult preflightRawModelStructure(
    mlir::ModuleOp module,
    const RawModelStructureLimits &limits = RawModelStructureLimits());

mlir::LogicalResult walkStructuredOperationsIterative(
    mlir::Operation *root,
    llvm::function_ref<mlir::LogicalResult(mlir::Operation *)> visitor,
    const RawModelStructureLimits &limits = RawModelStructureLimits());

struct StaticForTripCount {
  int64_t lowerBound = 0;
  int64_t upperBound = 0;
  int64_t step = 0;
  uint64_t tripCount = 0;
};

mlir::FailureOr<StaticForTripCount> analyzeStaticFor(mlir::scf::ForOp op);

mlir::LogicalResult verifyProcessLowerability(
    mlir::Operation *processLikeOp,
    const RawModelStructureLimits &limits = RawModelStructureLimits());

} // namespace acir::ac

#endif // ACIR_DIALECT_ACIR_PROCESSLOWERABILITY_H
