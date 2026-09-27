#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTSIGNATURE_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTSIGNATURE_H

#include "PythonImportAST.h"

#include <optional>

namespace acir::compiler::detail {

struct ParameterSyntax {
  AstNode parameter;
  llvm::StringRef binding;
  std::optional<AstNode> defaultValue;
};

struct FunctionSignatureSyntax {
  llvm::SmallVector<ParameterSyntax> parameters;
  bool hasVararg = false;
  bool hasKwarg = false;
};

// Parses Python's positional/default and keyword-only/default alignment.
// Callers explicitly request implicit receiver handling; ordinary helpers
// retain every formal, including the first positional parameter.
mlir::FailureOr<FunctionSignatureSyntax>
parseFunctionSignature(const AstNode &arguments, bool hasImplicitReceiver,
                       ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTSIGNATURE_H
