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
  StateType type;
  unsigned initial =
      TransactionalNoID; // This execution slice requires an explicit initial.
};
enum class TransactionalStepKind { Pure, StateRead, StateWrite, Assert };
struct TransactionalStep {
  TransactionalStepKind kind = TransactionalStepKind::Pure;
  mlir::Operation *operation = nullptr;
  unsigned id = 0;
  unsigned instance = 0;
  unsigned resource = TransactionalNoID;
  unsigned activation = TransactionalNoID;
  llvm::SmallVector<TransactionalValueID> operands, results;
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
  TransactionalSequence comb;
  llvm::SmallVector<unsigned> resources, rules;
};
struct TransactionalRulePlan {
  TransactionRuleCallOp registration;
  TransactionRuleOp definition;
  unsigned instance = 0;
  TransactionalSequence guard, body; // Empty guard yields means unconditional.
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
  // Ordered resource IDs parallel the selected Module's output names/types;
  // repetitions preserve exported aliases.
  llvm::ArrayRef<unsigned> getExports() const { return exports_; }
  mlir::FailureOr<unsigned> getPayloadWidth(mlir::Type type) const;

private:
  friend class TransactionalPlanBuilder;
  explicit TransactionalAnalysis(mlir::ModuleOp package) : package_(package) {}
  mlir::ModuleOp package_;
  std::vector<TransactionalInstancePlan> instances_;
  std::vector<TransactionalResourcePlan> resources_;
  std::vector<TransactionalRulePlan> rules_;
  std::vector<TransactionalValuePlan> values_;
  llvm::SmallVector<unsigned> exports_;
};
} // namespace acir::ac
#endif
