#ifndef PYCIRCUIT_DIALECT_HWIR_HWIRVERIFICATION_H
#define PYCIRCUIT_DIALECT_HWIR_HWIRVERIFICATION_H

#include "mlir/IR/BuiltinOps.h"

#include <memory>

namespace mlir {
class Pass;
} // namespace mlir

namespace acir::hwir {

/// Verify a complete HWIR package: contract version and placement, package
/// closure, resolved definitions, connected operands, admitted operations and
/// the selected system root. Operation verifiers already checked local
/// structure; this entry point owns the cross-operation contract.
mlir::LogicalResult verifyHWIRPackage(mlir::ModuleOp package);

/// Read-only verification pass registered as `--hwir-verify`. It never
/// rewrites the package and only runs the package verifier above.
std::unique_ptr<mlir::Pass> createHWIRVerifyPass();

void registerHWIRPasses();

} // namespace acir::hwir

#endif // PYCIRCUIT_DIALECT_HWIR_HWIRVERIFICATION_H
