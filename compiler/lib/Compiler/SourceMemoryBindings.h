#ifndef ACIR_LIB_COMPILER_SOURCEMEMORYBINDINGS_H
#define ACIR_LIB_COMPILER_SOURCEMEMORYBINDINGS_H
#include "PythonImportAST.h"
#include "SourceUnit.h"
#include "mlir/Pass/Pass.h"
#include <memory>

namespace acir::compiler::detail {
class SourceDeclarationContext;
class SourceRuleWritesAnalysis;
struct MemoryCallPlan {
  AstNode module, call;
  mlir::DictionaryAttr occurrence;
  mlir::FlatSymbolRefAttr callee;
  llvm::SmallVector<AstNode> actuals;
  llvm::SmallVector<std::string> inputNames, resultNames;
  llvm::SmallVector<mlir::Type> inputTypes, resultTypes;
  llvm::SmallVector<bool> resultFixedBits;
  llvm::SmallVector<mlir::Attribute> parameters, typeArguments;
  uint64_t depth = 0, addressWidth = 0, payloadWidth = 0, strobeWidth = 0;
  unsigned clockPosition = 0, resetPosition = 1;
};
struct ModuleDomainPlan {
  AstNode module;
  mlir::FlatSymbolRefAttr symbol;
  mlir::FunctionType signature;
  llvm::SmallVector<mlir::NamedAttribute> sourceCallAttributes;
  bool needsDomain = false;
};
/// Owns supplied header operations as well as the registry which borrows them.
struct SourceCompilationInputs {
  mlir::DictionaryAttr owner;
  llvm::SmallVector<mlir::OwningOpRef<mlir::ModuleOp>> headerStorage;
  std::unique_ptr<SourceHeaderRegistry> headers;
};
class SourceMemoryBindingsAnalysis {
public:
  explicit SourceMemoryBindingsAnalysis(mlir::Operation *operation);
  ~SourceMemoryBindingsAnalysis();
  mlir::LogicalResult initialize(mlir::DictionaryAttr owner,
      const SourceHeaderRegistry &headers, const SourceRuleWritesAnalysis &writes);
  bool isInitialized() const { return initialized; }
  bool isValid() const { return valid; }
  llvm::ArrayRef<MemoryCallPlan> memoryCalls() const;
  llvm::ArrayRef<ModuleDomainPlan> moduleDomains() const;
  mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> lower(
      mlir::DictionaryAttr owner, const SourceHeaderRegistry &headers);
private:
  bool matchesInputs(mlir::DictionaryAttr owner,
                     const SourceHeaderRegistry &headers) const;
  mlir::Operation *operation;
  mlir::Attribute capture;
  std::unique_ptr<SourceCompilationInputs> inputs;
  std::unique_ptr<SourceDeclarationContext> declarations;
  bool initialized = false, valid = false, consumed = false;
};
std::unique_ptr<mlir::Pass> createInferSourceBindingsPass(
    mlir::DictionaryAttr owner, const SourceHeaderRegistry &headers);
void registerSourceBindingsPass();
} // namespace acir::compiler::detail
#endif
