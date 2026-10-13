#ifndef PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H
#define PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <memory>
#include <optional>
#include <utility>

namespace acir::ac::detail {
class SourceCheckBudget;

struct SampledGuardView {
  mlir::DictionaryAttr id;
  mlir::ArrayAttr invocation;
  int64_t predecessor = -1;
  uint64_t predicateResult = 0, reachedResult = 0;
  mlir::Value predicate, reached;
};
struct SampledAdmissionView {
  ModuleOp definition;
  RuleOp rule;
  uint64_t clockCapture = 0, resetCapture = 0, fireResult = 0;
  mlir::Value clock, reset, fire;
  llvm::SmallVector<SampledGuardView> guards;
};
struct TransactionStateInventoryEntry {
  mlir::DictionaryAttr declaration;
  InstanceOp storage;
};
struct TransactionIncomingView {
  mlir::DictionaryAttr stateID;
  uint64_t dataInput = 0, enableInput = 0;
  mlir::BlockArgument data, enable;
};
struct TransactionCandidateView {
  mlir::DictionaryAttr stateID;
  uint64_t resultIndex = 0;
  mlir::OpResult result;
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
  mlir::FailureOr<SampledAdmissionView>
  verify(RuleOp rule, const HardwareBindings &bindings);
  mlir::LogicalResult verifyInvocation(const HardwareAnalysis &analysis,
                                       SourceCheckBudget &budget) const;
  /// Stable entry plus its owning container slot, for shared budget accounting.
  static uint64_t entryStorageBytes();

private:
  struct Entry;
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  llvm::SmallVector<std::unique_ptr<Entry>, 0> entries;
};

struct PhysicalStateEndpoint {
  HardwareIdentityScope ownerScope;
  ModuleOp owner;
  InstanceOp storage;
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
