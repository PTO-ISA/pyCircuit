#ifndef ACIR_LIB_DIALECT_ACIR_ACIRFINALRECORDUSES_H
#define ACIR_LIB_DIALECT_ACIR_ACIRFINALRECORDUSES_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

bool hasFinalRecordUses(RuleOp rule);
mlir::LogicalResult verifyFinalRecordUses(RuleOp rule);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRFINALRECORDUSES_H
