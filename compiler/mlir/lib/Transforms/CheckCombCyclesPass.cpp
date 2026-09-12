#include "pyc/Transforms/Passes.h"
#include "pyc/Transforms/CombDepGraph.h"

#include "pyc/Dialect/PYC/PYCOps.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

using namespace mlir;

namespace pyc {
namespace {

static bool isSequentialDef(Operation *op) {
  return isa<pyc::RegOp, pyc::FifoOp, pyc::ByteMemOp, pyc::SyncMemOp, pyc::SyncMemDPOp, pyc::AsyncFifoOp, pyc::CdcSyncOp>(op);
}

static std::string wireLabel(Value v) {
  if (auto w = v.getDefiningOp<pyc::WireOp>()) {
    if (auto n = w->getAttrOfType<StringAttr>("pyc.name"))
      return n.getValue().str();
  }
  std::string s;
  llvm::raw_string_ostream os(s);
  v.print(os);
  os.flush();
  return s;
}

static void collectWireReads(Value v,
                             const llvm::DenseSet<Value> &wires,
                             CombDepGraphCache &combCache,
                             llvm::DenseSet<Value> &out,
                             llvm::DenseSet<Value> &seen) {
  if (!v)
    return;
  if (!seen.insert(v).second)
    return;

  if (wires.contains(v)) {
    out.insert(v);
    return;
  }

  Operation *def = v.getDefiningOp();
  if (!def)
    return;

  if (isSequentialDef(def))
    return;

  if (auto inst = dyn_cast<pyc::InstanceOp>(def)) {
    unsigned resIdx = 0;
    if (auto r = dyn_cast<OpResult>(v))
      resIdx = r.getResultNumber();

    auto calleeAttr = inst->getAttrOfType<FlatSymbolRefAttr>("callee");
    const FuncCombSummary *calleeSum = nullptr;
    if (calleeAttr) {
      auto sym = SymbolTable::lookupSymbolIn(inst->getParentOfType<ModuleOp>(), calleeAttr.getValue());
      auto callee = dyn_cast_or_null<func::FuncOp>(sym);
      if (callee)
        calleeSum = combCache.getFuncSummary(callee);
    }

    if (calleeSum && resIdx < calleeSum->results.size()) {
      const auto &deps = calleeSum->results[resIdx].argDeps;
      auto inputs = inst.getInputs();
      unsigned n = std::min<unsigned>(static_cast<unsigned>(inputs.size()), deps.size());
      for (unsigned i = 0; i < n; ++i) {
        if (!deps.test(i))
          continue;
        collectWireReads(inputs[i], wires, combCache, out, seen);
      }
    }
    return;
  }

  if (auto comb = dyn_cast<pyc::CombOp>(def)) {
    for (Value in : comb.getInputs())
      collectWireReads(in, wires, combCache, out, seen);
    return;
  }

  for (Value opnd : def->getOperands())
    collectWireReads(opnd, wires, combCache, out, seen);
}

struct CheckCombCyclesPass : public PassWrapper<CheckCombCyclesPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CheckCombCyclesPass)

  StringRef getArgument() const override { return "pyc-check-comb-cycles"; }
  StringRef getDescription() const override {
    return "Detect combinational cycles involving pyc.wire/pyc.assign feedback without a pyc.reg break";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    CombDepGraphCache combCache(module);

    bool failedAny = false;

    for (func::FuncOp f : module.getOps<func::FuncOp>()) {
      llvm::DenseSet<Value> wires;
      f.walk([&](pyc::WireOp w) { wires.insert(w.getResult()); });

      // Build adjacency: wire(dst) -> wires read by its assign drivers.
      llvm::DenseMap<Value, llvm::DenseSet<Value>> deps;

      f.walk([&](pyc::AssignOp a) {
        Value dst = a.getDst();
        if (!wires.contains(dst))
          return;
        llvm::DenseSet<Value> reads;
        llvm::DenseSet<Value> seen;
        collectWireReads(a.getSrc(), wires, combCache, reads, seen);
        auto &s = deps[dst];
        for (Value r : reads)
          s.insert(r);
      });

      enum State : uint8_t { Unvisited = 0, Visiting = 1, Done = 2 };
      llvm::DenseMap<Value, State> state;
      llvm::SmallVector<Value> stack;

      auto dfs = [&](auto &&self, Value w) -> void {
        if (failedAny)
          return;
        state[w] = Visiting;
        stack.push_back(w);

        auto it = deps.find(w);
        if (it != deps.end()) {
          for (Value n : it->second) {
            if (failedAny)
              break;
            if (!wires.contains(n))
              continue;
            State st = state.lookup(n);
            if (st == Unvisited) {
              self(self, n);
              continue;
            }
            if (st == Visiting) {
              // Found a backedge: report a cycle slice from n..end.
              unsigned start = 0;
              for (; start < stack.size(); ++start) {
                if (stack[start] == n)
                  break;
              }
              llvm::SmallVector<Value> cycle;
              for (unsigned i = start; i < stack.size(); ++i)
                cycle.push_back(stack[i]);
              cycle.push_back(n);

              std::string msg;
              llvm::raw_string_ostream os(msg);
              os << "combinational cycle detected: ";
              for (unsigned i = 0; i < cycle.size(); ++i) {
                if (i)
                  os << " -> ";
                os << wireLabel(cycle[i]);
              }
              os.flush();

              f.emitError(msg);
              failedAny = true;
              break;
            }
          }
        }

        stack.pop_back();
        state[w] = Done;
      };

      for (Value w : wires) {
        if (state.lookup(w) == Unvisited)
          dfs(dfs, w);
        if (failedAny)
          break;
      }

      if (failedAny)
        break;
    }

    if (failedAny)
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<::mlir::Pass> createCheckCombCyclesPass() { return std::make_unique<CheckCombCyclesPass>(); }

static PassRegistration<CheckCombCyclesPass> pass;

} // namespace pyc
