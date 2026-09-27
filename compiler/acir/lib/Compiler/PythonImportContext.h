#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H

#include "PythonImportAST.h"
#include "SourceUnit.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <optional>
#include <string>

namespace acir::compiler::detail {

struct NamespaceBinding {
  mlir::FlatSymbolRefAttr target;
  AstNode site;
};

struct NamespaceImportUse {
  mlir::DictionaryAttr source;
  std::string name;
  mlir::FlatSymbolRefAttr target;
  AstNode site;
};

// Private state shared by the source-unit frontend stages. It retains the
// captured module and original source identity/module-root AST paths while
// exposing local names and supplied header declarations through separate maps.
class PythonImportContext {
protected:
  PythonImportContext(const CapturedSource &source, mlir::DictionaryAttr owner,
                      const SourceHeaderRegistry &headers,
                      ac::detail::EmitError emitError);

  std::string qualifiedName(llvm::StringRef name) const;
  void bindNamespaceName(llvm::StringRef name, mlir::FlatSymbolRefAttr target,
                         const AstNode &site);
  void recordNamespaceImport(mlir::DictionaryAttr provider,
                             llvm::StringRef remoteName,
                             mlir::FlatSymbolRefAttr target,
                             const AstNode &site);
  void registerLocalDeclaration(mlir::FlatSymbolRefAttr symbol,
                                mlir::Operation *declaration);
  mlir::Operation *
  lookupCanonicalDeclaration(mlir::FlatSymbolRefAttr symbol) const;
  mlir::LogicalResult attachNamespaceMetadata();

  const CapturedSource &source;
  mlir::DictionaryAttr owner;
  const SourceHeaderRegistry &headers;
  ac::detail::EmitError emitError;
  mlir::OpBuilder builder;
  std::string module;
  llvm::StringMap<NamespaceBinding> namespaceBindings;
  llvm::StringMap<mlir::Operation *> localDeclarations;
  llvm::SmallVector<NamespaceImportUse> namespaceImportUses;
  llvm::SmallVector<mlir::DictionaryAttr> dependencies;
  mlir::OwningOpRef<mlir::ModuleOp> body;
  mlir::OwningOpRef<mlir::ModuleOp> interface;
};

mlir::FailureOr<ac::MathIntAttr>
parseStaticInteger(mlir::OpBuilder &builder, llvm::StringRef spelling,
                   ac::detail::EmitError emitError);
mlir::DictionaryAttr staticValue(mlir::OpBuilder &builder, mlir::Attribute raw,
                                 ac::detail::EmitError emitError);
mlir::Type physicalType(mlir::DictionaryAttr logical,
                        mlir::MLIRContext *context);
mlir::DictionaryAttr valueConstraint(mlir::OpBuilder &builder,
                                     mlir::DictionaryAttr type);
mlir::DictionaryAttr absentDefault(mlir::OpBuilder &builder);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H
