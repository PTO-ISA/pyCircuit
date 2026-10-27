#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/BuiltinOps.h"
namespace acir::ac {
// Presence selects this contract; malformed markers never select old semantics.
bool isTransactionalPackage(mlir::Operation *operation);
bool isLinkedTransactionalPackage(mlir::Operation *operation);
bool transactionalTypesEqual(mlir::Type lhs, mlir::Type rhs);
// Unlinked module-reference constraints and published Method signatures. The
// definition form is derived from actual Method regions, never a body summary.
bool isTransactionalReferenceRequirement(mlir::Operation *operation);
mlir::ArrayAttr getTransactionalMethodInterfaces(mlir::Operation *operation);
// Complete capture-time generic Method constraints from actual pure call
// closures. Only locally owned requirements may be strengthened.
mlir::LogicalResult inferTransactionalReferenceRequirements(mlir::ModuleOp package);
mlir::LogicalResult verifyTransactionalReferenceBinding(
    mlir::Operation *requirement, mlir::Operation *provider,
    mlir::Operation *site);
// V2 verifies normalized resources, instances and callables. It is not execution
// admission or backend support.
mlir::LogicalResult verifyTransactionalPackage(mlir::ModuleOp package);
mlir::LogicalResult verifyTransactionalOperation(mlir::Operation *operation);
mlir::LogicalResult verifyTransactionalRegions(mlir::Operation *operation);
} // namespace acir::ac
#endif
