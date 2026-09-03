#include "ProcessStatePlanInternal.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FormatVariadic.h"

#include <map>

using namespace mlir;

namespace acir::detail {

namespace {

bool needsScalarStorageWrapper(mlir::Type type) {
  return mlir::isa<mlir::IntegerType, mlir::IndexType>(type);
}

} // namespace

LogicalResult
PlanSetBuilder::planProcessLiveness(ControlPlan &control,
                                    const ProcessStateLimits &limits) {
  auto makeWrapperOccurrence = [&](const ProcessPlannedValue &anchor,
                                   ProcessTransitionId transition,
                                   ProcessLiveSlotId slot,
                                   ProcessWrapperDirection direction) {
    auto wrapper = std::make_shared<ProcessSyntheticWrapperOccurrence::Impl>();
    wrapper->anchor = anchor.original().occurrence();
    wrapper->transition = transition;
    wrapper->slot = slot;
    wrapper->direction = direction;
    auto occurrence = std::make_shared<ProcessOccurrenceId::Impl>();
    occurrence->kind = ProcessOccurrenceKind::SyntheticWrapper;
    occurrence->syntheticWrapper = ProcessSyntheticWrapperOccurrence(wrapper);
    return ProcessOccurrenceId(occurrence);
  };
  auto makeSyntheticWrapperValue = [&](mlir::Type type,
                                       const ProcessOccurrenceId &occurrence,
                                       llvm::StringRef ownerPath) {
    auto coordinate = std::make_shared<ProcessValueCoordinate::Impl>();
    coordinate->kind = ProcessValueCoordinateKind::Result;
    coordinate->ownerPath = ownerPath.str();
    coordinate->index = 0;
    auto synthetic = std::make_shared<ProcessSyntheticPlannedValue::Impl>();
    synthetic->occurrence = occurrence;
    synthetic->coordinate = ProcessValueCoordinate(coordinate);
    auto value = std::make_shared<ProcessPlannedValue::Impl>();
    value->kind = ProcessPlannedValueKind::Synthetic;
    value->type = type;
    value->synthetic = ProcessSyntheticPlannedValue(synthetic);
    return ProcessPlannedValue(value);
  };
  auto makeLiveSlotValue = [&](mlir::Type type, ProcessLiveSlotId slot) {
    auto live = std::make_shared<ProcessLiveSlotPlannedValue::Impl>();
    live->slot = slot;
    auto value = std::make_shared<ProcessPlannedValue::Impl>();
    value->kind = ProcessPlannedValueKind::LiveSlot;
    value->type = type;
    value->liveSlot = ProcessLiveSlotPlannedValue(live);
    return ProcessPlannedValue(value);
  };
  auto renumberActions = [&](ProcessBlockPlan::Impl &block) {
    for (auto [index, action] : llvm::enumerate(block.actions))
      std::const_pointer_cast<ProcessActionPlan::Impl>(action.impl_)->id =
          index;
  };
  std::map<std::string, ProcessLiveSlotId> slotsByPath;
  llvm::DenseMap<ProcessBlockPlan::Impl *,
                 llvm::SmallVector<ProcessActionPlan, 2>>
      wrapsByBlock;
  llvm::DenseMap<ProcessBlockPlan::Impl *,
                 llvm::SmallVector<ProcessActionPlan, 2>>
      unwrapsByBlock;
  for (auto &transition : control.transitions) {
    if (transition->sourcePc == transition->targetPc)
      continue;
    auto sourceBlock = llvm::find_if(control.blocks, [&](const auto &block) {
      return block->pc == transition->sourcePc;
    });
    auto targetBlock = llvm::find_if(control.blocks, [&](const auto &block) {
      return block->pc == transition->targetPc;
    });
    if (sourceBlock == control.blocks.end() ||
        targetBlock == control.blocks.end())
      return failure();

    std::map<std::string, ProcessPlannedValue> definitions;
    for (const ProcessActionPlan &action : (*sourceBlock)->actions)
      for (const ProcessPlannedValue &result : action.results())
        if (result.kind() == ProcessPlannedValueKind::Original)
          definitions.emplace(result.original().path().str(), result);

    for (const ProcessActionPlan &action : (*targetBlock)->actions) {
      for (const ProcessPlannedValue &operand : action.operands()) {
        if (operand.kind() != ProcessPlannedValueKind::Original)
          continue;
        auto definition = definitions.find(operand.original().path().str());
        if (definition == definitions.end())
          continue;
        auto [slotIt, inserted] = slotsByPath.emplace(
            definition->first,
            ProcessLiveSlotId(static_cast<uint32_t>(control.liveSlots.size())));
        ProcessLiveSlotId slotId = slotIt->second;
        if (inserted) {
          if (control.liveSlots.size() >= limits.maxLiveSlots)
            return failure();
          auto slot = std::make_shared<ProcessLiveSlotPlan::Impl>();
          slot->id = slotId;
          slot->name = llvm::formatv("live{0:D8}", slotId.value()).str();
          slot->type = definition->second.type();
          slot->memberValues.push_back(definition->second);
          control.liveSlots.push_back(std::move(slot));
        }
        if (llvm::none_of(transition->stores, [&](const auto &store) {
              return store.impl_->slot == slotId;
            })) {
          ProcessPlannedValue storedSource = definition->second;
          if (needsScalarStorageWrapper(definition->second.type())) {
            ProcessOccurrenceId occurrence =
                makeWrapperOccurrence(definition->second, *transition->id,
                                      slotId, ProcessWrapperDirection::Wrap);
            ProcessPlannedValue wrapped =
                makeSyntheticWrapperValue(definition->second.type(), occurrence,
                                          definition->second.original().path());
            auto action = std::make_shared<ProcessActionPlan::Impl>();
            action->kind = ProcessActionKind::ScalarWrap;
            action->emission = ProcessEmissionClass::Wrap;
            action->occurrence = occurrence;
            action->operands = {definition->second};
            action->results = {wrapped};
            action->resultTypes = {definition->second.type()};
            action->cost = 1;
            wrapsByBlock[sourceBlock->get()].push_back(
                ProcessActionPlan(action));
            storedSource = wrapped;
          }
          auto store = std::make_shared<ProcessTransitionStorePlan::Impl>();
          store->slot = slotId;
          store->source = storedSource;
          if (definition->second.kind() == ProcessPlannedValueKind::Original)
            store->sourceValue = definition->second.original().value();
          transition->stores.push_back(ProcessTransitionStorePlan(store));
        }
        auto existingLoad = llvm::find_if(
            transition->loads, [&](const ProcessTransitionLoadPlan &load) {
              return load.impl_->slot == slotId;
            });
        if (existingLoad == transition->loads.end()) {
          auto load = std::make_shared<ProcessTransitionLoadPlan::Impl>();
          load->slot = slotId;
          if (needsScalarStorageWrapper(operand.type())) {
            ProcessPlannedValue loaded =
                makeLiveSlotValue(operand.type(), slotId);
            load->replacements.push_back(loaded);
            ProcessOccurrenceId occurrence =
                makeWrapperOccurrence(operand, *transition->id, slotId,
                                      ProcessWrapperDirection::Unwrap);
            auto action = std::make_shared<ProcessActionPlan::Impl>();
            action->kind = ProcessActionKind::ScalarUnwrap;
            action->emission = ProcessEmissionClass::Unwrap;
            action->occurrence = occurrence;
            action->operands = {loaded};
            action->results = {operand};
            action->resultTypes = {operand.type()};
            action->cost = 1;
            unwrapsByBlock[targetBlock->get()].push_back(
                ProcessActionPlan(action));
          } else {
            load->replacements.push_back(operand);
          }
          transition->loads.push_back(ProcessTransitionLoadPlan(load));
          (*targetBlock)->loads.push_back(ProcessTransitionLoadPlan(load));
        } else {
          auto load = std::const_pointer_cast<ProcessTransitionLoadPlan::Impl>(
              existingLoad->impl_);
          if (!needsScalarStorageWrapper(operand.type()))
            load->replacements.push_back(operand);
        }
      }
    }
  }
  for (auto &block : control.blocks) {
    auto unwraps = unwrapsByBlock.find(block.get());
    if (unwraps != unwrapsByBlock.end())
      block->actions.insert(block->actions.begin(), unwraps->second.begin(),
                            unwraps->second.end());
    auto wraps = wrapsByBlock.find(block.get());
    if (wraps != wrapsByBlock.end())
      block->actions.insert(block->actions.end(), wraps->second.begin(),
                            wraps->second.end());
    renumberActions(*block);
  }
  return success();
}

} // namespace acir::detail
