#ifndef ACIR_DIALECT_ACSIM_ACSIMTYPES_H
#define ACIR_DIALECT_ACSIM_ACSIMTYPES_H

#include "acir/Dialect/ACSim/ACSimDialect.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Types.h"

#include <cstdint>

#define GET_TYPEDEF_CLASSES
#include "acir/Dialect/ACSim/ACSimTypes.h.inc"

namespace acir::acsim {

inline constexpr uint64_t kMaxArrayVolume = 1ULL << 20;

} // namespace acir::acsim

#endif // ACIR_DIALECT_ACSIM_ACSIMTYPES_H
