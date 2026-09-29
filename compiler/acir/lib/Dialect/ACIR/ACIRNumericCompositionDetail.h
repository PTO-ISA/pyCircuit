#ifndef ACIR_LIB_DIALECT_ACIR_ACIRNUMERICCOMPOSITIONDETAIL_H
#define ACIR_LIB_DIALECT_ACIR_ACIRNUMERICCOMPOSITIONDETAIL_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac::composition_detail {

mlir::LogicalResult verifySource(RuleOp rule, mlir::ArrayAttr required);
mlir::LogicalResult verifyLowered(RuleOp rule, mlir::ArrayAttr required);
mlir::LogicalResult verifyNodeSchema(RuleOp rule, mlir::ArrayAttr required);
mlir::LogicalResult verifyUses(RuleOp rule, mlir::Block &body);
mlir::LogicalResult verifyFiniteInventory(RuleOp rule, mlir::Block &body);
bool safeControl(mlir::Value value, RuleOp rule);
bool valueIDMatchesOriginSlot(mlir::DictionaryAttr id,
                              mlir::DictionaryAttr origin, uint32_t slot);
bool rhsIdentityMatchesUse(mlir::DictionaryAttr useID,
                           mlir::DictionaryAttr valueID);
bool directUseAuthority(mlir::Value value, mlir::DictionaryAttr logical,
                        mlir::DictionaryAttr useID,
                        mlir::DictionaryAttr valueID, RuleOp rule);

} // namespace acir::ac::composition_detail

#endif
