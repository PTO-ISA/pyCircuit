#pragma once

#include <cstdint>
#include <memory>

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

namespace pyc {

// Summary of how a callee's output depends on its inputs within the tick/comb
// phase (i.e. across combinational logic only; sequential/stateful ops are
// cut points).
//
// Depth is expressed in "logic levels" using the same cost model as
// pyc-check-logic-depth:
// - 0 for wiring/aliases/constants
// - 1 for other combinational ops (approximate proxy)
// - instance boundaries are *not* cut points
struct CombResultSummary {
  llvm::BitVector argDeps;
  int64_t baseDepth = -1;                 // max depth from internal (non-arg) sources, -1 if none
  llvm::SmallVector<int64_t> argDepth{};  // max depth from each arg, -1 if unreachable
};

struct FuncCombSummary {
  unsigned numArgs = 0;
  unsigned numResults = 0;
  llvm::SmallVector<CombResultSummary> results;
};

// Cache for per-function combinational dependency summaries used by
// instance-aware verifiers (comb-cycle and logic-depth).
class CombDepGraphCache {
public:
  explicit CombDepGraphCache(mlir::ModuleOp module);

  // Returns nullptr on failure (and emits an error on the relevant op).
  const FuncCombSummary *getFuncSummary(mlir::func::FuncOp func);

private:
  mlir::ModuleOp module_;
  llvm::DenseMap<mlir::Operation *, std::unique_ptr<FuncCombSummary>> cache_;
  llvm::DenseSet<mlir::Operation *> inProgress_;
};

} // namespace pyc
