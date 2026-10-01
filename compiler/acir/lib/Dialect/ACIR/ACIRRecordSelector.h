#ifndef ACIR_LIB_DIALECT_ACIR_ACIRRECORDSELECTOR_H
#define ACIR_LIB_DIALECT_ACIR_ACIRRECORDSELECTOR_H

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

namespace acir::ac::record_detail {

struct SelectorField {
  mlir::StringAttr name;
  mlir::Type physical;
};

struct FinalRecordSelector {
  mlir::arith::AndIOp enable;
  StructCreateOp value;
};

bool isTrueI1(mlir::Value value);
bool isFalseI1(mlir::Value value);
bool isSafeUsePath(mlir::Value value);
mlir::LogicalResult verifyRecordLeafType(mlir::DictionaryAttr logical,
                                         mlir::Type type,
                                         mlir::Operation *owner);

mlir::FailureOr<FinalRecordSelector> verifyFinalRecordSelector(
    RuleOp rule, mlir::Value sourceData, mlir::Value usePath,
    mlir::Value useValid, mlir::Type recordType,
    llvm::ArrayRef<SelectorField> fields,
    llvm::ArrayRef<StructGetOp> sourceGets, StructCreateOp sourceCreate,
    mlir::arith::AndIOp fieldValidAnd);

} // namespace acir::ac::record_detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRRECORDSELECTOR_H
