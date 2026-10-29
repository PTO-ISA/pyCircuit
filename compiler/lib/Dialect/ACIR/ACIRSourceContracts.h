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
#include <utility>

namespace acir::ac {
class HardwareAnalysis;
struct HardwareBindings;
class ModuleImportOp;
} // namespace acir::ac

namespace acir::ac::detail {

using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

// Implementation storage for common small static-expression grammars, not an
// admission limit. The source closure preflight accounts for this same storage.
inline constexpr unsigned staticGrammarInlineBuckets = 16;

// Typed effects are verified intermediate IR, not published or final hardware.
// Inspect operation kinds without recursively traversing raw attribute/type
// graphs or imposing transaction-analysis limits on ordinary hardware.
mlir::Operation *findUnloweredTransaction(mlir::Operation *root);
mlir::LogicalResult verifyNoUnloweredTransactions(mlir::Operation *root);
bool isCombinationalHardwareOperation(mlir::Operation *operation);
// Validate every closed layout constraint while retaining only well-scoped
// Integer-formal dependencies until occurrence binding. Reports concreteness
// separately so a template declaration cannot establish executable validity.
mlir::LogicalResult verifySourceTemplateType(const HardwareAnalysis &analysis,
                                             mlir::Type type,
                                             const HardwareBindings &bindings,
                                             mlir::Operation *site,
                                             bool *concrete = nullptr);

// Existing source-state initializer grammar, also used to prove physical
// elaboration initialization under each actual storage binding.
mlir::LogicalResult verifyClosedStateInitializer(
    const HardwareAnalysis &analysis, mlir::Value initial,
    const HardwareBindings &bindings, mlir::Operation *site);

// Optional source-call authority on existing hardware module declarations.
mlir::LogicalResult verifyModuleSourceCallContract(mlir::Operation *owner);
mlir::LogicalResult verifyModuleSignature(mlir::Operation *declaration);
mlir::LogicalResult verifyModuleImportSummarySelection(ModuleImportOp imported);

// Callable constraints resolve hardware in the lexical module; diagnostics
// belong to the module or Method whose source contract is being checked.
enum class SourceCallWidthMode { Concrete, SymbolicFixedBits };
mlir::LogicalResult verifySourceCallHardwareConstraint(
    mlir::Attribute raw, mlir::Type physical, const HardwareAnalysis &analysis,
    mlir::Operation *lexicalModule, mlir::Operation *diagnosticOperation,
    llvm::StringRef description,
    SourceCallWidthMode widthMode = SourceCallWidthMode::Concrete);
// Shared nominal field schema. Only Boolean fields extend the legacy physical
// descriptor with an authoritative source constraint.
mlir::LogicalResult
verifyNominalStructField(mlir::Attribute raw, const HardwareAnalysis &analysis,
                         mlir::Operation *lexicalModule,
                         mlir::Operation *diagnosticOperation);
mlir::LogicalResult verifyRequiredSourceParameter(
    mlir::Attribute raw, mlir::StringAttr expectedName,
    mlir::DictionaryAttr sourceOwner, mlir::FlatSymbolRefAttr definition,
    mlir::Operation *diagnosticOperation, llvm::StringRef description);
mlir::LogicalResult verifyRequiredSourceParameter(
    mlir::Attribute raw, mlir::StringAttr expectedName,
    mlir::Operation *lexicalModule, mlir::Operation *diagnosticOperation,
    llvm::StringRef description);

// Resolve a branch payload only after bounded validation of its nominal
// closure.
mlir::FailureOr<mlir::Type> resolveSourceBranchResultType(
    mlir::Type actualType, const HardwareAnalysis &analysis,
    const HardwareBindings &lexicalBindings, mlir::Operation *diagnosticSite,
    llvm::function_ref<mlir::LogicalResult(uint64_t workUnits,
                                           uint64_t allocationBytes)>
        charge);

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
mlir::LogicalResult verifySourceGuardLocation(mlir::DictionaryAttr location,
                                              mlir::DictionaryAttr sourceOwner,
                                              EmitError emitError);
mlir::LogicalResult verifySourceEffectMetadata(mlir::DictionaryAttr occurrence,
                                               mlir::DictionaryAttr location,
                                               mlir::Operation *lexicalModule,
                                               EmitError emitError);

// Intrinsic immutable grammar only; no lexical-owner or source admission fact.
bool isVerifiedOccurrenceGrammar(mlir::DictionaryAttr value);
bool isVerifiedSourceSpanGrammar(mlir::DictionaryAttr value);
void rememberVerifiedSourceSpanGrammar(mlir::DictionaryAttr value);
std::pair<uint64_t, uint64_t>
sourceSpanGrammarCacheShape(mlir::MLIRContext *context);
bool isVerifiedStaticExpressionGrammar(mlir::DictionaryAttr value);
void rememberVerifiedStaticExpressionGrammar(mlir::DictionaryAttr value);
std::pair<uint64_t, uint64_t>
staticExpressionGrammarCacheShape(mlir::MLIRContext *context);
std::pair<uint64_t, uint64_t>
occurrenceGrammarCacheShape(mlir::MLIRContext *context);

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
mlir::LogicalResult verifyObservationID(mlir::DictionaryAttr value,
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
// Canonical order shared by source capture and namespace validation.
int compareSourceImportBindings(mlir::DictionaryAttr left,
                                mlir::DictionaryAttr right);
int compareClosedSourceStructure(mlir::Attribute left, mlir::Attribute right);

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
