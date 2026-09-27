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

struct ImportBinding {
  std::string module;
  mlir::FlatSymbolRefAttr symbol;
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

  const CapturedSource &source;
  mlir::DictionaryAttr owner;
  const SourceHeaderRegistry &headers;
  ac::detail::EmitError emitError;
  mlir::OpBuilder builder;
  std::string module;
  llvm::StringMap<mlir::DictionaryAttr> aliases;
  llvm::StringMap<ImportBinding> imports;
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
