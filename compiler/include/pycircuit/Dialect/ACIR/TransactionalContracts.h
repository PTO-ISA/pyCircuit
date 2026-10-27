#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/BuiltinOps.h"
namespace acir::ac {
// Presence selects this contract; malformed markers never select old semantics.
bool isTransactionalPackage(mlir::Operation *operation);
bool isLinkedTransactionalPackage(mlir::Operation *operation);
bool transactionalTypesEqual(mlir::Type lhs, mlir::Type rhs);
// V2 verifies normalized resources, instances and callables. It is not execution
// admission or backend support.
mlir::LogicalResult verifyTransactionalPackage(mlir::ModuleOp package);
mlir::LogicalResult verifyTransactionalOperation(mlir::Operation *operation);
mlir::LogicalResult verifyTransactionalRegions(mlir::Operation *operation);
} // namespace acir::ac
#endif
