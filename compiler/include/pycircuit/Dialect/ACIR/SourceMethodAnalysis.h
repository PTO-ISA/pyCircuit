#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H

#include "mlir/IR/SymbolTable.h"
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

/// Compile-time Module reference contracts. Receivers retain declaration-relative
/// symbolic references until link resolves their actual allocation identities.
mlir::LogicalResult verifySourceReferenceParameters(mlir::Operation *module);
mlir::FailureOr<mlir::Operation *> resolveSourceReferenceReceiver(
    mlir::Operation *module, mlir::DictionaryAttr receiver,
    SourceModuleLookup lookup, mlir::Operation *site);
mlir::LogicalResult verifySourceReferenceBindings(
    mlir::Operation *module, mlir::Operation *callee, mlir::Attribute bindings,
    SourceModuleLookup lookup, mlir::Operation *site);

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
  mlir::ArrayAttr getFormalProposals() const {
    if (isNativeTerminal())
      return {};
    return definition
               ? definition->getAttrOfType<mlir::ArrayAttr>("ac.formal_proposals")
               : row.getAs<mlir::ArrayAttr>("ac.formal_proposals");
  }
  /// Number of authored results, excluding checked trailing [data, intent]
  /// pairs for writable formals. Requires a verified Method contract.
  unsigned getNumPublicResults() const {
    auto proposals = getFormalProposals();
    return getFunctionType().getNumResults() -
           2 * (proposals ? proposals.size() : 0);
  }

mlir::FunctionType getFunctionType() const {
  if (isNativeMemoryTerminal()) {
    auto signature = cast<mlir::FunctionType>(module->getAttrOfType<mlir::TypeAttr>("function_type").getValue());
    auto kind = module->getAttrOfType<mlir::StringAttr>("primitive_kind").getValue();
    unsigned address = kind == "byte_mem" ? 2 : 3 + 2 * nativeMemoryPort;
    unsigned write = kind == "byte_mem" ? 3 : kind == "sync_mem_dp" ? 6 : 4;
    llvm::SmallVector<mlir::Type> inputs, results;
    if (nativeMemoryAction == NativeMemoryAction::Request ||
        nativeMemoryAction == NativeMemoryAction::Read)
      inputs.push_back(signature.getInput(address));
    else if (nativeMemoryAction == NativeMemoryAction::Write)
      for (unsigned ordinal : {write + 1, write + 2, write + 3})
        inputs.push_back(signature.getInput(ordinal));
    if (nativeMemoryAction == NativeMemoryAction::Result ||
        nativeMemoryAction == NativeMemoryAction::Read)
      results.push_back(signature.getResult(nativeMemoryPort));
    return mlir::FunctionType::get(module->getContext(), inputs, results);
  }
  if (isNativeQueueTerminal()) {
    auto moduleType = cast<mlir::FunctionType>(
        module->getAttrOfType<mlir::TypeAttr>("function_type").getValue());
    mlir::Type formalPayload = moduleType.getInput(3);
    llvm::SmallVector<mlir::Type> inputs, results;
    (nativeQueueAction == NativeQueueAction::Push ? inputs : results)
        .push_back(formalPayload);
    return mlir::FunctionType::get(module->getContext(), inputs, results);
  }
  return definition ? cast<mlir::FunctionType>(
                          definition->getAttrOfType<mlir::TypeAttr>("function_type")
                              .getValue())
                    : cast<mlir::FunctionType>(
                          row.getAs<mlir::TypeAttr>("function_type").getValue());
}
mlir::ArrayAttr getArgumentNames() const {
  if (isNativeMemoryTerminal()) {
    mlir::Builder builder(module->getContext());
    if (nativeMemoryAction == NativeMemoryAction::Result) return builder.getArrayAttr({});
    if (nativeMemoryAction == NativeMemoryAction::Write)
      return builder.getStrArrayAttr({"address", "value", "mask"});
    return builder.getStrArrayAttr({"address"});
  }
  if (isNativeQueueTerminal()) {
    mlir::Builder builder(module->getContext());
    return nativeQueueAction == NativeQueueAction::Push
               ? builder.getArrayAttr({builder.getStringAttr("value")})
               : builder.getArrayAttr({});
  }
  return definition ? definition->getAttrOfType<mlir::ArrayAttr>("argument_names")
                    : row.getAs<mlir::ArrayAttr>("argument_names");
}
mlir::ArrayAttr getParameters() const {
  if (isNativeMemoryTerminal()) {
    mlir::Builder builder(module->getContext());
    auto signature = getFunctionType();
    auto names = getArgumentNames();
    llvm::SmallVector<mlir::Attribute> parameters;
    for (auto [ordinal, type] : llvm::enumerate(signature.getInputs())) {
      mlir::Attribute constraint = nativeMemoryAction == NativeMemoryAction::Write && ordinal == 1
          ? nativeMemoryPayloadConstraint
          : builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
              builder.getNamedAttr("type", mlir::TypeAttr::get(type)),
              builder.getNamedAttr("source_kind", builder.getStringAttr("fixed_bits"))});
      parameters.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("name", names[ordinal]),
          builder.getNamedAttr("binding", builder.getStringAttr("positional_or_keyword")),
          builder.getNamedAttr("constraint", constraint),
          builder.getNamedAttr("default", builder.getDictionaryAttr({
              builder.getNamedAttr("present", builder.getBoolAttr(false))}))}));
    }
    return builder.getArrayAttr(parameters);
  }
  if (isNativeQueueTerminal()) {
    mlir::Builder builder(module->getContext());
    if (nativeQueueAction != NativeQueueAction::Push)
      return builder.getArrayAttr({});
    return builder.getArrayAttr({builder.getDictionaryAttr({
        builder.getNamedAttr("name", builder.getStringAttr("value")),
        builder.getNamedAttr("binding",
                             builder.getStringAttr("positional_or_keyword")),
        builder.getNamedAttr("constraint", nativeQueuePayloadConstraint),
        builder.getNamedAttr(
            "default",
            builder.getDictionaryAttr({builder.getNamedAttr(
                "present", builder.getBoolAttr(false))}))})});
  }
  return definition ? definition->getAttrOfType<mlir::ArrayAttr>("ac.parameters")
                    : row.getAs<mlir::ArrayAttr>("ac.parameters");
}
mlir::ArrayAttr getResultConstraints() const {
  if (isNativeMemoryTerminal()) {
    mlir::Builder builder(module->getContext());
    return getFunctionType().getNumResults()
        ? builder.getArrayAttr({nativeMemoryPayloadConstraint}) : builder.getArrayAttr({});
  }
  if (isNativeQueueTerminal()) {
    mlir::Builder builder(module->getContext());
    return nativeQueueAction == NativeQueueAction::Push
               ? builder.getArrayAttr({})
               : builder.getArrayAttr({nativeQueuePayloadConstraint});
  }
  return definition
             ? definition->getAttrOfType<mlir::ArrayAttr>("ac.result_constraints")
             : row.getAs<mlir::ArrayAttr>("ac.result_constraints");
}
mlir::StringAttr getReturnForm() const {
  if (isNativeMemoryTerminal())
    return mlir::StringAttr::get(module->getContext(), getFunctionType().getNumResults() ? "single" : "none");
  if (isNativeQueueTerminal())
    return mlir::StringAttr::get(module->getContext(),
                           nativeQueueAction == NativeQueueAction::Push
                               ? "none"
                               : "single");
  return definition ? definition->getAttrOfType<mlir::StringAttr>("ac.return_form")
                    : row.getAs<mlir::StringAttr>("ac.return_form");
}
mlir::StringAttr getVisibility() const {
  if (isNativeTerminal())
    return mlir::StringAttr::get(module->getContext(), "public");
  return definition ? definition->getAttrOfType<mlir::StringAttr>("sym_visibility")
                    : row.getAs<mlir::StringAttr>("visibility");
}
mlir::DictionaryAttr getDeclaration() const {
  if (isNativeMemoryTerminal())
    return nativeMemoryInstance ? nativeMemoryInstance->getAttrOfType<mlir::DictionaryAttr>("occurrence")
        : nativeMemoryOwnerRow.getAs<mlir::DictionaryAttr>("occurrence");
  if (isNativeQueueTerminal())
    return nativeQueueInstance
               ? nativeQueueInstance->getAttrOfType<mlir::DictionaryAttr>(
                     "occurrence")
                               : nativeQueueOwnerRow.getAs<mlir::DictionaryAttr>(
                                     "occurrence");
  return definition ? definition->getAttrOfType<mlir::DictionaryAttr>("occurrence")
                    : row.getAs<mlir::DictionaryAttr>("declaration");
}
mlir::DictionaryAttr getGuardLocation() const {
  if (isNativeTerminal())
    return {};
  return definition
             ? definition->getAttrOfType<mlir::DictionaryAttr>("ac.guard_location")
             : row.getAs<mlir::DictionaryAttr>("guard_location");
}
mlir::SymbolRefAttr getSymbol() const {
  if (isNativeMemoryTerminal()) {
    std::string name = nativeMemoryAction == NativeMemoryAction::Request ? "request" :
        nativeMemoryAction == NativeMemoryAction::Result ? "result" :
        nativeMemoryAction == NativeMemoryAction::Read ? "read" : "write";
    if (nativeMemoryAction == NativeMemoryAction::Request || nativeMemoryAction == NativeMemoryAction::Result)
      name += std::to_string(nativeMemoryPort);
    return mlir::SymbolRefAttr::get(module->getContext(), mlir::SymbolTable::getSymbolName(module).getValue(),
        {mlir::FlatSymbolRefAttr::get(module->getContext(), name)});
  }
  if (isNativeQueueTerminal()) {
    llvm::StringRef name = nativeQueueAction == NativeQueueAction::Push
                         ? "push"
                         : nativeQueueAction == NativeQueueAction::Pop ? "pop"
                                                                       : "peek";
    return mlir::SymbolRefAttr::get(module->getContext(),
                              mlir::SymbolTable::getSymbolName(module).getValue(),
                              {mlir::FlatSymbolRefAttr::get(module->getContext(),
                                                     name)});
  }
  auto name = definition ? definition->getAttrOfType<mlir::StringAttr>("sym_name")
                         : row.getAs<mlir::StringAttr>("name");
  return mlir::SymbolRefAttr::get(module->getContext(),
                            mlir::SymbolTable::getSymbolName(module).getValue(),
                            {mlir::FlatSymbolRefAttr::get(name)});
}
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

/// Closed declaration-relative selector; never an allocation or formal actual.
mlir::LogicalResult verifySourceRelativeReceiver(
    mlir::DictionaryAttr receiver, mlir::Operation *diagnosticSite,
    const SourceReceiverProofFees *fees = nullptr);
/// Resolve only the nominal public, concrete Method contract. Actual receiver
/// selection and fresh binding remain the invocation analysis owner's proof.
mlir::FailureOr<ResolvedSourceMethod> resolveSourceRelativeMethodContract(
    mlir::Operation *caller, mlir::SymbolRefAttr callee,
    mlir::DictionaryAttr receiver, SourceModuleLookup lookup,
    mlir::Operation *diagnosticSite,
    SourceReceiverProofContext *context = nullptr,
    const SourceReceiverProofFees *fees = nullptr);
mlir::LogicalResult verifySourceRelativeMethodBinding(
    const ResolvedSourceMethod &expected, const SourceMethodView &actual,
    const HardwareBindings &actualBindings, mlir::Operation *diagnosticSite,
    const SourceReceiverProofFees *fees = nullptr);

mlir::FailureOr<SourceMethodView>
lookupSourceMethod(mlir::Operation *module, llvm::StringRef name,
                   mlir::Operation *diagnosticSite);
mlir::FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call);
/// Resolve a call under bindings for its lexical caller module. Relative calls
/// establish only declaration authority, retaining the selector for invocation
/// analysis. Other receiver paths remain relative to the lexical caller.
/// The lookup is borrowed only for this synchronous resolution.
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    SourceMethodCallOp call, const HardwareBindings &parentBindings,
    SourceModuleLookup lookup, SourceReceiverProofContext *context = nullptr,
    const SourceReceiverProofFees *fees = nullptr);
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    mlir::Operation *callerModule, mlir::DictionaryAttr callRow,
    const HardwareBindings &parentBindings, SourceModuleLookup lookup,
    mlir::Operation *diagnosticSite,
    const SourceReceiverProofFees *fees = nullptr);

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
