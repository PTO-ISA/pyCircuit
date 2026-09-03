#include "backend/rob.hpp"

#include <sstream>

namespace davincioo::backend {

namespace {

bool ShouldRaiseException(const PTOInst& inst) {
  return inst.opcode == PTOOpcode::Unknown;
}

}  // namespace

ROB::ROB(ROBConfig config)
    : SimObject("rob"),
      config_(config) {}

void ROB::BindInstInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  inst_in_ = queue;
}

void ROB::BindCompletionInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  done_in_ = queue;
}

void ROB::BindRenameOutput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  rename_out_ = queue;
}

void ROB::BindRetireOutput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  retire_out_ = queue;
}

const std::vector<PTOInst>& ROB::Processed() const {
  return processed_;
}

const std::vector<ROBEntry>& ROB::Entries() const {
  return entries_;
}

std::string ROB::DumpState() const {
  std::ostringstream stream;
  stream << "ROB(head=" << head_
         << ", tail=" << tail_
         << ", count=" << count_
         << ", capacity=" << entries_.size() << ")";
  for (std::size_t index = 0; index < entries_.size(); ++index) {
    const auto& entry = entries_[index];
    if (entry.status == ROBEntryStatus::Free || entry.inst == nullptr) {
      continue;
    }
    stream << "\n  slot=" << index
           << " rid=" << entry.rid
           << " status=" << ToString(entry.status)
           << " inst={" << DumpPTOInst(*entry.inst) << "}";
  }
  return stream.str();
}

std::size_t ROB::Capacity() const {
  return entries_.size();
}

std::size_t ROB::Count() const {
  return count_;
}

bool ROB::Full() const {
  return count_ >= entries_.size();
}

bool ROB::Empty() const {
  return count_ == 0;
}

void ROB::Build() {
  GFSIM_ASSERT(config_.entries > 0);
  GFSIM_ASSERT((config_.entries & (config_.entries - 1)) == 0);
  GFSIM_ASSERT(inst_in_ != nullptr);
  GFSIM_ASSERT(done_in_ != nullptr);
  GFSIM_ASSERT(rename_out_ != nullptr);
  GFSIM_ASSERT(retire_out_ != nullptr);
  entries_.assign(config_.entries, ROBEntry{});
}

void ROB::Reset() {
  processed_.clear();
  for (ROBEntry& entry : entries_) {
    entry.rid = 0;
    entry.status = ROBEntryStatus::Free;
    entry.inst.reset();
  }
  head_ = 0;
  tail_ = 0;
  count_ = 0;
  next_rid_ = 0;
}

bool ROB::CanAlloc() const {
  return count_ < entries_.size();
}

void ROB::Allocate(PTOInstRef inst) {
  GFSIM_ASSERT(inst != nullptr);
  GFSIM_ASSERT(CanAlloc());
  ROBEntry& entry = entries_[tail_];
  GFSIM_ASSERT(entry.status == ROBEntryStatus::Free);

  entry.rid = next_rid_++;
  entry.status = ROBEntryStatus::Alloc;
  entry.inst = std::move(inst);
  entry.inst->rob_id = entry.rid;
  entry.inst->engine_id = std::numeric_limits<std::uint64_t>::max();
  entry.inst->dispatched = false;
  entry.inst->src_ready = false;
  entry.inst->runtime_latency = 0;
  entry.inst->timestamps.rob_alloc_cycle = this->CurrentCycle();
  entry.inst->timestamps.rename_cycle = kUnsetCycleStamp;
  entry.inst->timestamps.dispatch_cycle = kUnsetCycleStamp;
  entry.inst->timestamps.engine_pop_cycle = kUnsetCycleStamp;
  entry.inst->timestamps.engine_complete_cycle = kUnsetCycleStamp;
  entry.inst->timestamps.rob_retire_cycle = kUnsetCycleStamp;
  entry.inst->output_replaced_tile_tags.assign(entry.inst->tile_reg_outputs.size(), kInvalidTileTag);
  for (auto& reg : entry.inst->tile_reg_inputs) {
    reg.tile_tag = kInvalidTileTag;
    reg.rename_managed = false;
  }
  for (auto& reg : entry.inst->tile_reg_outputs) {
    reg.tile_tag = kInvalidTileTag;
    reg.rename_managed = false;
  }

  tail_ = (tail_ + 1) % entries_.size();
  ++count_;
  this->MarkProgress();
}

void ROB::ConsumeCompletion(const PTOInstRef& inst) {
  GFSIM_ASSERT(inst != nullptr);
  for (ROBEntry& entry : entries_) {
    if (entry.status != ROBEntryStatus::Issued || entry.inst == nullptr) {
      continue;
    }
    if (entry.rid != inst->rob_id) {
      continue;
    }
    entry.status = ShouldRaiseException(*inst) ? ROBEntryStatus::Exception : ROBEntryStatus::Completed;
    entry.inst->runtime_latency = 0;
    entry.inst->timestamps.engine_complete_cycle = inst->timestamps.engine_complete_cycle;
    this->MarkProgress();
    return;
  }
}

void ROB::AllocateMultipleEntries(std::size_t max_count) {
  std::size_t allocated = 0;
  while (allocated < max_count && !inst_in_->Empty() && CanAlloc()) {
    Allocate(inst_in_->Read());
    ++allocated;
  }
}

void ROB::IssueMultipleEntries(std::size_t max_count) {
  std::size_t issued = 0;
  for (std::size_t offset = 0; offset < entries_.size() && issued < max_count; ++offset) {
    const std::size_t idx = (head_ + offset) % entries_.size();
    ROBEntry& entry = entries_[idx];
    if (entry.status != ROBEntryStatus::Alloc || entry.inst == nullptr) {
      continue;
    }
    if (rename_out_->Full()) {
      return;
    }

    rename_out_->Write(entry.inst);
    entry.status = ROBEntryStatus::Issued;
    this->MarkProgress();
    ++issued;
  }
}

void ROB::SetAllocWidth(std::size_t width) {
  GFSIM_ASSERT(width > 0);
  alloc_width_ = width;
}

void ROB::SetIssueWidth(std::size_t width) {
  GFSIM_ASSERT(width > 0);
  issue_width_ = width;
}

void ROB::RetireHead() {
  while (count_ > 0) {
    ROBEntry& entry = entries_[head_];
    if (entry.status != ROBEntryStatus::Completed && entry.status != ROBEntryStatus::Exception) {
      break;
    }
    if (entry.inst != nullptr && retire_out_->Full()) {
      break;
    }

    if (entry.inst != nullptr) {
      entry.inst->timestamps.rob_retire_cycle = this->CurrentCycle();
      retire_out_->Write(entry.inst);
      processed_.push_back(*entry.inst);
      entry.inst.reset();
    }
    entry.status = ROBEntryStatus::Resolved;
    entry.status = ROBEntryStatus::Free;
    head_ = (head_ + 1) % entries_.size();
    --count_;
    this->MarkProgress();
  }
}

void ROB::Work() {
  GFSIM_ASSERT(inst_in_ != nullptr);
  GFSIM_ASSERT(done_in_ != nullptr);
  GFSIM_ASSERT(rename_out_ != nullptr);
  GFSIM_ASSERT(retire_out_ != nullptr);

  if (!done_in_->Empty()) {
    ConsumeCompletion(done_in_->Read());
  }
  RetireHead();
  AllocateMultipleEntries(alloc_width_);
  IssueMultipleEntries(issue_width_);
}

void ROB::Xfer() {}

}  // namespace davincioo::backend
