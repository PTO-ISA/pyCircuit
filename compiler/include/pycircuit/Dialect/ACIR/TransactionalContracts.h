#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALCONTRACTS_H
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include <functional>
#include <optional>
namespace acir::ac {
// Presence selects this contract; malformed markers never select old semantics.
bool isTransactionalPackage(mlir::Operation *operation);
bool isLinkedTransactionalPackage(mlir::Operation *operation);
bool transactionalTypesEqual(mlir::Type lhs, mlir::Type rhs);
// Read-only reachability proof for closed payload conditions. Unknown values
// retain control flow; this never replaces payload/type validation.
std::optional<bool> evaluateTransactionalPredicate(
    mlir::Value value, const std::function<bool()> &debit);
// Unlinked module-reference constraints and published Method signatures. The
// definition form is derived from actual Method regions, never a body summary.
bool isTransactionalReferenceRequirement(mlir::Operation *operation);
mlir::ArrayAttr getTransactionalMethodInterfaces(mlir::Operation *operation);
// Complete capture-time generic Method constraints from actual pure call
// closures. Only locally owned requirements may be strengthened.
mlir::LogicalResult
inferTransactionalReferenceRequirements(mlir::ModuleOp package);
mlir::LogicalResult
verifyTransactionalReferenceBinding(mlir::Operation *requirement,
                                    mlir::Operation *provider,
                                    mlir::Operation *site);
mlir::LogicalResult
resolveTransactionalMethodConstraints(mlir::ModuleOp package,
                                      bool requireClosed = false);
// Solve only from SSA constraints. Unbound provider facts may remain in an
// unlinked generic; executable closure requires every payload to be solved.
mlir::LogicalResult
solveTransactionalPayloadConstraints(mlir::ModuleOp package,
                                     bool requireClosed = false);
// Specialize Rule registrations and close resource identities after instance
// occurrences are bound.
// Modules are in root-first occurrence order; input structure is verified by
// the linking boundary. Allocates default queues and rewrites bound interfaces.
mlir::LogicalResult resolveTransactionalResourceConnections(
    mlir::ModuleOp package, llvm::ArrayRef<ModuleOp> modules);
// V2 verifies normalized resources, instances and callables. It is not
// execution admission or backend support.
mlir::LogicalResult verifyTransactionalPackage(mlir::ModuleOp package);
mlir::LogicalResult verifyTransactionalOperation(mlir::Operation *operation);
mlir::LogicalResult verifyTransactionalRegions(mlir::Operation *operation);
} // namespace acir::ac
#endif
