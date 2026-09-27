#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTMODULES_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTMODULES_H

#include "PythonImportInternal.h"
#include "PythonImportSignature.h"

namespace acir::compiler::detail {

struct ModuleParameter {
  ParameterSyntax syntax;
  std::string name;
  unsigned ordinal = 0;
  std::string category;
  mlir::DictionaryAttr type;
  mlir::DictionaryAttr defaultValue;
};

struct ModuleMember {
  enum class Kind { Connection, OwnedState, ChildInstance };

  Kind kind;
  std::string name;
  AstNode declaration;
  std::string parameter;
  mlir::FlatSymbolRefAttr child;
  mlir::Operation *childHeader = nullptr;
  mlir::DictionaryAttr logicalType;
  mlir::Attribute initialValue;
  mlir::Type payloadType;
  mlir::Value handle;
  mlir::Value currentHandle;
  mlir::Value nextHandle;
  llvm::SmallVector<size_t> childInputMembers;
  llvm::SmallVector<size_t> childOutputMembers;
  bool read = false;
  bool write = false;
  std::string precision = "exact";
  llvm::SmallVector<AstNode> readSites;
  llvm::SmallVector<AstNode> writeSites;
};

struct RuleRegistration {
  AstNode call;
  AstNode method;
  AstNode outputTarget;
  std::string outputMember;
};

struct ModuleAction {
  enum class Kind { Member, RuleRegistration };

  Kind kind;
  size_t index = 0;
  AstNode site;
};

struct ModuleModel {
  AstNode declaration;
  AstNode constructor;
  mlir::FlatSymbolRefAttr symbol;
  llvm::SmallVector<ModuleParameter> parameters;
  llvm::SmallVector<ModuleMember, 0> members;
  llvm::SmallVector<AstNode> inactiveRuleMethods;
  llvm::SmallVector<RuleRegistration> registrations;
  llvm::SmallVector<ModuleAction> actions;
};

bool isModuleDocstring(const AstNode &node);
bool isModuleSelfMember(const AstNode &node, llvm::StringRef *name = nullptr);
AstNode relativeToModule(const AstNode &anchor, const AstNode &node);
mlir::LogicalResult
validateChildParameterCategories(mlir::ArrayAttr parameters,
                                 ac::detail::EmitError emitError);

class ModuleCompiler {
public:
  ModuleCompiler(RecordCompiler &sourceCompiler, const AstNode &declaration);

  mlir::LogicalResult run();

private:
  mlir::FailureOr<ModuleModel> classify();
  mlir::FailureOr<ac::StaticExprAttr>
  staticExpression(const ModuleModel &model, const AstNode &expression,
                   mlir::DictionaryAttr expected);
  mlir::LogicalResult emitHeader(ModuleModel &model);
  mlir::LogicalResult emitBody(ModuleModel &model, RuleCompiler &rules);
  mlir::LogicalResult classifyInstanceArguments(ModuleModel &parent,
                                                ModuleMember &member);

  RecordCompiler &sourceCompiler;
  const AstNode &declaration;
};

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTMODULES_H
