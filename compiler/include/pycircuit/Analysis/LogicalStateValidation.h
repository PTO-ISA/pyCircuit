#ifndef PYCIRCUIT_DIALECT_ACIR_LOGICALSTATEVALIDATION_H
#define PYCIRCUIT_DIALECT_ACIR_LOGICALSTATEVALIDATION_H

#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

namespace mlir {
class Operation;
} // namespace mlir

namespace llvm {
class StringRef;
} // namespace llvm

namespace acir::ac {

class HardwareAnalysis;
class SourceStateOp;
struct HardwareBindings;

namespace detail {
class SourceCheckBudget;
} // namespace detail

/// Bindings of the owning module of an operation, as used by the state
/// control checks. Single semantic implementation; previously duplicated as
/// the file-local `scope` helper of ACIRTransactionOps.cpp.
HardwareBindings stateOwnerScope(mlir::Operation *operation);

/// Require a width-one Bits control value under resolved bindings.
/// Single semantic implementation of the previous file-local `predicate`.
mlir::LogicalResult stateControlPredicate(const HardwareAnalysis &analysis,
                                          mlir::Value value,
                                          const HardwareBindings &bindings,
                                          mlir::Operation *site,
                                          llvm::StringRef description);

/// Single declarative update bound to the state's Q: iterate the actual Q
/// users, charge the budget per examined user, require exactly one update.
/// A Q used in both the state and candidate positions of one update counts
/// once. Shared admission analysis consumes this through the common
/// interface so it does not depend on a concrete operation class.
mlir::FailureOr<acir::LogicalStateUpdateOpInterface>
admissionPairLogicalStateUpdate(acir::LogicalStateOpInterface state,
                                detail::SourceCheckBudget &budget);

/// Local same-block pairing of one routed state declaration and its single
/// update, without bindings: the GFIR native verifier proves it locally and
/// the package declaration binding re-establishes it. A Q used in both the
/// state and candidate positions of one update counts once. Pass a budget
/// to charge one unit per examined Q user when running inside the bounded
/// source-check plan; native verification passes none.
mlir::FailureOr<acir::LogicalStateUpdateOpInterface>
verifySameBlockStateUpdatePairing(acir::LogicalStateOpInterface state,
                                  detail::SourceCheckBudget *budget = nullptr);

/// Full routed-bound verification of one logical-state declaration:
/// structural contract, width-one clock/reset controls, initial/Q payload
/// equivalence, optional payload constraint, the routed exactly-one-update
/// pairing and the closed static-initializer DAG. Pass a shared immutable
/// analysis and bindings when the package traversal already owns one.
/// Pass a budget to charge the Q-user pairing scan and the initializer DAG
/// traversal; native verification passes none.
/// This is bound verification: parsing or mlir::verify alone never
/// authorizes publication, refinement or execution (see B1-boundary.md).
mlir::LogicalResult verifyLogicalStateDeclarationBinding(
    acir::LogicalStateOpInterface state,
    const HardwareAnalysis *analysis = nullptr,
    const HardwareBindings *bindings = nullptr,
    detail::SourceCheckBudget *budget = nullptr);

/// Source-side bound verification of one source ac.state declaration:
/// the shared declaration-bound sequence plus the closed static
/// initializer. The routed pairing belongs to the GFIR declaration
/// binding above (B1-boundary.md).
mlir::LogicalResult verifyLogicalStateSourceBinding(SourceStateOp state);

/// Full routed-bound verification of one update binding: structural
/// contract, direct module-graph placement in a versioned routed package,
/// empty admitted attribute shape, Q defined by the same-block routed state
/// declaration, candidate payload equivalence and width-one write intent.
/// Pass a shared immutable analysis and bindings when the package traversal
/// already owns one; null constructs them from the package exactly as the
/// previous per-op verification did.
mlir::LogicalResult verifyLogicalStateUpdateBinding(
    acir::LogicalStateUpdateOpInterface update,
    const HardwareAnalysis *analysis = nullptr,
    const HardwareBindings *bindings = nullptr);

} // namespace acir::ac

#endif // PYCIRCUIT_DIALECT_ACIR_LOGICALSTATEVALIDATION_H
