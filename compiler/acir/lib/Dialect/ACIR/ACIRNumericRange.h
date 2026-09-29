#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICRANGE_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICRANGE_H

#include "ACIRNumericProof.h"

namespace acir::ac {

mlir::LogicalResult verifyCheckedToBitsWitness(RuleOp rule,
                                               ValueBindingOp resultBinding,
                                               NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICRANGE_H
