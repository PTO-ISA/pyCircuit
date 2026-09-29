#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTINPUTADD_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTINPUTADD_H

#include "ACIRNumericProof.h"

namespace acir::ac {

mlir::LogicalResult
verifyExactInputConstantAddWitness(RuleOp rule, ValueBindingOp resultBinding,
                                   NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTINPUTADD_H
