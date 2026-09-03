#include "ProcessStatePlanInternal.h"

#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"

#include <iomanip>
#include <memory>
#include <sstream>

using namespace mlir;

namespace acir::detail {
namespace {

static bool isSuspensionOp(Operation *op) {
  return isa<ac::WaitUntilOp>(op) || isa<ac::WaitForOp>(op) ||
         isa<ac::AwaitEventOp>(op) || isa<ac::YieldSimOp>(op);
}

static bool isYieldSim(Operation *op) { return isa<ac::YieldSimOp>(op); }

static ProcessWakeKind wakeKindForOp(Operation *op) {
  if (isa<ac::WaitUntilOp>(op))
    return ProcessWakeKind::Condition;
  if (isa<ac::WaitForOp>(op))
    return ProcessWakeKind::Resource;
  if (isa<ac::AwaitEventOp>(op))
    return ProcessWakeKind::EventQueue;
  if (isa<ac::YieldSimOp>(op))
    return ProcessWakeKind::NextDelta;
  llvm_unreachable("unknown suspension op");
}

static std::string wakeTypeKeyForOp(Operation *op) {
  if (isa<ac::WaitUntilOp>(op))
    return "@acir_wake_condition";
  if (isa<ac::WaitForOp>(op))
    return "@acir_wake_resource";
  if (isa<ac::AwaitEventOp>(op))
    return "@acir_wake_event_queue";
  if (isa<ac::YieldSimOp>(op))
    return "@acir_wake_next_delta";
  llvm_unreachable("unknown suspension op");
}

static std::string pcName(uint32_t index) {
  if (index == 0)
    return "entry";
  std::ostringstream s;
  s << "pc" << std::setfill('0') << std::setw(8) << index;
  return s.str();
}

static std::string blockPath(const std::string &defKey,
                             const std::string &pcNameStr, uint32_t blockIdx) {
  std::ostringstream s;
  s << defKey << "/plan/pc/" << pcNameStr << "/b" << std::setfill('0')
    << std::setw(8) << blockIdx;
  return s.str();
}

} // namespace

FailureOr<std::unique_ptr<PlanSetBuilder::ControlPlan>>
PlanSetBuilder::planProcessContinuation(const ExpandedProcess &expanded,
                                        const ProcessStateLimits &limits) {
  auto plan = std::make_unique<ControlPlan>();

  if (expanded.actions.empty())
    return mlir::failure();

  uint32_t nextPcId = 0;
  uint32_t nextBlockId = 0;
  uint32_t nextWakeId = 0;
  uint32_t nextTransitionId = 0;

  // Entry PC
  auto entryPc = std::make_shared<ProcessPcPlan::Impl>();
  entryPc->id = ProcessPcId(nextPcId++);
  entryPc->name = pcName(0);
  plan->pcs.push_back(entryPc);

  struct Susp {
    size_t idx;
    ProcessWakeKind kind;
    std::string typeKey;
    Operation *op;
  };
  SmallVector<Susp> suspensions;
  for (auto [i, a] : llvm::enumerate(expanded.actions)) {
    if (isSuspensionOp(a.operation))
      suspensions.push_back({i, wakeKindForOp(a.operation),
                             wakeTypeKeyForOp(a.operation), a.operation});
  }

  // Resume PCs
  uint32_t resumeIdx = 1;
  SmallVector<uint32_t> pcMap(expanded.actions.size(), 0);
  for (const auto &s : suspensions) {
    if (!isYieldSim(s.op)) {
      auto rpc = std::make_shared<ProcessPcPlan::Impl>();
      rpc->id = ProcessPcId(nextPcId);
      rpc->name = pcName(resumeIdx);
      plan->pcs.push_back(rpc);
      pcMap[s.idx] = nextPcId;
      ++nextPcId;
      ++resumeIdx;
    } else {
      pcMap[s.idx] = 0; // yield_sim resumes at entry
    }
  }

  // Segment starts
  SmallVector<size_t> starts;
  starts.push_back(0);
  for (const auto &s : suspensions)
    starts.push_back(s.idx + 1);

  for (size_t seg = 0; seg < suspensions.size(); ++seg) {
    size_t start = starts[seg];
    size_t end = suspensions[seg].idx;
    uint32_t pcId = (seg == 0) ? 0 : pcMap[suspensions[seg - 1].idx];
    const auto &susp = suspensions[seg];

    auto block = std::make_shared<ProcessBlockPlan::Impl>();
    block->id = ProcessBlockId(nextBlockId);
    block->pc = ProcessPcId(pcId);
    block->originBlock = expanded.actions[start].operation->getBlock();
    block->originRegion = block->originBlock->getParent();
    block->path =
        blockPath(expanded.definitionKey, plan->pcs[pcId]->name, nextBlockId);
    plan->pcs[pcId]->blocks.push_back(ProcessBlockId(nextBlockId));
    if (plan->pcs[pcId]->entryPath.empty())
      plan->pcs[pcId]->entryPath = block->path;

    // Actions in segment
    for (size_t i = start; i <= end; ++i) {
      auto act = std::make_shared<ProcessActionPlan::Impl>();
      act->id = static_cast<uint32_t>(i - start);
      act->kind = expanded.actions[i].kind;
      act->emission = ProcessEmissionClass::ForwardOnly;
      if (act->kind == ProcessActionKind::ForCondition ||
          act->kind == ProcessActionKind::ForIncrement)
        act->emission = ProcessEmissionClass::CopyScalar;
      act->occurrence = expanded.actions[i].occurrence;
      act->sourceOperation = act->kind == ProcessActionKind::Constant
                                 ? nullptr
                                 : expanded.actions[i].operation;
      act->iterationVector = expanded.actions[i].iterationVector;
      act->operands = expanded.actions[i].operands;
      act->results = expanded.actions[i].results;
      act->cost = act->emission == ProcessEmissionClass::ForwardOnly ? 0 : 1;
      for (const ProcessPlannedValue &result : act->results)
        act->resultTypes.push_back(result.type());
      act->scalarOp = expanded.actions[i].scalarOperation;
      if (act->kind == ProcessActionKind::Constant) {
        act->emission = ProcessEmissionClass::CopyScalar;
        act->cost = 1;
        auto scalar = std::make_shared<ProcessScalarOperationPlan::Impl>();
        scalar->name = "index.constant";
        scalar->properties = "{}";
        act->scalarOp = ProcessScalarOperationPlan(std::move(scalar));
      }
      if (act->kind == ProcessActionKind::Original && act->sourceOperation) {
        llvm::StringRef dialect =
            act->sourceOperation->getName().getDialectNamespace();
        if ((dialect == "arith" || dialect == "index" ||
             dialect == "builtin") &&
            act->sourceOperation->getNumRegions() == 0) {
          act->emission = ProcessEmissionClass::CopyScalar;
          act->cost = 1;
          if (!act->scalarOp) {
            auto scalar = std::make_shared<ProcessScalarOperationPlan::Impl>();
            scalar->name = act->sourceOperation->getName().getStringRef().str();
            scalar->properties = "{}";
            act->scalarOp = ProcessScalarOperationPlan(std::move(scalar));
          }
        }
      }
      block->actions.push_back(ProcessActionPlan(act));
    }

    // Suspension edge
    auto edge = std::make_shared<ProcessControlEdgePlan::Impl>();
    edge->kind = ProcessControlEdgeKind::Suspend;

    // Wake
    auto wake = std::make_shared<ProcessWakePlan::Impl>();
    wake->id = ProcessWakeId(nextWakeId);
    wake->kind = susp.kind;
    wake->typeKey = susp.typeKey;
    wake->operation = susp.op;
    wake->operationPath = expanded.actions[susp.idx].operationPath;
    wake->target = "";
    wake->occurrence = expanded.actions[susp.idx].occurrence;
    wake->iterationVector = expanded.actions[susp.idx].iterationVector;

    // Subscription sources
    for (const auto &opd : expanded.actions[susp.idx].operands) {
      auto src = std::make_shared<ProcessSubscriptionSourcePlan::Impl>();
      if (opd.kind() == ProcessPlannedValueKind::Original) {
        src->kind = ProcessSubscriptionSourceKind::Value;
        src->value = opd.original().value();
        src->owner = opd.original().occurrence().original().operation();
        src->path = opd.original().path().str();
      } else if (opd.kind() == ProcessPlannedValueKind::Capture) {
        src->kind = ProcessSubscriptionSourceKind::Capture;
        src->capture = opd.capture().capture();
      } else {
        src->kind = ProcessSubscriptionSourceKind::Value;
      }
      wake->sources.push_back(ProcessSubscriptionSourcePlan(src));
    }

    // Transition
    auto tr = std::make_shared<ProcessTransitionPlan::Impl>();
    tr->id = ProcessTransitionId(nextTransitionId);
    tr->sourcePc = ProcessPcId(pcId);
    tr->targetPc = ProcessPcId(pcMap[susp.idx]);
    tr->wake = ProcessWakeId(nextWakeId);

    edge->transition = ProcessTransitionId(nextTransitionId);
    block->edge = ProcessControlEdgePlan(edge);

    plan->blocks.push_back(block);
    plan->wakes.push_back(wake);
    plan->transitions.push_back(tr);

    ++nextWakeId;
    ++nextTransitionId;
    ++nextBlockId;
  }

  // A suspension nested directly in an scf.if does not dominate the
  // continuation after the if.  Materialize the branch in the current PC and
  // clone the post-if continuation for the non-suspending arm.  The original
  // continuation remains the resume PC for the suspending arm.
  //
  // Expansion deliberately keeps occurrence-qualified leaf actions, so this
  // repair operates on the immutable action records and never rewrites the
  // frozen source IR.
  ac::ProcessOp process = expanded.process;
  for (const Susp &susp : suspensions) {
    auto ifOp = susp.op->getParentOfType<scf::IfOp>();
    if (!ifOp || ifOp->getParentOp() != process.getOperation())
      continue;

    auto belongsTo = [&](Operation *operation, Region &region) {
      if (!operation)
        return false;
      Operation *nested = operation;
      while (nested && nested->getParentOp() != ifOp.getOperation())
        nested = nested->getParentOp();
      return nested && nested->getParentRegion() == &region;
    };
    bool suspendedInThen = belongsTo(susp.op, ifOp.getThenRegion());
    bool suspendedInElse = !ifOp.getElseRegion().empty() &&
                           belongsTo(susp.op, ifOp.getElseRegion());
    if (!suspendedInThen && !suspendedInElse)
      continue;
    Region *otherRegion =
        suspendedInThen ? &ifOp.getElseRegion() : &ifOp.getThenRegion();
    if (llvm::any_of(suspensions, [&](const Susp &candidate) {
          return candidate.op != susp.op && !otherRegion->empty() &&
                 belongsTo(candidate.op, *otherRegion);
        }))
      continue;

    auto branchBlockIt = llvm::find_if(plan->blocks, [&](const auto &block) {
      return llvm::any_of(block->actions, [&](const ProcessActionPlan &action) {
        return action.sourceOperation() == susp.op;
      });
    });
    if (branchBlockIt == plan->blocks.end() ||
        !(*branchBlockIt)->edge.has_value() ||
        (*branchBlockIt)->edge->kind() != ProcessControlEdgeKind::Suspend)
      continue;

    auto appendRenumbered = [](std::vector<ProcessActionPlan> &target,
                               const ProcessActionPlan &source) {
      auto action = std::make_shared<ProcessActionPlan::Impl>(*source.impl_);
      action->id = static_cast<uint32_t>(target.size());
      target.push_back(ProcessActionPlan(action));
    };
    std::vector<ProcessActionPlan> prefix;
    std::vector<ProcessActionPlan> thenActions;
    std::vector<ProcessActionPlan> elseActions;
    for (const ProcessActionPlan &action : (*branchBlockIt)->actions) {
      if (belongsTo(action.sourceOperation(), ifOp.getThenRegion()))
        appendRenumbered(thenActions, action);
      else if (!ifOp.getElseRegion().empty() &&
               belongsTo(action.sourceOperation(), ifOp.getElseRegion()))
        appendRenumbered(elseActions, action);
      else
        appendRenumbered(prefix, action);
    }
    std::vector<ProcessActionPlan> &suspendingActions =
        suspendedInThen ? thenActions : elseActions;
    std::vector<ProcessActionPlan> &continuingActions =
        suspendedInThen ? elseActions : thenActions;
    if (llvm::none_of(suspendingActions, [&](const ProcessActionPlan &action) {
          return action.sourceOperation() == susp.op;
        }))
      continue;

    ProcessTransitionId originalTransition =
        (*branchBlockIt)->edge->transition();
    if (originalTransition.value() >= plan->transitions.size())
      return failure();
    ProcessPcId resumePc =
        plan->transitions[originalTransition.value()]->targetPc.value();
    auto resumeBlockIt = llvm::find_if(plan->blocks, [&](const auto &block) {
      return block->pc == resumePc &&
             block->path == plan->pcs[resumePc.value()]->entryPath;
    });
    if (resumeBlockIt == plan->blocks.end() ||
        !(*resumeBlockIt)->edge.has_value())
      return failure();

    ProcessPcId branchPc = (*branchBlockIt)->pc.value();
    auto suspendingBlock = std::make_shared<ProcessBlockPlan::Impl>();
    suspendingBlock->id = ProcessBlockId(nextBlockId++);
    suspendingBlock->pc = branchPc;
    suspendingBlock->originBlock = susp.op->getBlock();
    suspendingBlock->originRegion = susp.op->getParentRegion();
    suspendingBlock->path =
        blockPath(expanded.definitionKey, plan->pcs[branchPc.value()]->name,
                  suspendingBlock->id->value());
    suspendingBlock->actions = std::move(suspendingActions);
    suspendingBlock->edge = (*branchBlockIt)->edge;

    auto continuingBlock = std::make_shared<ProcessBlockPlan::Impl>();
    continuingBlock->id = ProcessBlockId(nextBlockId++);
    continuingBlock->pc = branchPc;
    continuingBlock->originBlock =
        otherRegion->empty() ? ifOp->getBlock() : &otherRegion->front();
    continuingBlock->originRegion =
        otherRegion->empty() ? ifOp->getParentRegion() : otherRegion;
    continuingBlock->path =
        blockPath(expanded.definitionKey, plan->pcs[branchPc.value()]->name,
                  continuingBlock->id->value());
    continuingBlock->actions = std::move(continuingActions);
    for (const ProcessActionPlan &action : (*resumeBlockIt)->actions)
      appendRenumbered(continuingBlock->actions, action);

    const ProcessControlEdgePlan &resumeEdge = *(*resumeBlockIt)->edge;
    if (resumeEdge.kind() == ProcessControlEdgeKind::Suspend) {
      ProcessTransitionId resumeTransition = resumeEdge.transition();
      if (resumeTransition.value() >= plan->transitions.size())
        return failure();
      auto transition = std::make_shared<ProcessTransitionPlan::Impl>(
          *plan->transitions[resumeTransition.value()]);
      transition->id = ProcessTransitionId(nextTransitionId++);
      transition->sourcePc = branchPc;
      ProcessWakeId resumeWake = transition->wake.value();
      if (resumeWake.value() >= plan->wakes.size())
        return failure();
      auto wake = std::make_shared<ProcessWakePlan::Impl>(
          *plan->wakes[resumeWake.value()]);
      wake->id = ProcessWakeId(nextWakeId++);
      transition->wake = wake->id;
      auto edge = std::make_shared<ProcessControlEdgePlan::Impl>();
      edge->kind = ProcessControlEdgeKind::Suspend;
      edge->transition = transition->id;
      continuingBlock->edge = ProcessControlEdgePlan(edge);
      plan->wakes.push_back(std::move(wake));
      plan->transitions.push_back(std::move(transition));
    } else {
      continuingBlock->edge = resumeEdge;
    }

    std::optional<ProcessPlannedValue> condition;
    for (const ExpandedAction &action : expanded.actions) {
      for (const ProcessPlannedValue &result : action.results) {
        if (result.kind() == ProcessPlannedValueKind::Original &&
            result.original().value() == ifOp.getCondition()) {
          condition = result;
          break;
        }
      }
      if (condition)
        break;
    }
    if (!condition)
      return failure();

    auto edge = std::make_shared<ProcessControlEdgePlan::Impl>();
    edge->kind = ProcessControlEdgeKind::Branch;
    edge->condition = *condition;
    edge->trueBlock =
        suspendedInThen ? suspendingBlock->id : continuingBlock->id;
    edge->falseBlock =
        suspendedInThen ? continuingBlock->id : suspendingBlock->id;
    (*branchBlockIt)->actions = std::move(prefix);
    (*branchBlockIt)->edge = ProcessControlEdgePlan(edge);

    plan->pcs[branchPc.value()]->blocks.push_back(*suspendingBlock->id);
    plan->pcs[branchPc.value()]->blocks.push_back(*continuingBlock->id);
    plan->blocks.push_back(std::move(suspendingBlock));
    plan->blocks.push_back(std::move(continuingBlock));
  }

  return plan;
}

} // namespace acir::detail
