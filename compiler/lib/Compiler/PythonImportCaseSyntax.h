#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTCASESYNTAX_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTCASESYNTAX_H

#include "PythonImportAST.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace acir::compiler::detail {

/// Syntax only: atoms retain their original pattern sites, not typed keys.
struct CapturedMatchArmSyntax {
  AstNode originalSite;
  llvm::SmallVector<AstNode> atoms;
  bool catchAll = false;
};

using MatchSyntaxError =
    llvm::function_ref<mlir::InFlightDiagnostic(const AstNode &)>;

/// Consume a Match in a structurally verified capture. Subject/body ASTs remain
/// in the original nodes. Declaration identity, keys and coverage belong to the
/// importer and common semantic analysis, never this syntax view.
mlir::FailureOr<llvm::SmallVector<CapturedMatchArmSyntax>>
readMatchSyntax(const AstNode &statement, MatchSyntaxError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTCASESYNTAX_H
