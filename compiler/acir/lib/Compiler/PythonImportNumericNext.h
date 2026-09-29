#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTNUMERICNEXT_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTNUMERICNEXT_H

#include "PythonImportRules.h"
#include <optional>

namespace acir::compiler::detail {
struct PythonNumericNextPlan {
  AstNode assignment, current, increment, add, mask, result;
  std::string name;
  mlir::DictionaryAttr domain;
};
mlir::FailureOr<std::optional<PythonNumericNextPlan>>
analyzePythonNumericNext(const RulePlan &rule, const ModuleModel &module,
                         const AstNode &method, ac::detail::EmitError error);
mlir::FailureOr<llvm::SmallVector<mlir::Value, 2>>
emitPythonNumericNext(const PythonNumericNextPlan &plan, mlir::Value current,
                      mlir::DictionaryAttr state, mlir::Operation *rule,
                      mlir::FlatSymbolRefAttr symbol,
                      const AstNode &moduleDeclaration, llvm::StringRef path,
                      mlir::OpBuilder &builder);
} // namespace acir::compiler::detail
#endif
