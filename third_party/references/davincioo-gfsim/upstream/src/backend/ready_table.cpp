#include "backend/ready_table.hpp"

#include <sstream>

namespace davincioo::backend {

namespace {

bool AllTileInputsReady(const PTOInst& inst) {
  for (std::size_t index = 0; index < inst.tile_input_valid.size(); ++index) {
    if (inst.tile_input_valid[index] && !inst.tile_input_ready[index]) {
      return false;
    }
  }
  return true;
}

}  // namespace

ReadyTable::ReadyTable(std::size_t issue_queue_count)
    : SimObject("ready_table"),
      issue_queue_count_(issue_queue_count),
      dispatch_inputs_(issue_queue_count, nullptr),
      issue_enqueue_outputs_(issue_queue_count, nullptr),
      issue_wakeup_outputs_(issue_queue_count, nullptr) {}

void ReadyTable::BindDispatchInput(std::size_t index, SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(index < dispatch_inputs_.size());
  GFSIM_ASSERT(queue != nullptr);
  dispatch_inputs_[index] = queue;
}

void ReadyTable::BindIssueEnqueueOutput(std::size_t index, SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(index < issue_enqueue_outputs_.size());
  GFSIM_ASSERT(queue != nullptr);
  issue_enqueue_outputs_[index] = queue;
}

void ReadyTable::BindIssueWakeupOutput(std::size_t index, SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(index < issue_wakeup_outputs_.size());
  GFSIM_ASSERT(queue != nullptr);
  issue_wakeup_outputs_[index] = queue;
}

void ReadyTable::BindWakeupInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  wakeup_in_ = queue;
}

void ReadyTable::BindRobCompletionOutput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  rob_done_out_ = queue;
}

bool ReadyTable::IsTagReady(std::uint64_t tag) const {
  return ready_tags_.contains(tag);
}

void ReadyTable::Build() {
  GFSIM_ASSERT(wakeup_in_ != nullptr);
  GFSIM_ASSERT(rob_done_out_ != nullptr);
  for (std::size_t index = 0; index < issue_queue_count_; ++index) {
    GFSIM_ASSERT(dispatch_inputs_[index] != nullptr);
    GFSIM_ASSERT(issue_enqueue_outputs_[index] != nullptr);
    GFSIM_ASSERT(issue_wakeup_outputs_[index] != nullptr);
  }
}

void ReadyTable::Reset() {
  ready_tags_.clear();
}

void ReadyTable::InitializeReadyState(PTOInst& inst) {
  inst.tile_input_valid.assign(inst.tile_reg_inputs.size(), false);
  inst.tile_input_ready.assign(inst.tile_reg_inputs.size(), true);
  for (std::size_t index = 0; index < inst.tile_reg_inputs.size(); ++index) {
    const auto& reg = inst.tile_reg_inputs[index];
    if (!reg.rename_managed || reg.tile_tag == kInvalidTileTag) {
      inst.tile_input_valid[index] = false;
      inst.tile_input_ready[index] = true;
      continue;
    }
    inst.tile_input_valid[index] = true;
    inst.tile_input_ready[index] = ready_tags_.contains(reg.tile_tag);
  }
  inst.src_ready = AllTileInputsReady(inst);
}

void ReadyTable::MarkOutputsNotReady(const PTOInst& inst) {
  for (const auto& reg : inst.tile_reg_outputs) {
    if (!reg.rename_managed || reg.tile_tag == kInvalidTileTag) {
      continue;
    }
    ready_tags_.erase(reg.tile_tag);
  }
}

void ReadyTable::MarkOutputsReady(const PTOInst& inst) {
  for (const auto& reg : inst.tile_reg_outputs) {
    if (!reg.rename_managed || reg.tile_tag == kInvalidTileTag) {
      continue;
    }
    ready_tags_.insert(reg.tile_tag);
  }
}

bool ReadyTable::CanBroadcastWakeup() const {
  if (rob_done_out_->Full()) {
    return false;
  }
  for (auto* queue : issue_wakeup_outputs_) {
    if (queue->Full()) {
      return false;
    }
  }
  return true;
}

void ReadyTable::Work() {
  if (!wakeup_in_->Empty()) {
    PTOInstRef completed = wakeup_in_->Front();
    GFSIM_ASSERT(completed != nullptr);
    if (!CanBroadcastWakeup()) {
      return;
    }
    completed = wakeup_in_->Read();
    MarkOutputsReady(*completed);
    for (auto* queue : issue_wakeup_outputs_) {
      queue->Write(completed);
    }
    rob_done_out_->Write(completed);
    this->MarkProgress();
  }

  for (std::size_t index = 0; index < issue_queue_count_; ++index) {
    auto* dispatch_in = dispatch_inputs_[index];
    auto* enqueue_out = issue_enqueue_outputs_[index];
    if (dispatch_in->Empty() || enqueue_out->Full()) {
      continue;
    }

    PTOInstRef inst = dispatch_in->Front();
    GFSIM_ASSERT(inst != nullptr);
    inst = dispatch_in->Read();
    InitializeReadyState(*inst);
    MarkOutputsNotReady(*inst);
    enqueue_out->Write(inst);
    this->MarkProgress();
  }
}

void ReadyTable::Xfer() {}

std::string ReadyTable::DumpState() const {
  std::ostringstream stream;
  stream << "ready_table(tags={";
  bool first = true;
  for (const std::uint64_t tag : ready_tags_) {
    if (!first) {
      stream << ",";
    }
    first = false;
    stream << tag;
  }
  stream << "})";
  return stream.str();
}

}  // namespace davincioo::backend
