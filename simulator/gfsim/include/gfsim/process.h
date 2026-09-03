#ifndef GFSIM_PROCESS_H
#define GFSIM_PROCESS_H

#include "gfsim/object.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace gfsim {

enum class ProcessStatus : uint8_t {
  Runnable,
  Suspended,
  Terminated,
  Failed,
};

enum class ProcessWakeKind : uint8_t {
  Condition,
  Resource,
  EventQueue,
  NextDelta,
};

struct ProcessWake {
  ProcessWakeKind kind = ProcessWakeKind::Condition;
  uint32_t id = std::numeric_limits<uint32_t>::max();

  auto operator<=>(const ProcessWake &) const = default;
};

enum class ProcessStepKind : uint8_t {
  Continue,
  Suspend,
  Terminate,
  Fail,
};

struct ProcessStep {
  ProcessStepKind kind = ProcessStepKind::Fail;
  uint32_t nextPc = 0;
  std::optional<ProcessWake> wake;
  uint64_t continuationId = 0;
  std::string_view diagnostic = "invalid_process_step";

  static ProcessStep continueAt(uint32_t nextPc) {
    return {.kind = ProcessStepKind::Continue, .nextPc = nextPc};
  }

  static ProcessStep suspendAt(uint32_t nextPc, ProcessWake wake,
                               uint64_t continuationId) {
    return {.kind = ProcessStepKind::Suspend,
            .nextPc = nextPc,
            .wake = wake,
            .continuationId = continuationId,
            .diagnostic = {}};
  }

  static ProcessStep terminate() {
    return {.kind = ProcessStepKind::Terminate, .diagnostic = {}};
  }

  static ProcessStep fail(std::string_view diagnostic) {
    return {.kind = ProcessStepKind::Fail, .diagnostic = diagnostic};
  }
};

/// CRTP runtime for compiler-generated enum-PC processes. The generated
/// Derived::executeProcessStep call is statically bound on the hot path.
template <typename Derived> class ProcessRuntime : public SimObject {
public:
  ProcessRuntime(std::string name, ObjectId id, SimObject *parent,
                 uint32_t entryPc, uint64_t fairnessWork)
      : SimObject(ObjectKind::Process, std::move(name), id, parent),
        entryPc_(entryPc), committedPc_(entryPc), proposedPc_(entryPc),
        fairnessWork_(fairnessWork) {}

  void doWork(Epoch epoch) override {
    if (committedStatus_ != ProcessStatus::Runnable || pendingCommit_)
      return;

    if (traceEndRequested_) {
      proposedPc_ = committedPc_;
      proposedStatus_ = ProcessStatus::Terminated;
      proposedWake_.reset();
      proposedContinuationId_ = 0;
      proposedDiagnostic_ = {};
      pendingCommit_ = true;
      return;
    }

    uint32_t localPc = committedPc_;
    for (uint64_t work = 0; work < fairnessWork_; ++work) {
      ProcessStep step = derived().executeProcessStep(localPc, epoch);
      switch (step.kind) {
      case ProcessStepKind::Continue:
        localPc = step.nextPc;
        break;
      case ProcessStepKind::Suspend:
        if (!step.wake || step.continuationId == 0) {
          proposeFailure("invalid_process_continuation");
          return;
        }
        proposedPc_ = step.nextPc;
        proposedStatus_ = ProcessStatus::Suspended;
        proposedWake_ = step.wake;
        proposedContinuationId_ = step.continuationId;
        proposedDiagnostic_ = {};
        pendingCommit_ = true;
        return;
      case ProcessStepKind::Terminate:
        proposedPc_ = localPc;
        proposedStatus_ = ProcessStatus::Terminated;
        proposedWake_.reset();
        proposedContinuationId_ = 0;
        proposedDiagnostic_ = {};
        pendingCommit_ = true;
        return;
      case ProcessStepKind::Fail:
        proposeFailure(step.diagnostic.empty() ? "process_failed"
                                               : step.diagnostic);
        return;
      }
    }
    proposedPc_ = localPc;
    proposeFailure("process_fairness_exceeded");
  }

  void doXfer(Epoch) override {
    if (!pendingCommit_)
      return;
    committedPc_ = proposedPc_;
    committedStatus_ = proposedStatus_;
    committedWake_ = proposedWake_;
    committedContinuationId_ = proposedContinuationId_;
    committedDiagnostic_ = proposedDiagnostic_;
    pendingCommit_ = false;
    if (committedStatus_ == ProcessStatus::Terminated)
      traceEndRequested_ = false;
    if (committedStatus_ == ProcessStatus::Failed)
      setRuntimeFailureCode(committedDiagnostic_);
  }

  bool hasPendingCommit() const override { return pendingCommit_; }
  bool isRunnable(Epoch) const override {
    return committedStatus_ == ProcessStatus::Runnable && !pendingCommit_;
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.quiescent =
        !pendingCommit_ && (committedStatus_ == ProcessStatus::Terminated ||
                            committedStatus_ == ProcessStatus::Failed);
    if (state.quiescent) {
      state.reason.clear();
      return state;
    }
    if (pendingCommit_)
      state.reason = "pending_commit";
    else if (committedStatus_ == ProcessStatus::Runnable)
      state.reason = "process_runnable_unscheduled";
    else {
      state.reason = "process_suspended";
      if (committedWake_) {
        std::string kind;
        switch (committedWake_->kind) {
        case ProcessWakeKind::Condition:
          kind = "condition";
          break;
        case ProcessWakeKind::Resource:
          kind = "resource";
          break;
        case ProcessWakeKind::EventQueue:
          kind = "event_queue";
          break;
        case ProcessWakeKind::NextDelta:
          kind = "next_delta";
          break;
        }
        state.subscriptions.push_back(kind + ":" +
                                      std::to_string(committedWake_->id));
      }
    }
    return state;
  }

  bool wake(ProcessWake wake, uint64_t continuationId) {
    if (committedStatus_ != ProcessStatus::Suspended || !committedWake_ ||
        *committedWake_ != wake || committedContinuationId_ != continuationId)
      return false;
    committedStatus_ = ProcessStatus::Runnable;
    committedWake_.reset();
    return true;
  }

  bool requestTraceEnd() override {
    if (pendingCommit_ || committedStatus_ != ProcessStatus::Suspended ||
        !committedWake_ || committedWake_->kind != ProcessWakeKind::NextDelta)
      return false;
    traceEndRequested_ = true;
    committedStatus_ = ProcessStatus::Runnable;
    committedWake_.reset();
    return true;
  }

  uint32_t pc() const { return committedPc_; }
  ProcessStatus status() const { return committedStatus_; }
  uint64_t continuationId() const { return committedContinuationId_; }
  std::optional<ProcessWake> subscription() const { return committedWake_; }
  uint64_t fairnessWork() const { return fairnessWork_; }
  std::string_view diagnosticCode() const { return committedDiagnostic_; }

  bool validate() const {
    if (fairnessWork_ == 0)
      return false;
    if (committedStatus_ == ProcessStatus::Suspended)
      return committedWake_.has_value() && committedContinuationId_ != 0;
    return !committedWake_.has_value();
  }

  void reset() override {
    committedPc_ = entryPc_;
    proposedPc_ = entryPc_;
    committedStatus_ = ProcessStatus::Runnable;
    proposedStatus_ = ProcessStatus::Runnable;
    committedWake_.reset();
    proposedWake_.reset();
    committedContinuationId_ = 0;
    proposedContinuationId_ = 0;
    committedDiagnostic_ = {};
    proposedDiagnostic_ = {};
    pendingCommit_ = false;
    traceEndRequested_ = false;
    clearRuntimeFailureCode();
  }

private:
  Derived &derived() { return static_cast<Derived &>(*this); }

  void proposeFailure(std::string_view diagnostic) {
    proposedStatus_ = ProcessStatus::Failed;
    proposedWake_.reset();
    proposedContinuationId_ = 0;
    proposedDiagnostic_ = diagnostic;
    pendingCommit_ = true;
  }

  uint32_t entryPc_ = 0;
  uint32_t committedPc_ = 0;
  uint32_t proposedPc_ = 0;
  uint64_t fairnessWork_ = 0;
  ProcessStatus committedStatus_ = ProcessStatus::Runnable;
  ProcessStatus proposedStatus_ = ProcessStatus::Runnable;
  std::optional<ProcessWake> committedWake_;
  std::optional<ProcessWake> proposedWake_;
  uint64_t committedContinuationId_ = 0;
  uint64_t proposedContinuationId_ = 0;
  std::string_view committedDiagnostic_;
  std::string_view proposedDiagnostic_;
  bool pendingCommit_ = false;
  bool traceEndRequested_ = false;
};

} // namespace gfsim

#endif // GFSIM_PROCESS_H
