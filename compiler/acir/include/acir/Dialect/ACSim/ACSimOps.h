#ifndef ACIR_DIALECT_ACSIM_ACSIMOPS_H
#define ACIR_DIALECT_ACSIM_ACSIMOPS_H

#include "acir/Dialect/ACSim/ACSimDialect.h"
#include "acir/Dialect/ACSim/ACSimTypes.h"

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include <cstdint>

#define GET_OP_CLASSES
#include "acir/Dialect/ACSim/ACSimOps.h.inc"

namespace acir::acsim {

inline constexpr uint64_t kMaxModelNodes = 1ULL << 20;
inline constexpr uint64_t kMaxModelEdges = 1ULL << 22;
inline constexpr uint64_t kMaxModelRegionDepth = 512;
inline constexpr uint64_t kMaxExpandedObjects = 1ULL << 20;
inline constexpr uint64_t kMaxAttributeElements = 1ULL << 20;
inline constexpr uint64_t kMaxAttributeStringBytes = 1ULL << 24;

/// Whole-file gate used by the canonical optimizer entrypoint. Files without
/// ACSim operations are ignored; files containing ACSim require exactly one
/// closed model.
mlir::LogicalResult verifyCanonicalACSimFile(mlir::ModuleOp module);

} // namespace acir::acsim

#endif // ACIR_DIALECT_ACSIM_ACSIMOPS_H
