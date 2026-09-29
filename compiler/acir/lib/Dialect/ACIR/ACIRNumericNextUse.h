#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICNEXTUSE_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICNEXTUSE_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

bool hasNumericNextUseContract(RuleOp rule);
mlir::LogicalResult verifyNumericNextUseClosure(RuleOp rule);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICNEXTUSE_H
