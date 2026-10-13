#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEHELPERANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEHELPERANALYSIS_H

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FunctionExtras.h"

namespace acir::ac {

using SourceHelperLookup =
    llvm::function_ref<mlir::func::FuncOp(mlir::FlatSymbolRefAttr)>;

mlir::LogicalResult verifySourceValueHelper(mlir::func::FuncOp helper,
                                           SourceHelperLookup lookup);
mlir::LogicalResult verifySourceValueHelperClosure(
    llvm::ArrayRef<mlir::func::FuncOp> helpers, SourceHelperLookup lookup);

} // namespace acir::ac

#endif
