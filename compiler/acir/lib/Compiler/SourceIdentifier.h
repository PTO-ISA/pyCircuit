#ifndef ACIR_LIB_COMPILER_SOURCEIDENTIFIER_H
#define ACIR_LIB_COMPILER_SOURCEIDENTIFIER_H

#include "SourceUnit.h"

namespace acir::compiler::detail {

// Verifies one already-captured Python AST identifier. bindingName applies
// the additional restrictions Python applies when the name is bound.
mlir::LogicalResult verifyPythonAstIdentifier(
    llvm::StringRef name, bool bindingName, ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_SOURCEIDENTIFIER_H
