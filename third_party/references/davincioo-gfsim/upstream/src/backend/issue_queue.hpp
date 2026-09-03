#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <string>

#include "davincioo/model/config.hpp"
#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo::backend {

class IssueQueue : public SimObject {
public:
  IssueQueue(IssueQueueConfig config, PTOEngineKind kind, std::string name);

  void BindEnqueueInput(SimQueue<PTOInstRef>* queue);
  void BindWakeupInput(SimQueue<PTOInstRef>* queue);
  void BindIssuedOutput(SimQueue<PTOInstRef>* queue);
  void BindReadyChecker(std::function<bool(std::uint64_t)> checker);

  std::size_t Count() const;

  void Build() override;
  void Reset() override;
  void Work() override;
  void Xfer() override;
  std::string DumpState() const;

private:
  bool AllSourcesReady(const PTOInst& inst) const;
  void RefreshSourceReady(PTOInst& inst) const;
  bool MatchesWakeupTag(const PTOInst& waiting, const PTOInst& completed) const;
  void ApplyWakeup(const PTOInstRef& completed);
  std::size_t PickReadyIndex() const;

  IssueQueueConfig config_{};
  PTOEngineKind kind_ = PTOEngineKind::Scalar;
  SimQueue<PTOInstRef>* enqueue_in_ = nullptr;
  SimQueue<PTOInstRef>* wakeup_in_ = nullptr;
  SimQueue<PTOInstRef>* issued_out_ = nullptr;
  std::function<bool(std::uint64_t)> ready_checker_;
  std::deque<PTOInstRef> entries_;
};

}  // namespace davincioo::backend
