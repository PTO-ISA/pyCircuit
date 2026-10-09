#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <functional>

using namespace mlir;
namespace acir::ac {

FailureOr<SmallVector<Operation *>>
HardwareAnalysis::getIndependentWorkSubtrees(ModuleOp definition) const {
  SmallVector<Operation *> result;
  HardwareBindings parent;
  parent.owner = definition;
  // Preparing pins must only read current state. Async memory Work also
  // diagnoses/stages effects, so keep such subtrees in the existing schedule.
  llvm::DenseSet<Operation *> active;
  std::function<bool(ModuleOp)> hasWorkDependentRead = [&](ModuleOp module) {
    if (!active.insert(module).second)
      return true;
    llvm::scope_exit cleanup([&] { active.erase(module); });
    for (Operation &op : module.getBody().front()) {
      if (isa<SourceObserveOp, SourceExpectOp>(op))
        return true;
      if (!isa<InstanceOp, CollectionOp>(op))
        continue;
      auto *callee = isa<InstanceOp>(op)
                         ? resolveCallee(cast<InstanceOp>(op))
                         : resolveCallee(cast<CollectionOp>(op));
      if (!callee || getPrimitiveKind(callee) == "byte_mem")
        return true;
      if (auto child = dyn_cast<ModuleOp>(callee))
        if (hasWorkDependentRead(child))
          return true;
    }
    return false;
  };
  for (Operation &op : definition.getBody().front()) {
    if (!isa<InstanceOp, CollectionOp>(op))
      continue;
    auto bindings = isa<InstanceOp>(op)
                        ? bindInstance(cast<InstanceOp>(op), parent)
                        : bindInstance(cast<CollectionOp>(op), parent);
    if (failed(bindings))
      return failure();
    auto child = dyn_cast<ModuleOp>(bindings->owner);
    if (!child || hasWorkDependentRead(child))
      continue;
    auto facts = analyzeModule(child, *bindings);
    if (failed(facts))
      return failure();
    llvm::DenseSet<Value> visited;
    std::function<bool(Value)> prepared = [&](Value value) {
      if (!visited.insert(value).second)
        return true;
      if (auto argument = dyn_cast<BlockArgument>(value)) {
        if (argument.getOwner() == &definition.getBody().front())
          return true;
        if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp()))
          return prepared(rule.getCaptures()[argument.getArgNumber()]);
        return false;
      }
      Operation *producer = value.getDefiningOp();
      if (isa_and_nonnull<InstanceOp, CollectionOp>(producer)) {
        if (producer != &op)
          return false;
        unsigned port = cast<OpResult>(value).getResultNumber();
        // Self feedback is independent only at an input-independent output
        // (a temporal cut). Preserve field-sensitive analysis conservatively:
        // every field of the referenced port must be independent.
        return llvm::none_of(facts->dependencies, [&](const auto &dependency) {
          return dependency.output.port == port && !dependency.inputs.empty();
        });
      }
      if (auto rule = dyn_cast_or_null<RuleOp>(producer))
        return prepared(
            cast<YieldOp>(rule.getBody().front().back())
                .getValues()[cast<OpResult>(value).getResultNumber()]);
      if (!producer || !isMemoryEffectFree(producer))
        return false;
      return llvm::all_of(producer->getOperands(), prepared);
    };
    if (llvm::all_of(op.getOperands(), prepared))
      result.push_back(&op);
  }
  return result;
}

} // namespace acir::ac
