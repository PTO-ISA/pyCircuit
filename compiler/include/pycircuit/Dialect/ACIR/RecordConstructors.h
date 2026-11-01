#ifndef PYCIRCUIT_DIALECT_ACIR_RECORDCONSTRUCTORS_H
#define PYCIRCUIT_DIALECT_ACIR_RECORDCONSTRUCTORS_H
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
namespace acir::ac {
mlir::LogicalResult verifyRecordDefaults(StructOp declaration);
mlir::LogicalResult normalizeRecordConstructors(mlir::ModuleOp package);
} // namespace acir::ac
#endif
