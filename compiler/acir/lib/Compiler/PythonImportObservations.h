#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTOBSERVATIONS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTOBSERVATIONS_H

#include "PythonImportAST.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler::detail {

class PythonImportObservationProducer {
public:
  PythonImportObservationProducer(
      mlir::OpBuilder &builder, llvm::StringRef sourcePath,
      mlir::FlatSymbolRefAttr moduleSymbol, const AstNode &moduleDeclaration,
      mlir::DictionaryAttr registration, ac::detail::EmitError emitError,
      const llvm::StringMap<mlir::Value> &localValues,
      const llvm::StringMap<mlir::Value> &entryValues,
      const llvm::StringMap<mlir::DictionaryAttr> &valueTypes);

  /// Emit one already-classified canonical observation call. An empty
  /// intrinsic means the expression is not an observation and returns false.
  mlir::FailureOr<bool>
  emit(const AstNode &statement, llvm::StringRef intrinsic, mlir::Value path,
       llvm::SmallVectorImpl<mlir::Attribute> &requiredObservations);

private:
  mlir::FailureOr<mlir::StringAttr> staticString(const AstNode &node,
                                                 llvm::StringRef role);
  mlir::FailureOr<mlir::Value>
  dynamicValue(const AstNode &node,
               mlir::SmallVectorImpl<mlir::Attribute> &valueIDs,
               mlir::SmallVectorImpl<mlir::Attribute> &constraints);

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

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTOBSERVATIONS_H
