#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTADD_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTADD_H

#include "ACIRNumericProof.h"

namespace acir::ac {

mlir::LogicalResult verifyExactConstantAddWitness(RuleOp rule,
                                                  ValueBindingOp resultBinding,
                                                  NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICEXACTADD_H
