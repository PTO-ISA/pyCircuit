#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRULECONTROL_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRULECONTROL_H
#include "PythonImportRuleExpressions.h"
namespace acir::compiler::detail {
bool needsComposedRule(const AstNode &method);
mlir::LogicalResult emitComposedRule(
    mlir::OpBuilder &builder, ac::RuleOp rule, const RulePlan &plan,
    const ModuleModel &module, const AstNode &method, llvm::StringRef path,
    const llvm::StringMap<mlir::Value> &entries,
    const llvm::StringMap<mlir::DictionaryAttr> &types,
    llvm::function_ref<llvm::StringRef(const AstNode &)> observationIntrinsic,
    ac::detail::EmitError error);
} // namespace acir::compiler::detail
#endif
