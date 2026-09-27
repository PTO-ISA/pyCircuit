#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H

#include "PythonImportAST.h"
#include "SourceUnit.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/StringMap.h"

#include <optional>
#include <string>

namespace acir::compiler::detail {

struct ImportBinding {
  std::string module;
  mlir::FlatSymbolRefAttr symbol;
};

class RecordCompiler {
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

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTINTERNAL_H
