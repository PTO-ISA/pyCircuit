#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H

#include "PythonImportAST.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/Builders.h"

namespace acir::compiler::detail {

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
mlir::Operation *createSourceOperation(
    mlir::OpBuilder &builder, mlir::Location location, llvm::StringRef name,
    mlir::ValueRange operands, mlir::TypeRange results,
    llvm::ArrayRef<mlir::NamedAttribute> attributes, unsigned regionCount = 0);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTCONTEXT_H
