#include "ProcessStatePlanInternal.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <limits>
#include <optional>

using namespace mlir;

namespace acir::detail {

LogicalResult
PlanSetBuilder::planProcessCost(ControlPlan &control,
                                const ProcessStateLimits &limits) {
  // Compute exact per-block cost from the contract formula:
  //   block_cost = sum(entry loads, each 1)
  //              + sum(scalar_unwrap actions, each 1)
  //              + sum(copy_scalar, inline, invoke, for_condition,
  //                    for_increment leaf actions, each 1)
  //              + if edge is suspend:
  //                  sum(scalar_wrap actions, each 1)
  //                + sum(store emissions, each 1)
  //                + 1 (wake invoke)
  //                + 1 (acsim.suspend)
  //                otherwise: 1 (cf.cond_br, cf.br, or terminate)
  //
  // Fairness = iterative max sum of block costs over every path
  // in every PC-local DAG. Reject zero, overflow, cycles, cap excess.
  //
  // For yield-only: block cost = 2 (wake invoke + acsim.suspend)

  for (auto &block : control.blocks) {
    block->cost = 0;

    // Count entry loads. Scalar unwraps are explicit actions below.
    block->cost += static_cast<uint64_t>(block->loads.size());

    // Count scalar_unwrap, copy_scalar, inline, invoke actions
    for (const auto &action : block->actions) {
      if (action.kind() == ProcessActionKind::ScalarWrap ||
          action.kind() == ProcessActionKind::ScalarUnwrap ||
          action.emission() == ProcessEmissionClass::CopyScalar ||
          action.emission() == ProcessEmissionClass::Inline ||
          action.emission() == ProcessEmissionClass::Invoke ||
          action.kind() == ProcessActionKind::ForCondition ||
          action.kind() == ProcessActionKind::ForIncrement) {
        block->cost += 1;
      }
    }

    // Edge cost
    if (block->edge.has_value() &&
        block->edge->kind() == ProcessControlEdgeKind::Suspend) {
      ProcessTransitionId transition = block->edge->transition();
      if (transition.value() >= control.transitions.size())
        return failure();
      block->cost += control.transitions[transition.value()]->stores.size();
      // Wake invoke + acsim.suspend
      block->cost += 2;
    } else if (block->edge.has_value()) {
      block->cost += 1; // cf.cond_br, cf.br, or terminate
    }
  }

  // Compute the longest path in each PC-local DAG with Kahn traversal.  Block
  // IDs are dense global ordinals, while each PC owns a canonical ordered
  // subset of those IDs.
  uint64_t fairness = 0;
  for (const auto &pc : control.pcs) {
    if (pc->blocks.empty())
      return failure();
    SmallVector<uint32_t> indegree(control.blocks.size());
    SmallVector<std::optional<uint64_t>> distance(control.blocks.size());
    auto successors = [](const ProcessControlEdgePlan &edge) {
      SmallVector<ProcessBlockId, 2> result;
      if (edge.kind() == ProcessControlEdgeKind::Branch) {
        result.push_back(edge.trueBlock());
        result.push_back(edge.falseBlock());
      } else if (edge.kind() == ProcessControlEdgeKind::LocalContinue) {
        result.push_back(edge.targetBlock());
      }
      return result;
    };
    for (ProcessBlockId id : pc->blocks) {
      if (id.value() >= control.blocks.size() ||
          control.blocks[id.value()]->pc != pc->id)
        return failure();
      for (ProcessBlockId successor :
           successors(*control.blocks[id.value()]->edge)) {
        if (successor.value() >= control.blocks.size() ||
            control.blocks[successor.value()]->pc != pc->id)
          return failure();
        ++indegree[successor.value()];
      }
    }

    SmallVector<ProcessBlockId> ready;
    for (ProcessBlockId id : pc->blocks)
      if (indegree[id.value()] == 0)
        ready.push_back(id);
    ProcessBlockId entry = pc->blocks.front();
    distance[entry.value()] = control.blocks[entry.value()]->cost;
    size_t processed = 0;
    uint64_t pcWork = 0;
    for (size_t cursor = 0; cursor < ready.size(); ++cursor) {
      ProcessBlockId id = ready[cursor];
      ++processed;
      if (distance[id.value()])
        pcWork = std::max(pcWork, *distance[id.value()]);
      for (ProcessBlockId successor :
           successors(*control.blocks[id.value()]->edge)) {
        if (distance[id.value()]) {
          uint64_t successorCost = control.blocks[successor.value()]->cost;
          if (*distance[id.value()] >
              std::numeric_limits<uint64_t>::max() - successorCost)
            return failure();
          uint64_t candidate = *distance[id.value()] + successorCost;
          if (!distance[successor.value()] ||
              candidate > *distance[successor.value()])
            distance[successor.value()] = candidate;
        }
        if (--indegree[successor.value()] == 0)
          ready.push_back(successor);
      }
    }
    if (processed != pc->blocks.size() ||
        llvm::any_of(pc->blocks, [&](ProcessBlockId id) {
          return !distance[id.value()].has_value();
        }))
      return failure();
    fairness = std::max(fairness, pcWork);
  }

  if (fairness == 0)
    return control.blocks.front()->originBlock
               ? control.blocks.front()
                     ->originBlock->getParentOp()
                     ->emitOpError("process fairness must be non-zero")
               : failure();

  if (fairness > limits.maxFairnessWork)
    return failure();
  control.fairnessWork = fairness;

  return success();
}

} // namespace acir::detail
