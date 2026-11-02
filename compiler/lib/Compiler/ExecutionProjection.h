#ifndef ACIR_LIB_COMPILER_EXECUTIONPROJECTION_H
#define ACIR_LIB_COMPILER_EXECUTIONPROJECTION_H

#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

namespace acir::compiler {

mlir::LogicalResult validateExecutionModelDeclaration(mlir::Operation *owner);

// Operates only on the authenticated linked clone. The source-unit body and
// header remain unchanged. No output may be published on failed admission.
mlir::LogicalResult selectExecutionModel(mlir::ModuleOp package,
                                         ac::ModuleOp &root,
                                         llvm::StringRef execution);

} // namespace acir::compiler

#endif
