#ifndef ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
#define ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H

#include "acir/Dialect/ACIR/ACIRAttributes.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace acir::ac::detail {

using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

enum class ExpectedTypeKind { Logical, Static };

struct ResolvedRecordView {
  mlir::FlatSymbolRefAttr symbol;
  llvm::SmallVector<mlir::DictionaryAttr> fieldLogicalTypes;
};

using RecordResolver = llvm::function_ref<mlir::FailureOr<ResolvedRecordView>(
    mlir::FlatSymbolRefAttr)>;

mlir::FailureOr<MathIntAttr> parseMathIntAttr(mlir::MLIRContext *context,
                                              llvm::StringRef spelling,
                                              EmitError emitError);

mlir::FailureOr<uint64_t> decodeU64(mlir::IntegerAttr value,
                                    llvm::StringRef description,
                                    EmitError emitError);

mlir::LogicalResult verifySourceSpan(mlir::DictionaryAttr value,
                                     EmitError emitError);

mlir::LogicalResult verifyPathComponent(mlir::DictionaryAttr value,
                                        EmitError emitError);

mlir::LogicalResult verifySite(mlir::DictionaryAttr value, EmitError emitError);

mlir::LogicalResult verifyLogicalTypeStructure(mlir::DictionaryAttr value,
                                               EmitError emitError);
mlir::LogicalResult verifyStaticTypeStructure(mlir::DictionaryAttr value,
                                              EmitError emitError);
mlir::LogicalResult verifyStaticValueStructure(mlir::DictionaryAttr value,
                                               EmitError emitError);
mlir::LogicalResult verifyDefaultStructure(mlir::DictionaryAttr value,
                                           EmitError emitError);

mlir::LogicalResult verifyTypeResolved(mlir::DictionaryAttr type,
                                       ExpectedTypeKind kind,
                                       RecordResolver resolver,
                                       EmitError emitError);

mlir::LogicalResult verifyStaticValueMatchesType(
    mlir::DictionaryAttr value, mlir::DictionaryAttr expectedType,
    ExpectedTypeKind kind, RecordResolver resolver, EmitError emitError);

mlir::LogicalResult verifyDefaultMatchesType(mlir::DictionaryAttr defaultValue,
                                             mlir::DictionaryAttr expectedType,
                                             ExpectedTypeKind kind,
                                             RecordResolver resolver,
                                             EmitError emitError);

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
