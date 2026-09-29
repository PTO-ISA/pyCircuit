#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTCHECKS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTCHECKS_H

#include "PythonImportAST.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler::detail {

class PythonImportCheckProducer {
public:
  PythonImportCheckProducer(
      mlir::OpBuilder &builder, llvm::StringRef sourcePath,
      mlir::FlatSymbolRefAttr moduleSymbol, const AstNode &moduleDeclaration,
      mlir::DictionaryAttr registration, ac::detail::EmitError emitError,
      const llvm::StringMap<mlir::Value> &localValues,
      const llvm::StringMap<mlir::Value> &entryValues,
      const llvm::StringMap<mlir::DictionaryAttr> &valueTypes);

  mlir::LogicalResult
  emitAssert(const AstNode &statement, mlir::Value path, uint64_t obligation,
             llvm::SmallVectorImpl<mlir::Attribute> &requiredChecks);

private:
  mlir::OpBuilder &builder;
  llvm::StringRef sourcePath;
  mlir::FlatSymbolRefAttr moduleSymbol;
  const AstNode &moduleDeclaration;
  mlir::DictionaryAttr registration;
  ac::detail::EmitError emitError;
  const llvm::StringMap<mlir::Value> &localValues;
  const llvm::StringMap<mlir::Value> &entryValues;
  const llvm::StringMap<mlir::DictionaryAttr> &valueTypes;
};

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTCHECKS_H
