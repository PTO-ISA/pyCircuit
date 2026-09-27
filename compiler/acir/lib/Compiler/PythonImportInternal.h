#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H

#include "PythonImportAST.h"
#include "PythonImportContext.h"
#include "SourceUnit.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"

namespace acir::compiler::detail {

class RecordCompiler : protected PythonImportContext {
public:
  RecordCompiler(const CapturedSource &source, mlir::DictionaryAttr owner,
                 const SourceHeaderRegistry &headers,
                 ac::detail::EmitError emitError);

  mlir::FailureOr<SourceUnitArtifacts> run();

private:
  mlir::LogicalResult scanImportsAndAliases();
  mlir::LogicalResult emitRecord(const AstNode &node);
  mlir::LogicalResult emitValueHelper(const AstNode &node);
  mlir::FailureOr<mlir::DictionaryAttr> annotation(const AstNode &node);
  mlir::FailureOr<mlir::Value> expression(const AstNode &node,
                                          mlir::func::FuncOp function,
                                          mlir::OpBuilder &at,
                                          mlir::Value &liveValid);
  mlir::FailureOr<mlir::Value> recordCall(const AstNode &node,
                                          mlir::func::FuncOp function,
                                          mlir::OpBuilder &at,
                                          mlir::Value &liveValid);
  mlir::FailureOr<mlir::Value> constant(const AstNode &node,
                                        mlir::DictionaryAttr expected,
                                        mlir::OpBuilder &at);
  mlir::LogicalResult cloneImportedDeclarations();
};

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H
