#ifndef ACIR_LIB_DIALECT_ACIR_SOURCESTATICRECORDCONSTRUCTORS_H
#define ACIR_LIB_DIALECT_ACIR_SOURCESTATICRECORDCONSTRUCTORS_H

#include "ACIRStaticEvaluation.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/DenseSet.h"

namespace acir::ac::detail {
using EvaluateStaticChild =
    llvm::function_ref<mlir::FailureOr<mlir::DictionaryAttr>(StaticExprAttr)>;
mlir::FailureOr<mlir::func::FuncOp>
resolveSourceStaticRecordConstructor(mlir::FlatSymbolRefAttr callee,
                                     mlir::Operation *fixedScope,
                                     EmitError error);
mlir::FailureOr<llvm::SmallVector<unsigned>>
bindStaticRecordArguments(mlir::func::FuncOp helper, mlir::ArrayAttr arguments,
                          EmitError error);
mlir::FailureOr<mlir::DictionaryAttr> evaluateSourceStaticRecordConstructor(
    mlir::func::FuncOp helper, mlir::ArrayAttr arguments,
    mlir::Operation *fixedScope,
    llvm::DenseSet<mlir::Operation *> &activeConstructors,
    EvaluateStaticChild evaluateChild, EmitError error);
} // namespace acir::ac::detail
#endif
