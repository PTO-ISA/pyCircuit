#ifndef ACIR_LIB_COMPILER_SOURCENAMESPACE_H
#define ACIR_LIB_COMPILER_SOURCENAMESPACE_H

#include "SourceUnit.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler::detail {

struct NamespaceExportBinding {
  mlir::StringAttr name;
  mlir::FlatSymbolRefAttr target;
};

// Validate namespace record structure/order and same-source/name target
// consistency without checking declaration targets or current providers.
mlir::LogicalResult
verifyNamespaceRecordShapes(mlir::ModuleOp header,
                            ac::detail::EmitError emitError);

mlir::LogicalResult verifyRetainedNamespaceTargets(
    mlir::ModuleOp header,
    const llvm::StringMap<mlir::Operation *> &declarations,
    ac::detail::EmitError emitError);

mlir::LogicalResult
verifyNamespaceTargetCategory(mlir::Operation *declaration,
                              mlir::FlatSymbolRefAttr target,
                              ac::detail::EmitError emitError);

mlir::FailureOr<llvm::SmallVector<NamespaceExportBinding>>
readNamespaceExports(mlir::ModuleOp header,
                     const SourceHeaderRegistry &registry,
                     ac::detail::EmitError emitError);

mlir::LogicalResult verifyNamespaceImports(
    mlir::ModuleOp header, const SourceHeaderRegistry &registry,
    ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_SOURCENAMESPACE_H
