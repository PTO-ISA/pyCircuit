#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEIDENTIFIER_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEIDENTIFIER_H

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringRef.h"

namespace acir::ac::detail {

// Check a captured Python AST identifier. A binding has extra Python
// restrictions.
mlir::LogicalResult verifyPythonAstIdentifier(
    llvm::StringRef name, bool bindingName,
    llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

} // namespace acir::ac::detail

#endif // PYCIRCUIT_DIALECT_ACIR_SOURCEIDENTIFIER_H
