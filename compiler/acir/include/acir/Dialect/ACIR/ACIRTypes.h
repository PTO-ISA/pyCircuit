#ifndef ACIR_DIALECT_ACIR_ACIRTYPES_H
#define ACIR_DIALECT_ACIR_ACIRTYPES_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Types.h"
#include "mlir/Interfaces/DataLayoutInterfaces.h"

#include "acir/Dialect/ACIR/ACIRAttributes.h"

#define GET_TYPEDEF_CLASSES
#include "acir/Dialect/ACIR/ACIRTypes.h.inc"

namespace acir::ac {

/// Returns true when `type` is, or recursively contains, an interface-only
/// channel type.
bool containsChannelType(mlir::Type type);

/// Returns true when `type` is, or recursively contains, a Queue or Var
/// runtime-wrapper type. Immutable payload types cannot contain either wrapper.
bool containsQueueOrVarType(mlir::Type type);

/// Returns true for a closed, immutable payload value that may be carried by a
/// Queue or represented by a Var.
bool isImmutablePayloadType(mlir::Type type);

/// Returns true for the broader v0.1 transaction-level payload inventory. This
/// includes dynamically sized lists and remains separate during the hard-break
/// migration.
bool isNormativePayloadType(mlir::Type type);

} // namespace acir::ac

#endif // ACIR_DIALECT_ACIR_ACIRTYPES_H
