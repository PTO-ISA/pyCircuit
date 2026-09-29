#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTNUMERIC_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTNUMERIC_H

#include "PythonImportRules.h"

#include <optional>

namespace acir::compiler::detail {

struct PythonImportNumericPlan {
  AstNode assignment;
  AstNode current;
  AstNode literal;
  AstNode result;
  std::string inputName;
  std::string targetName;
  std::string opcode;
  mlir::DictionaryAttr inputType;
  bool comparison = false;
};

mlir::FailureOr<std::optional<PythonImportNumericPlan>>
analyzePythonImportNumericRule(const RulePlan &rule, const ModuleModel &module,
                               const AstNode &method,
                               ac::detail::EmitError emitError);

void attachPythonImportNumericContract(const PythonImportNumericPlan &plan,
                                       mlir::Operation *rule,
                                       mlir::FlatSymbolRefAttr moduleSymbol,
                                       mlir::OpBuilder &builder);

mlir::LogicalResult emitPythonImportNumericRecipe(
    const PythonImportNumericPlan &plan, mlir::Value current,
    mlir::FlatSymbolRefAttr moduleSymbol, const AstNode &moduleDeclaration,
    llvm::StringRef sourcePath, mlir::OpBuilder &builder,
    ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTNUMERIC_H
