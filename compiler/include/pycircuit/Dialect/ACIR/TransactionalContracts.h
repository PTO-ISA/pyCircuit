#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
namespace acir::ac {
// Presence selects this contract; malformed markers never select old semantics.
bool isTransactionalPackage(mlir::Operation *operation);
bool isLinkedTransactionalPackage(mlir::Operation *operation);
bool transactionalTypesEqual(mlir::Type lhs, mlir::Type rhs);
// V2 verifies resource/instance structure only. It is not execution admission.
mlir::LogicalResult verifyTransactionalPackage(mlir::ModuleOp package);
// The payload-closure walk over the same package (B2 step-2 design r3 row):
// the enum/struct/table and symbolic-Bits payload widths under an empty
// binding context, run by the shared packed-width core from this dialect
// target. The native verifyTransactionalPackage tail calls it directly; it
// remains the staged linked-package acceptance entry.
mlir::LogicalResult verifyTransactionalLinkedPackage(mlir::ModuleOp package);
mlir::LogicalResult verifyTransactionalOperation(mlir::Operation *operation);
mlir::LogicalResult verifyTransactionalRegions(mlir::Operation *operation);
} // namespace acir::ac
#endif
