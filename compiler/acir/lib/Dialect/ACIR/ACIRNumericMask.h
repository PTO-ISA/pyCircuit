#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICMASK_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICMASK_H

#include "ACIRNumericProof.h"

namespace acir::ac {

mlir::LogicalResult
verifyExactInputConstantMaskWitness(RuleOp rule, ValueBindingOp resultBinding,
                                    NumericProofOp proof);

mlir::LogicalResult verifyLowBitsInputMaskWitness(RuleOp rule,
                                                  ValueBindingOp resultBinding,
                                                  NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICMASK_H
