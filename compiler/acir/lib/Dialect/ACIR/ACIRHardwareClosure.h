#ifndef ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSURE_H
#define ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSURE_H

#include "mlir/IR/BuiltinOps.h"

namespace acir::ac {

mlir::LogicalResult verifyFinalHardware(mlir::ModuleOp package);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSURE_H
