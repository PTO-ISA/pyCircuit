#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALANALYSIS_H

#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/SmallVector.h"
#include <functional>
#include <memory>
#include <vector>

namespace acir::ac {
using TransactionalValueID = unsigned;
inline constexpr unsigned TransactionalNoID = ~0u;
using TransactionalDebit =
    std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)>;

// The immutable plan borrows the verified linked source package. IDs denote
// actual allocations and SSA values, never mutable candidate-state versions.
struct TransactionalValuePlan {
  mlir::Type type;
  unsigned instance = 0;
  unsigned rule = TransactionalNoID;
};
struct TransactionalResourcePlan {
  ResourceAllocOp allocation;
  unsigned instance = 0;
  mlir::Type type;
  unsigned initial =
      TransactionalNoID; // State initial value; Queue starts empty.
  uint64_t depth = 0;
  bool isQueue() const { return mlir::isa<QueueType>(type); }
  mlir::Type getPayload() const {
    if (auto queue = mlir::dyn_cast<QueueType>(type))
      return queue.getPayload();
    return mlir::cast<StateType>(type).getPayload();
  }
};
enum class TransactionalStepKind {
  Pure,
  // A structured branch result. Operands are predicate, then value, else
  // value; availability follows only the selected arm.
  Merge,
  StateRead,
  StateWrite,
  QueueHead,
  QueuePop,
  QueuePush,
  Assert,
  Observe
};
struct TransactionalInvocationPlan {
  mlir::Operation *call = nullptr;
  unsigned instance = 0;
};
struct TransactionalPathTerm {
  TransactionalValueID value;
  bool positive;
};
struct TransactionalRequirement {
  unsigned value;
  llvm::SmallVector<TransactionalPathTerm> path;
};
// A call-site-specialized Method invocation. Step ranges preserve evaluation
// order, while ownership distinguishes nested calls from their caller's work.
struct TransactionalMethodInvocation {
  TransactionMethodCallOp call;
  TransactionMethodOp definition;
  unsigned instance = 0, rule = TransactionalNoID;
  unsigned parent = TransactionalNoID;
  bool inGuard = false;
  unsigned beginStep = 0, guardEndStep = 0, endStep = 0;
  llvm::SmallVector<TransactionalValueID> arguments, results, guardYields;
  llvm::SmallVector<TransactionalPathTerm> path;
  llvm::SmallVector<unsigned> guardSteps, bodySteps;
};
struct TransactionalStep {
  TransactionalStepKind kind = TransactionalStepKind::Pure;
  mlir::Operation *operation = nullptr;
  unsigned id = 0;
  unsigned instance = 0;
  unsigned resource = TransactionalNoID;
  // Implicit Rule loads with the same (rule, formal) denote one logical
  // access. Their selected paths form the demand union; explicit calls have
  // no formal identity and remain separate accesses even on an alias.
  unsigned ruleParameter = TransactionalNoID;
  unsigned rule = TransactionalNoID;
  unsigned activation = TransactionalNoID;
  unsigned methodInvocation = TransactionalNoID;
  llvm::SmallVector<TransactionalPathTerm> path;
  llvm::SmallVector<TransactionalValueID> operands, results;
  llvm::SmallVector<TransactionalInvocationPlan> invocation;
};
struct TransactionalSequence {
  TransactionalSequence() = default;
  TransactionalSequence(TransactionalSequence &&) noexcept = default;
  TransactionalSequence &operator=(TransactionalSequence &&) noexcept = default;
  TransactionalSequence(const TransactionalSequence &) = delete;
  TransactionalSequence &operator=(const TransactionalSequence &) = delete;
  std::vector<std::unique_ptr<TransactionalStep>> steps;
  llvm::SmallVector<TransactionalValueID> yields;
};
struct TransactionalInstancePlan {
  ModuleOp definition;
  InstanceOp declaration; // Empty for the selected root.
  unsigned parent = TransactionalNoID;
  llvm::SmallVector<unsigned> children;
  TransactionalSequence comb;
  llvm::SmallVector<unsigned> resources, rules;
};
struct TransactionalRulePlan {
  TransactionRuleCallOp registration;
  TransactionRuleOp definition;
  unsigned instance = 0;
  llvm::SmallVector<TransactionalPathTerm> path;
  TransactionalSequence guard, body; // Empty guard yields means unconditional.
  // Every actually invoked Method contributes its guard to the outer Rule.
  // These values may depend on earlier body computations, so effects execute
  // only after both sequences have supplied the complete admission predicate.
  llvm::SmallVector<TransactionalRequirement> requirements;
  llvm::SmallVector<TransactionalRequirement> controls;
  // Old occupancy requirements, including nonconsuming observations used by
  // this transaction. Consumption remains a distinct QueuePop effect.
  llvm::SmallVector<TransactionalRequirement> queueReads, queueWrites;
};
class TransactionalAnalysis final {
public:
  static mlir::FailureOr<std::unique_ptr<TransactionalAnalysis>>
  create(mlir::ModuleOp package, llvm::StringRef top, TransactionalDebit debit);
  mlir::ModuleOp getPackage() const { return package_; }
  ModuleOp getTop() const { return instances_.front().definition; }
  llvm::ArrayRef<TransactionalInstancePlan> getInstances() const {
    return instances_;
  }
  llvm::ArrayRef<TransactionalResourcePlan> getResources() const {
    return resources_;
  }
  llvm::ArrayRef<TransactionalRulePlan> getRules() const { return rules_; }
  llvm::ArrayRef<TransactionalValuePlan> getValues() const { return values_; }
  llvm::ArrayRef<TransactionalMethodInvocation> getMethodInvocations() const {
    return methodInvocations_;
  }
  // Ordered resource IDs parallel the selected Module's output names/types;
  // repetitions preserve exported aliases.
  llvm::ArrayRef<unsigned> getExports() const { return exports_; }
  mlir::FailureOr<unsigned> getPayloadWidth(mlir::Type type) const;

private:
  friend class TransactionalPlanBuilder;
  explicit TransactionalAnalysis(mlir::ModuleOp package) : package_(package) {}
  mlir::ModuleOp package_;
  mlir::OwningOpRef<mlir::ModuleOp> initializers_;
  std::vector<TransactionalInstancePlan> instances_;
  std::vector<TransactionalResourcePlan> resources_;
  std::vector<TransactionalRulePlan> rules_;
  std::vector<TransactionalValuePlan> values_;
  std::vector<TransactionalMethodInvocation> methodInvocations_;
  llvm::SmallVector<unsigned> exports_;
};
} // namespace acir::ac
#endif
