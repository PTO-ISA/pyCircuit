#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICPROOF_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICPROOF_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

mlir::LogicalResult verifyRuleNumericClosure(RuleOp rule);
mlir::LogicalResult verifySourceNumericClosure(RuleOp rule,
                                               mlir::ArrayAttr required);
mlir::LogicalResult verifyExactConstantAddWitness(RuleOp rule,
                                                  ValueBindingOp resultBinding,
                                                  NumericProofOp proof);
mlir::LogicalResult verifyExactInputWitness(RuleOp rule,
                                            ValueBindingOp resultBinding,
                                            NumericProofOp proof);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRNUMERICPROOF_H
