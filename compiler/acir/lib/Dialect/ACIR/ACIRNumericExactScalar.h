#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTSCALAR_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTSCALAR_H

#include "ACIRNumericProof.h"

namespace acir::ac {

mlir::LogicalResult
verifyExactInputConstantSubWitness(RuleOp rule, ValueBindingOp resultBinding,
                                   NumericProofOp proof);

mlir::LogicalResult verifyExactInputConstantCompareWitness(
    RuleOp rule, ValueBindingOp resultBinding, NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTSCALAR_H
