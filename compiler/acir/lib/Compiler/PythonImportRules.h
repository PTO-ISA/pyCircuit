#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRULES_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRULES_H

#include "PythonImportModules.h"

namespace acir::compiler::detail {

struct BoundRuleArgument {
  std::string localName;
  size_t memberIndex = 0;
  AstNode formal;
  AstNode actual;
};

struct RulePlan {
  size_t registrationIndex = 0;
  FunctionSignatureSyntax signature;
  llvm::SmallVector<BoundRuleArgument> arguments;
  llvm::SmallVector<size_t> inputs;
  llvm::SmallVector<size_t> outputs;
};

bool rulePlanHasMemberInput(const RulePlan &plan, size_t memberIndex);

class RuleCompiler {
public:
  RuleCompiler(RecordCompiler &sourceCompiler, ModuleModel &module);

  mlir::LogicalResult analyze();
  mlir::LogicalResult validateInactiveMethods();
  mlir::LogicalResult emit(size_t registrationIndex,
                           mlir::Operation *moduleOperation);

private:
  mlir::FailureOr<mlir::Value>
  expression(const RulePlan &plan, const AstNode &node,
             const llvm::StringMap<mlir::Value> &values,
             mlir::OpBuilder &builder, mlir::DictionaryAttr resultType);
  mlir::LogicalResult analyzeRegistration(size_t registrationIndex,
                                          RulePlan &plan);

  RecordCompiler &sourceCompiler;
  ModuleModel &module;
  llvm::SmallVector<RulePlan, 0> plans;
};

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTRULES_H
