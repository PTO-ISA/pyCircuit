#include "backend/issue_queue.hpp"

#include <sstream>

namespace davincioo::backend {

IssueQueue::IssueQueue(IssueQueueConfig config, PTOEngineKind kind, std::string name)
    : SimObject(std::move(name)),
      config_(config),
      kind_(kind) {}

void IssueQueue::BindEnqueueInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  enqueue_in_ = queue;
}

void IssueQueue::BindWakeupInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  wakeup_in_ = queue;
}

void IssueQueue::BindIssuedOutput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  issued_out_ = queue;
}

void IssueQueue::BindReadyChecker(std::function<bool(std::uint64_t)> checker) {
  ready_checker_ = std::move(checker);
}

std::size_t IssueQueue::Count() const {
  return entries_.size();
}

void IssueQueue::Build() {
  GFSIM_ASSERT(config_.entries > 0);
  GFSIM_ASSERT(enqueue_in_ != nullptr);
  GFSIM_ASSERT(wakeup_in_ != nullptr);
  GFSIM_ASSERT(issued_out_ != nullptr);
  GFSIM_ASSERT(static_cast<bool>(ready_checker_));
}

void IssueQueue::Reset() {
  entries_.clear();
}

bool IssueQueue::AllSourcesReady(const PTOInst& inst) const {
  for (std::size_t index = 0; index < inst.tile_input_valid.size(); ++index) {
    if (inst.tile_input_valid[index] && !inst.tile_input_ready[index]) {
      return false;
    }
  }
  return true;
}

void IssueQueue::RefreshSourceReady(PTOInst& inst) const {
  for (std::size_t index = 0; index < inst.tile_input_valid.size(); ++index) {
    if (!inst.tile_input_valid[index]) {
      inst.tile_input_ready[index] = true;
      continue;
    }
    const auto& input = inst.tile_reg_inputs[index];
    if (!input.rename_managed || input.tile_tag == kInvalidTileTag) {
      inst.tile_input_ready[index] = true;
      continue;
    }
    inst.tile_input_ready[index] = ready_checker_(input.tile_tag);
  }
  inst.src_ready = AllSourcesReady(inst);
}

bool IssueQueue::MatchesWakeupTag(const PTOInst& waiting, const PTOInst& completed) const {
  for (const auto& produced : completed.tile_reg_outputs) {
    if (!produced.rename_managed || produced.tile_tag == kInvalidTileTag) {
      continue;
    }
    for (std::size_t index = 0; index < waiting.tile_reg_inputs.size(); ++index) {
      if (!waiting.tile_input_valid[index] || waiting.tile_input_ready[index]) {
        continue;
      }
      const auto& input = waiting.tile_reg_inputs[index];
      if (input.rename_managed && input.tile_tag == produced.tile_tag) {
        return true;
      }
    }
  }
  return false;
}

void IssueQueue::ApplyWakeup(const PTOInstRef& completed) {
  GFSIM_ASSERT(completed != nullptr);
  for (auto& entry : entries_) {
    GFSIM_ASSERT(entry != nullptr);
    for (const auto& produced : completed->tile_reg_outputs) {
      if (!produced.rename_managed || produced.tile_tag == kInvalidTileTag) {
        continue;
      }
      for (std::size_t index = 0; index < entry->tile_reg_inputs.size(); ++index) {
        if (!entry->tile_input_valid[index] || entry->tile_input_ready[index]) {
          continue;
        }
        const auto& input = entry->tile_reg_inputs[index];
        if (input.rename_managed && input.tile_tag == produced.tile_tag) {
          entry->tile_input_ready[index] = true;
          this->MarkProgress();
        }
      }
    }
    entry->src_ready = AllSourcesReady(*entry);
  }
}

std::size_t IssueQueue::PickReadyIndex() const {
  std::size_t selected = entries_.size();
  std::uint64_t best_rob_id = std::numeric_limits<std::uint64_t>::max();
  for (std::size_t index = 0; index < entries_.size(); ++index) {
    const auto& entry = entries_[index];
    if (entry == nullptr || !entry->src_ready) {
      continue;
    }
    if (entry->rob_id < best_rob_id) {
      best_rob_id = entry->rob_id;
      selected = index;
    }
  }
  return selected;
}

void IssueQueue::Work() {
  GFSIM_ASSERT(enqueue_in_ != nullptr);
  GFSIM_ASSERT(wakeup_in_ != nullptr);
  GFSIM_ASSERT(issued_out_ != nullptr);

  while (!wakeup_in_->Empty()) {
    PTOInstRef completed = wakeup_in_->Read();
    ApplyWakeup(completed);
  }

  if (!enqueue_in_->Empty() && entries_.size() < config_.entries) {
    PTOInstRef inst = enqueue_in_->Read();
    GFSIM_ASSERT(inst != nullptr);
    RefreshSourceReady(*inst);
    entries_.push_back(inst);
    this->MarkProgress();
  }

  if (issued_out_->Full()) {
    return;
  }

  const std::size_t pick_index = PickReadyIndex();
  if (pick_index == entries_.size()) {
    return;
  }

  PTOInstRef issued = entries_[pick_index];
  GFSIM_ASSERT(issued != nullptr);
  issued->timestamps.issue_cycle = this->CurrentCycle();
  issued_out_->Write(issued);
  entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(pick_index));
  this->MarkProgress();
}

void IssueQueue::Xfer() {}

std::string IssueQueue::DumpState() const {
  std::ostringstream stream;
  stream << Name() << "(count=" << entries_.size() << ")";
  for (std::size_t index = 0; index < entries_.size(); ++index) {
    const auto& entry = entries_[index];
    if (entry == nullptr) {
      continue;
    }
    stream << "\n  slot=" << index
           << " src_ready=" << (entry->src_ready ? 1 : 0)
           << " inst={" << DumpPTOInst(*entry) << "}";
  }
  return stream.str();
}

}  // namespace davincioo::backend
