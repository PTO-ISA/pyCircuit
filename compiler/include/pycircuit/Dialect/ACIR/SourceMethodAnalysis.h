#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>
#include <map>
#include <memory>
namespace acir::ac {

using SourceModuleLookup =
    llvm::function_ref<mlir::Operation *(mlir::FlatSymbolRefAttr)>;

/// Checked allocation authority, borrowed from an actual body or import row.
/// Imported payload types and configuration never consult a provider body.
struct SourceQueueView {
  mlir::Operation *module = nullptr;
  InstanceOp allocation;
  mlir::DictionaryAttr row;
  mlir::Type getPayloadType() const;
  mlir::Attribute getPayloadConstraint() const;
  mlir::DictionaryAttr getOccurrence() const;
  mlir::DictionaryAttr getDomain() const;
  mlir::DictionaryAttr getConfig() const;
  bool isPushUnbound() const;
  bool isPopUnbound() const;
};
mlir::FailureOr<SourceQueueView>
lookupSourceQueue(mlir::Operation *module, llvm::StringRef name,
                  mlir::Operation *diagnosticSite);
/// Shared queue configuration/type verification for native and imported owners.
mlir::LogicalResult verifySourceQueueConfiguration(
    mlir::Operation *lexicalModule, mlir::DictionaryAttr config,
    mlir::Type payloadType, mlir::Operation *diagnosticSite);
/// Derive checked allocation attributes/domain from the actual lexical FIFO
/// instance. The canonical primitive declaration remains configuration
/// authority; this row adds only allocation-specific source binding facts.
/// Publication adds raw operand value facts under its current value scope.
mlir::FailureOr<mlir::DictionaryAttr> getSourceQueueOwnerRow(InstanceOp queue);

enum class NativeQueueAction { None, Push, Pop, Peek };
enum class NativeMemoryAction { None, Request, Result, Read, Write };
/// Source-owned Memory authority is reconstructed from the actual allocation;
/// imported rows are checked again against the canonical primitive signature.
mlir::FailureOr<mlir::DictionaryAttr> getSourceMemoryOwnerRow(InstanceOp memory);

/// A checked, borrowed Method contract from a definition or interface row.
struct SourceMethodView {
  mlir::Operation *module = nullptr;
  SourceMethodOp definition;
  mlir::DictionaryAttr row;
  NativeQueueAction nativeQueueAction = NativeQueueAction::None;
  /// The actual terminal allocation for a body call. Imported summary calls
  /// retain the checked owner row instead. Neither form invents a Method body
  /// or callable frame.
  InstanceOp nativeQueueInstance;
  mlir::DictionaryAttr nativeQueueOwnerRow;
  mlir::Type nativeQueuePayloadType;
  mlir::Attribute nativeQueuePayloadConstraint;
  bool isNativeQueueTerminal() const {
    return nativeQueueAction != NativeQueueAction::None;
  }
  NativeMemoryAction nativeMemoryAction = NativeMemoryAction::None;
  unsigned nativeMemoryPort = 0;
  InstanceOp nativeMemoryInstance;
  mlir::DictionaryAttr nativeMemoryOwnerRow;
  mlir::Type nativeMemoryPayloadType;
  mlir::Attribute nativeMemoryPayloadConstraint;
  bool isNativeMemoryTerminal() const {
    return nativeMemoryAction != NativeMemoryAction::None;
  }
  bool isNativeTerminal() const {
    return isNativeQueueTerminal() || isNativeMemoryTerminal();
  }
  mlir::FunctionType getFunctionType() const;
  mlir::ArrayAttr getArgumentNames() const;
  mlir::ArrayAttr getParameters() const;
  mlir::ArrayAttr getResultConstraints() const;
  mlir::ArrayAttr getFormalProposals() const;
  /// Number of authored results, excluding checked trailing [data, intent]
  /// pairs for writable formals. Requires a verified Method contract.
  unsigned getNumPublicResults() const;
  mlir::StringAttr getReturnForm() const;
  mlir::StringAttr getVisibility() const;
  mlir::DictionaryAttr getDeclaration() const;
  mlir::DictionaryAttr getGuardLocation() const;
  mlir::SymbolRefAttr getSymbol() const;
};

/// Check the optional writable-formal mapping and its physical result suffix.
/// The ordinary parameter/result hardware constraints must also be verified.
mlir::LogicalResult
verifySourceMethodFormalProposals(const SourceMethodView &method,
                                  mlir::Operation *diagnosticSite);

/// Closed local schema for call owner bindings. A missing callee proposal
/// contract means external-header closure still has to establish membership.
/// Actual SSA/current-candidate identity remains an invocation analysis proof.
mlir::LogicalResult verifySourceFormalOwnerBindings(
    mlir::Attribute raw, unsigned argumentCount,
    std::optional<mlir::ArrayAttr> calleeProposals,
    mlir::FunctionType callerSignature, mlir::ArrayAttr callerProposals,
    llvm::function_ref<bool(llvm::StringRef)> hasCallerState,
    mlir::Operation *diagnosticSite);

struct ResolvedSourceMethod {
  SourceMethodView method;
  HardwareBindings bindings;
  mlir::DictionaryAttr receiver;
};

/// A borrowed budget interface for one immutable receiver proof invocation.
/// No callable, parameter binding, effect or actual-call verdict is cached.
struct SourceReceiverProofFees {
  llvm::function_ref<mlir::LogicalResult(mlir::Operation *, mlir::Operation *)>
      moduleMiss;
  llvm::function_ref<mlir::LogicalResult(SourceMethodOp, mlir::Operation *)>
      selectedMethod;
  llvm::function_ref<mlir::LogicalResult(mlir::Operation *, uint64_t, uint64_t)>
      workStorage;
  llvm::function_ref<mlir::LogicalResult(mlir::Operation *, mlir::Operation *,
                                       mlir::Operation *, mlir::Operation *)>
      instanceBinding;
};

/// Must not outlive or span mutation of its borrowed IR. Analysis and
/// independent source replay each own a fresh context. Only complete
/// inventories are stored.
class SourceReceiverProofContext {
public:
  struct Instance {
    mlir::StringAttr name;
    mlir::DictionaryAttr occurrence;
    mlir::Operation *allocation = nullptr;
    mlir::DictionaryAttr row;
  };
  struct Inventory {
    std::unique_ptr<Instance[]> instances;
    size_t instanceCount = 0;
    std::unique_ptr<SourceMethodView[]> methods;
    size_t methodCount = 0;
    bool hasSummary = false;
  };
  mlir::FailureOr<const Inventory *>
  prepare(mlir::Operation *module, mlir::Operation *site,
          const SourceReceiverProofFees &fees);
  mlir::FailureOr<SourceMethodView> method(mlir::Operation *module,
                                           llvm::StringRef name,
                                           mlir::Operation *site,
                                           const SourceReceiverProofFees &fees);

private:
  std::map<mlir::Operation *, Inventory> complete;
};

mlir::FailureOr<SourceMethodView>
lookupSourceMethod(mlir::Operation *module, llvm::StringRef name,
                   mlir::Operation *diagnosticSite);
mlir::FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call);
/// Resolve an actual call under bindings for its lexical caller module.
/// The lookup is borrowed only for this synchronous resolution. The receiver
/// occurrence path in the result remains relative to that caller.
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    SourceMethodCallOp call, const HardwareBindings &parentBindings,
    SourceModuleLookup lookup, SourceReceiverProofContext *context = nullptr,
    const SourceReceiverProofFees *fees = nullptr);
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    mlir::Operation *callerModule, mlir::DictionaryAttr callRow,
    const HardwareBindings &parentBindings, SourceModuleLookup lookup,
    mlir::Operation *diagnosticSite);

/// A source-only transparent registration of a shared Method body. The checked
/// capture ordinals bind explicit Method inputs; lexical captures remain owned
/// by the Method declaration. An absent marker preserves direct-body semantics.
struct SourceRegistrationEntryView {
  SourceMethodOp method;
  SourceMethodCallOp call;
  llvm::SmallVector<unsigned> argumentCaptureOrdinals;
};
mlir::FailureOr<std::optional<SourceRegistrationEntryView>>
getSourceRegistrationEntry(SourceTransactionOp transaction);

struct SourceMethodSummary {
  bool mayBlock = true;
  llvm::SmallVector<mlir::SymbolRefAttr> callees;
};

/// Facts derived from the current typed operations, never imported proof flags.
/// Body and interface views share call closure, recursion and totality proof.
/// Publication derivation remains a separate fallible request.
class SourceMethodSummariesAnalysis {
public:
  explicit SourceMethodSummariesAnalysis(mlir::Operation *package);
  /// The caller supplies native-verified canonical declarations, after checking
  /// primitive catalog authority, snapshot equality and interface dependencies.
  /// The lookup callback is borrowed only during this synchronous construction.
  SourceMethodSummariesAnalysis(
      llvm::ArrayRef<mlir::Operation *> moduleDeclarations,
      SourceModuleLookup lookup);
  bool isValid() const { return valid; }
  const SourceMethodSummary *lookup(SourceMethodOp method) const;
  const SourceMethodSummary *lookup(mlir::SymbolRefAttr method) const;
  bool isTotal(SourceTransactionOp transaction) const;

  /// Fallible direct publication facts; independent of totality admission.
  /// The cache, including failures, belongs to this analysis of the current IR.
  mlir::FailureOr<mlir::DictionaryAttr>
  getPublicationSummary(ModuleOp definition);
  mlir::FailureOr<bool> requiresTransactionPublication(ModuleOp definition);

private:
  llvm::DenseMap<mlir::Attribute, SourceMethodSummary> methods;
  llvm::DenseMap<mlir::Operation *, bool> transactions;
  llvm::DenseMap<mlir::Operation *, mlir::DictionaryAttr> publicationSummaries;
  mlir::Operation *package = nullptr;
  bool valid = false;
};

/// Check closed schema and local scope without consulting provider bodies.
mlir::LogicalResult
verifySourceTransactionSummary(mlir::Operation *moduleDeclaration,
                               mlir::DictionaryAttr summary);

/// Require the exact canonical summary rederived from the current provider.
mlir::LogicalResult
verifyPublishedTransactionSummary(ModuleOp definition,
                                  mlir::DictionaryAttr published,
                                  SourceMethodSummariesAnalysis &analysis);

} // namespace acir::ac

#endif
