#ifndef ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
#define ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H

#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <cstdint>

namespace acir::ac::detail {

using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

// Optional source-call authority on existing hardware module declarations.
mlir::LogicalResult verifyModuleSourceCallContract(mlir::Operation *owner);

// Validate original hardware syntax against its lexical module inventories.
// Bound forwarded actuals are resolved separately by HardwareAnalysis.
mlir::LogicalResult verifyHardwareAttributeScope(mlir::Attribute attribute,
                                               mlir::Operation *owner);
mlir::LogicalResult verifyHardwareTypeScope(mlir::Type type,
                                          mlir::Operation *owner);

mlir::DictionaryAttr declarationSourceOwner(mlir::Operation *operation);
bool hasQualifiedDeclarationIdentityForOwner(mlir::Operation *operation,
                                             mlir::DictionaryAttr owner);
mlir::LogicalResult verifyOriginDefinition(mlir::DictionaryAttr origin,
                                           mlir::FlatSymbolRefAttr expected,
                                           llvm::StringRef description,
                                           EmitError emitError);
mlir::LogicalResult verifyNamespaceRecordShapes(mlir::ModuleOp unit,
                                                EmitError emitError);
mlir::LogicalResult verifyRetainedNamespaceTargets(
    mlir::ModuleOp unit, const llvm::StringMap<mlir::Operation *> &declarations,
    EmitError emitError);
mlir::LogicalResult
verifyNamespaceTargetCategory(mlir::Operation *declaration,
                              mlir::FlatSymbolRefAttr target,
                              EmitError emitError);

mlir::LogicalResult verifyDeclarationMetadata(mlir::Operation *operation,
                                              mlir::DictionaryAttr owner,
                                              mlir::DictionaryAttr origin,
                                              mlir::StringAttr role);

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
mlir::FailureOr<uint32_t> decodeU32(mlir::IntegerAttr value,
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

mlir::LogicalResult verifySourceOwner(mlir::DictionaryAttr value,
                                      EmitError emitError);
mlir::LogicalResult verifyExpansionFrame(mlir::DictionaryAttr value,
                                         EmitError emitError);
mlir::LogicalResult verifyOccurrence(mlir::DictionaryAttr value,
                                     EmitError emitError);
mlir::LogicalResult verifySpecKey(mlir::DictionaryAttr value,
                                  EmitError emitError);
mlir::LogicalResult verifyValueID(mlir::DictionaryAttr value,
                                  EmitError emitError);
mlir::LogicalResult verifyUseID(mlir::DictionaryAttr value,
                                EmitError emitError);
mlir::LogicalResult verifyCheckID(mlir::DictionaryAttr value,
                                  EmitError emitError);
mlir::LogicalResult verifyProofScope(mlir::DictionaryAttr value,
                                     EmitError emitError);
mlir::LogicalResult verifyOwnerRef(mlir::DictionaryAttr value,
                                   EmitError emitError);
mlir::LogicalResult verifyStateID(mlir::DictionaryAttr value,
                                  EmitError emitError);
mlir::LogicalResult verifyStateRef(mlir::DictionaryAttr value,
                                   EmitError emitError);

// Total structural order for attributes admitted by closed source identity
// schemas. Callers must verify the enclosing Occurrence or StateRef first.
int compareClosedSourceStructure(mlir::Attribute left, mlir::Attribute right);

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
