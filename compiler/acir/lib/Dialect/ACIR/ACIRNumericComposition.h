#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICCOMPOSITION_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICCOMPOSITION_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

bool hasNumericCompositionContract(RuleOp rule);
mlir::LogicalResult verifyNumericCompositionClosure(RuleOp rule);
mlir::LogicalResult verifyNumericCompositionExpect(SourceExpectOp op);
mlir::LogicalResult verifyNumericCompositionObserve(SourceObserveOp op);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICCOMPOSITION_H
