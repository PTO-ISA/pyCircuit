#ifndef ACIR_LIB_COMPILER_SOURCENAMESPACE_H
#define ACIR_LIB_COMPILER_SOURCENAMESPACE_H

#include "SourceUnit.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler::detail {

struct NamespaceExportBinding {
  mlir::StringAttr name;
  mlir::FlatSymbolRefAttr target;
};

mlir::FailureOr<llvm::SmallVector<NamespaceExportBinding>>
readNamespaceExports(mlir::ModuleOp header,
                     const SourceHeaderRegistry &registry,
                     ac::detail::EmitError emitError);

mlir::LogicalResult verifyNamespaceImports(mlir::ModuleOp header,
                                           const SourceHeaderRegistry &registry,
                                           ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_SOURCENAMESPACE_H
