#ifndef ACIR_LIB_DIALECT_ACIR_ACIRFINALUSES_H
#define ACIR_LIB_DIALECT_ACIR_ACIRFINALUSES_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

bool hasGenericFinalUses(RuleOp rule);
mlir::LogicalResult verifyGenericFinalUses(RuleOp rule);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRFINALUSES_H
