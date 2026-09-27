#ifndef ACIR_LIB_DIALECT_ACIR_ACIRSTATICEVALUATION_H
#define ACIR_LIB_DIALECT_ACIR_ACIRSTATICEVALUATION_H

#include "ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "mlir/IR/Operation.h"

namespace acir::ac::detail {

mlir::FailureOr<ResolvedRecordView>
resolveSourceRecord(mlir::FlatSymbolRefAttr symbol, mlir::Operation *site,
                    EmitError emitError);

mlir::FailureOr<mlir::DictionaryAttr>
evaluateSourceStaticExpr(StaticExprAttr expression, mlir::Operation *site,
                         EmitError emitError);

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRSTATICEVALUATION_H
