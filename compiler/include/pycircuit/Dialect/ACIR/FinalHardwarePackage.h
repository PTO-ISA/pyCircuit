#ifndef PYCIRCUIT_DIALECT_ACIR_FINALHARDWAREPACKAGE_H
#define PYCIRCUIT_DIALECT_ACIR_FINALHARDWAREPACKAGE_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/StringRef.h"

namespace acir::ac {

/// Final-package admission certificate published by the canonical hardware
/// lowering boundary.  It means that the merged package has passed hardware
/// verification and contains no source transaction semantics that a backend
/// would have to interpret.
///
/// The canonical producer is ac-lower-transactions.  Consumers must still run
/// the normal final-package verifier: this marker records the completed
/// pipeline boundary; it is not a substitute for validation of serialized IR.
inline constexpr llvm::StringLiteral HardwareOnlyAttrName =
    "hwir.hardware_only";

inline bool isHardwareOnlyPackage(mlir::ModuleOp package) {
  return package && static_cast<bool>(package->getAttrOfType<mlir::UnitAttr>(
                        HardwareOnlyAttrName));
}

inline void markHardwareOnlyPackage(mlir::ModuleOp package) {
  package->setAttr(HardwareOnlyAttrName,
                   mlir::UnitAttr::get(package.getContext()));
}

} // namespace acir::ac

#endif // PYCIRCUIT_DIALECT_ACIR_FINALHARDWAREPACKAGE_H
