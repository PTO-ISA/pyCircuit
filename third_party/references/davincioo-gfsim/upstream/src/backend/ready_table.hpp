#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo::backend {

class ReadyTable : public SimObject {
public:
  explicit ReadyTable(std::size_t issue_queue_count);

  void BindDispatchInput(std::size_t index, SimQueue<PTOInstRef>* queue);
  void BindIssueEnqueueOutput(std::size_t index, SimQueue<PTOInstRef>* queue);
  void BindIssueWakeupOutput(std::size_t index, SimQueue<PTOInstRef>* queue);
  void BindWakeupInput(SimQueue<PTOInstRef>* queue);
  void BindRobCompletionOutput(SimQueue<PTOInstRef>* queue);
  bool IsTagReady(std::uint64_t tag) const;

  void Build() override;
  void Reset() override;
  void Work() override;
  void Xfer() override;
  std::string DumpState() const;

private:
  void InitializeReadyState(PTOInst& inst);
  void MarkOutputsNotReady(const PTOInst& inst);
  void MarkOutputsReady(const PTOInst& inst);
  bool CanBroadcastWakeup() const;

  std::size_t issue_queue_count_ = 0;
  std::vector<SimQueue<PTOInstRef>*> dispatch_inputs_;
  std::vector<SimQueue<PTOInstRef>*> issue_enqueue_outputs_;
  std::vector<SimQueue<PTOInstRef>*> issue_wakeup_outputs_;
  SimQueue<PTOInstRef>* wakeup_in_ = nullptr;
  SimQueue<PTOInstRef>* rob_done_out_ = nullptr;
  std::set<std::uint64_t> ready_tags_;
};

}  // namespace davincioo::backend
