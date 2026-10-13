#ifndef PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H
#define PYCIRCUIT_HARDWARE_SAMPLED_ADMISSION_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
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
mlir::LogicalResult proveSampledAdmission(const SampledAdmissionView &view,
                                          const HardwareAnalysis &analysis,
                                          const HardwareBindings &bindings,
                                          SourceCheckBudget &budget);
mlir::LogicalResult verifyTransactionWiring(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    SourceCheckBudget &budget);
} // namespace acir::ac::detail
#endif
