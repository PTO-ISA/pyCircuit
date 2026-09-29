#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRULEEXPRESSIONS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRULEEXPRESSIONS_H

#include "PythonImportRules.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler::detail {

struct RuleExpressionValue {
  mlir::Value value;
  mlir::Value valid;
  mlir::DictionaryAttr logical;
  mlir::DictionaryAttr id;
  std::optional<unsigned> numeric;
};

/// Captures expression meaning in source ACIR. Width selection and arithmetic
/// legalization remain native MLIR lowering responsibilities.
class RuleExpressionProducer {
public:
  RuleExpressionProducer(mlir::OpBuilder &builder, ac::RuleOp rule,
                         const ModuleModel &module, llvm::StringRef sourcePath,
                         const llvm::StringMap<mlir::Value> &entries,
                         const llvm::StringMap<mlir::DictionaryAttr> &types,
                         ac::detail::EmitError error);
  mlir::FailureOr<RuleExpressionValue> read(const AstNode &node,
                                            mlir::Value path);
  mlir::FailureOr<RuleExpressionValue> emit(const AstNode &node,
                                            mlir::Value path);
  mlir::FailureOr<RuleExpressionValue> boundary(const AstNode &expression,
                                                const AstNode &site,
                                                RuleExpressionValue value,
                                                mlir::Value path,
                                                mlir::DictionaryAttr domain);
  mlir::LogicalResult assertion(const AstNode &statement, mlir::Value &live);
  mlir::Value boolean(bool value, const AstNode &site);
  mlir::Value conjunction(mlir::Value lhs, mlir::Value rhs,
                          const AstNode &site);
  mlir::Value disjunction(mlir::Value lhs, mlir::Value rhs,
                          const AstNode &site);
  mlir::Value invert(mlir::Value value, const AstNode &site);
  mlir::Value continueAfter(mlir::Value live, mlir::Value path,
                            mlir::Value valid, const AstNode &site);
  mlir::DictionaryAttr identity(const AstNode &site, unsigned slot = 0);
  void finish();

private:
  mlir::DictionaryAttr origin(const AstNode &site);
  mlir::DictionaryAttr reference(unsigned index, llvm::StringRef kind);
  unsigned numericNode(mlir::DictionaryAttr id, llvm::StringRef opcode,
                       llvm::ArrayRef<mlir::Attribute> operands,
                       mlir::DictionaryAttr target = {});
  mlir::DictionaryAttr check(const AstNode &site, llvm::StringRef kind,
                             mlir::Value condition, mlir::Value path);
  mlir::Operation *operation(llvm::StringRef name, const AstNode &site,
                             mlir::ValueRange operands, mlir::TypeRange results,
                             llvm::ArrayRef<mlir::NamedAttribute> attrs);
  mlir::OpBuilder &b;
  ac::RuleOp rule;
  const ModuleModel &module;
  llvm::StringRef sourcePath;
  const llvm::StringMap<mlir::Value> &entries;
  const llvm::StringMap<mlir::DictionaryAttr> &types;
  ac::detail::EmitError error;
  llvm::SmallVector<mlir::Attribute> nodes;
  llvm::SmallVector<mlir::Attribute> checks;
};

} // namespace acir::compiler::detail
#endif
