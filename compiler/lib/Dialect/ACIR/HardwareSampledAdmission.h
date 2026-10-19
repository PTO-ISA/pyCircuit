#ifndef PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H
#define PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <memory>
#include <optional>
#include <utility>

namespace acir::ac::detail {
class SourceCheckBudget;

enum class SampledControlKind : uint8_t { CallableGuard, BranchSelect, ValueAvailable, ResourceAvailable };
struct SampledValueRequirement {
  uint64_t control = 0, operand = 0;
  mlir::StringAttr kind;
  mlir::DictionaryAttr occurrence, location;
  ValueRequireOp operation;
};
struct SampledPathRef {
  std::optional<uint64_t> selectionControl;
  bool whenTrue = false;
};
struct SampledControlView {
  SampledControlKind kind;
  mlir::DictionaryAttr id;
  mlir::ArrayAttr invocation;
  int64_t predecessor = -1;
  SampledPathRef executionPath;
  uint64_t rawResult = 0, reachedResult = 0;
  mlir::Value raw, reached;
};
struct SampledBodyCheckView {
  SampledPathRef executionPath;
  mlir::DictionaryAttr id;
  mlir::ArrayAttr invocation;
  uint64_t conditionResult = 0, pathResult = 0;
  mlir::Value condition, path;
};
struct SampledObservationView {
  SampledPathRef executionPath;
  mlir::DictionaryAttr id;
  uint64_t pathResult = 0, firstValueResult = 0, valueCount = 0;
  mlir::Value path;
  llvm::SmallVector<mlir::Value> values;
};
struct SampledEhrPredecessor {
  mlir::DictionaryAttr registration;
  uint64_t dataCapture = 0, enableCapture = 0;
  mlir::Value data, enable;
};
struct SampledEhrEntry {
  mlir::DictionaryAttr stateID;
  uint64_t oldQCapture = 0, result = 0;
  mlir::Value oldQ, selected;
  llvm::SmallVector<SampledEhrPredecessor> predecessors;
};
struct SampledQueueProspectiveView {
  uint64_t capacityCapture = 0;
  std::optional<uint64_t> validCapture, externalPopCapture;
  llvm::SmallVector<uint64_t> priorPops;
};
struct SampledResourceView {
  uint64_t control = 0;
  mlir::StringAttr kind;
  mlir::DictionaryAttr owner, declaration, occurrence, location;
  std::optional<SampledQueueProspectiveView> prospective;
  std::optional<uint64_t> memoryPort;
};
struct SampledQueueProposalView {
  mlir::DictionaryAttr queue;
  mlir::StringAttr kind;
  mlir::DictionaryAttr occurrence, location;
  uint64_t rawPathResult = 0, pathResult = 0;
  mlir::Value rawPath, path;
  std::optional<uint64_t> resourceControl, dataResult;
  mlir::Value data;
  llvm::SmallVector<uint64_t> excludedControls;
};
struct SampledMemoryProposalView {
  mlir::DictionaryAttr memory;
  mlir::StringAttr kind;
  uint64_t port = 0, rawPathResult = 0, pathResult = 0;
  mlir::DictionaryAttr occurrence, location;
  mlir::Value rawPath, path;
  llvm::SmallVector<uint64_t> argumentResults;
  llvm::SmallVector<mlir::Value> arguments;
};
struct SampledWriteDomainView {
  uint64_t candidate = 0, maskResult = 0;
  mlir::ArrayAttr selectors;
  mlir::Value mask;
  mlir::DictionaryAttr stateID;
  mlir::Type ownerType;
  TableType table;
  mlir::ArrayAttr prefix, suffix;
};
struct SampledAdmissionView {
  ModuleOp definition;
  RuleOp rule;
  uint64_t clockCapture = 0, resetCapture = 0, fireResult = 0;
  uint64_t checkBase = 0, totalResults = 0;
  mlir::Value clock, reset, fire;
  llvm::SmallVector<SampledControlView> controls;
  llvm::SmallVector<SampledBodyCheckView> bodyChecks;
  llvm::SmallVector<SampledObservationView> observations;
  llvm::SmallVector<SampledEhrEntry> ehrEntries;
  llvm::SmallVector<SampledValueRequirement> requirements;
  llvm::SmallVector<SampledResourceView> resources;
  llvm::SmallVector<SampledQueueProposalView> queueProposals;
  llvm::SmallVector<SampledMemoryProposalView> memoryProposals;
  llvm::SmallVector<SampledWriteDomainView> writeDomains;
};
struct TransactionStateInventoryEntry {
  mlir::DictionaryAttr declaration;
  mlir::Operation *storage = nullptr;
};
struct TransactionIncomingView {
  mlir::DictionaryAttr stateID;
  uint64_t dataInput = 0, enableInput = 0;
  mlir::BlockArgument data, enable;
};
struct TransactionCandidateView {
  mlir::DictionaryAttr stateID;
  uint64_t dataIndex = 0, enableIndex = 0;
  mlir::OpResult data, enable;
};
struct TransactionWriterView {
  mlir::DictionaryAttr registration;
  RuleOp rule;
  uint64_t ordinaryResults = 0;
  llvm::SmallVector<TransactionCandidateView> candidates;
};
struct TransactionWiringView {
  ModuleOp definition;
  ModulePortLayout ports;
  llvm::SmallVector<TransactionStateInventoryEntry> states;
  llvm::SmallVector<TransactionIncomingView> incoming;
  llvm::SmallVector<TransactionWriterView> writers;
};

struct CheckedCandidateEnable {
  mlir::Value effectiveEnable, rawIntent;
};
struct HardwareIdentityScope {
  ModuleOp root;
  HardwareBindings rootBindings;
  llvm::SmallVector<InstanceOp> path;
};
struct HardwareIdentityEndpoint {
  mlir::Value value;
  HardwareIdentityScope scope;
  mlir::Type resolvedType;
};
enum class HardwareIdentityTraversal { DefinitionLocal, OrdinaryInstances };
struct HardwareIdentityStopPolicy {
  HardwareIdentityTraversal traversal;
  llvm::ArrayRef<HardwareIdentityEndpoint> boundaries;
};

/// Complete definition-local identity boundaries for one read-only validation.
/// The borrowed IR must outlive this result and remain unchanged. This is not
/// a cached verification verdict; successful prefix proofs have a separate
/// invocation-local owner below.
class PreparedSampledBoundaries {
public:
  PreparedSampledBoundaries(PreparedSampledBoundaries &&) = default;
  PreparedSampledBoundaries &operator=(PreparedSampledBoundaries &&) = default;
  PreparedSampledBoundaries(const PreparedSampledBoundaries &) = delete;
  PreparedSampledBoundaries &
  operator=(const PreparedSampledBoundaries &) = delete;

  const HardwareIdentityScope &getScope() const { return scope; }
  llvm::ArrayRef<HardwareIdentityEndpoint> getBoundaries() const {
    return boundaries;
  }
  mlir::LogicalResult verifyFor(ModuleOp definition,
                                const HardwareBindings &bindings,
                                SourceCheckBudget &budget) const;

private:
  PreparedSampledBoundaries(
      HardwareIdentityScope &&scope,
      llvm::SmallVector<HardwareIdentityEndpoint> &&boundaries);
  friend mlir::FailureOr<std::optional<PreparedSampledBoundaries>>
  prepareSampledBoundaries(ModuleOp definition,
                           const HardwareAnalysis &analysis,
                           const HardwareBindings &bindings,
                           SourceCheckBudget &budget);

  HardwareIdentityScope scope;
  llvm::SmallVector<HardwareIdentityEndpoint> boundaries;
};

mlir::FailureOr<std::optional<PreparedSampledBoundaries>>
prepareSampledBoundaries(ModuleOp definition, const HardwareAnalysis &analysis,
                         const HardwareBindings &bindings,
                         SourceCheckBudget &budget);

/// Reuses only definition-local preparation and completed prefix proofs.
/// The analysis, budget, package and complete boundaries must remain fixed for
/// this object's lifetime. Actual occurrence and root wiring proofs stay
/// separate.
class SampledAdmissionValidation {
public:
  SampledAdmissionValidation(const HardwareAnalysis &analysis,
                             SourceCheckBudget &budget);
  ~SampledAdmissionValidation();
  SampledAdmissionValidation(const SampledAdmissionValidation &) = delete;
  SampledAdmissionValidation &
  operator=(const SampledAdmissionValidation &) = delete;
  SampledAdmissionValidation(SampledAdmissionValidation &&) = delete;
  SampledAdmissionValidation &operator=(SampledAdmissionValidation &&) = delete;

  mlir::FailureOr<const PreparedSampledBoundaries *>
  prepare(ModuleOp definition, const HardwareBindings &bindings);
  /// Borrow immutable definition schema; physical occurrence proofs are
  /// separate.
  mlir::FailureOr<const TransactionWiringView *>
  wiring(ModuleOp definition, const HardwareBindings &bindings);
  mlir::FailureOr<SampledAdmissionView>
  verify(RuleOp rule, const HardwareBindings &bindings);
  mlir::FailureOr<llvm::SmallVector<CheckedCandidateEnable, 0>>
  verifyCandidateEnables(const TransactionWriterView &writer,
                         const HardwareBindings &bindings);
  mlir::LogicalResult verifyInvocation(const HardwareAnalysis &analysis,
                                       SourceCheckBudget &budget) const;
  /// Stable entry plus its owning container slot, for shared budget accounting.
  static uint64_t entryStorageBytes();
  /// Opaque intent context/records, shared with raw prepayment accounting.
  static uint64_t intentProofStorageBytes();
  static uint64_t intentNodeStorageBytes();
  static uint64_t intentRootStorageBytes();
  static uint64_t intentFrameStorageBytes();
  /// Actual private prefix-proof records, shared with raw prepayment.
  static uint64_t identityMemoEntryStorageBytes();
  static uint64_t prefixIndexEntryStorageBytes();
  static uint64_t prefixKnownAtomStorageBytes();
  static uint64_t prefixValueStorageBytes();
  static uint64_t prefixCacheStorageBytes();
  static uint64_t prefixConditionalStorageBytes();
  static uint64_t prefixFrameStorageBytes();
  static uint64_t prefixSelectionPathsStorageBytes();

private:
  mlir::FailureOr<SampledAdmissionView>
  verifySampledAdmissionImpl(RuleOp rule, const HardwareBindings &bindings);
  mlir::FailureOr<llvm::SmallVector<CheckedCandidateEnable, 0>>
  verifyCandidateEnablesImpl(const TransactionWriterView &writer,
                             const SampledAdmissionView &sampled,
                             const HardwareBindings &bindings);
  struct Entry;
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  llvm::SmallVector<std::unique_ptr<Entry>, 0> entries;
};

struct PhysicalStateEndpoint {
  HardwareIdentityScope ownerScope;
  ModuleOp owner;
  mlir::Operation *storage = nullptr;
  HardwareBindings ownerBindings, storageBindings;
  mlir::Type payloadType;
  mlir::Value clock, reset, enable, data, initial, q;
};

mlir::FailureOr<SampledAdmissionView>
parseSampledAdmission(RuleOp rule, const HardwareAnalysis &analysis,
                      const HardwareBindings &bindings,
                      SourceCheckBudget &budget);
mlir::FailureOr<TransactionWiringView>
parseTransactionWiring(ModuleOp module, const HardwareAnalysis &analysis,
                       const HardwareBindings &bindings,
                       SourceCheckBudget &budget);
mlir::FailureOr<PhysicalStateEndpoint>
resolveTransactionState(ModuleOp relativeTo, mlir::DictionaryAttr stateID,
                        const HardwareAnalysis &analysis,
                        const HardwareBindings &bindings,
                        SourceCheckBudget &budget);
mlir::FailureOr<HardwareIdentityEndpoint>
makeIdentityEndpoint(mlir::Value value, const HardwareIdentityScope &scope,
                     const HardwareAnalysis &analysis,
                     SourceCheckBudget &budget);
mlir::FailureOr<HardwareIdentityEndpoint>
normalizeIdentityEndpoint(const HardwareIdentityEndpoint &endpoint,
                          const HardwareIdentityStopPolicy &stops,
                          const HardwareAnalysis &analysis,
                          SourceCheckBudget &budget);
mlir::LogicalResult proveSampledAdmission(
    const SampledAdmissionView &view, const PreparedSampledBoundaries &prepared,
    const HardwareAnalysis &analysis, const HardwareBindings &bindings,
    SourceCheckBudget &budget);
mlir::LogicalResult verifyTransactionWiring(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    SampledAdmissionValidation &validation, SourceCheckBudget &budget);
} // namespace acir::ac::detail
#endif
