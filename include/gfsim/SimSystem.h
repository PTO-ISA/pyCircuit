#ifndef GFSIM_SIMSYSTEM_H
#define GFSIM_SIMSYSTEM_H

#include "gfsim/ObservationSlot.h"
#include "gfsim/SimModule.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace gfsim {

enum class SimSystemState { PreBuild, Built, Ready, Failed };
enum class SimStepResult { InvalidState, Running, Quiescent, Failed };
enum class SimFailurePhase { None, Build, Evaluate, Check, Drive, Xfer };

struct SimFailureInfo {
  SimFailurePhase phase = SimFailurePhase::None;
  std::string_view code;
  std::string_view message;
  std::string_view instance;
  std::string_view sourceJson;
  std::string_view checkIdJson;
};

class SimSystem {
public:
  virtual ~SimSystem() = default;

  bool AttachObservations(ObservationSlots &observations) noexcept {
    if (state_ != SimSystemState::PreBuild || observations_)
      return false;
    observations_ = &observations;
    return true;
  }

  bool HasObservations(const ObservationSlots &observations) const noexcept {
    return observations_ == &observations;
  }

  // Register a top-level module. Its nested children are reached through the
  // parent's callbacks, not additional entries in this system list.
  bool AddModule(SimModule &module) {
    if (state_ != SimSystemState::PreBuild ||
        std::find(roots_.begin(), roots_.end(), &module) != roots_.end())
      return false;
    roots_.push_back(&module);
    return true;
  }

  void Build() {
    if (state_ != SimSystemState::PreBuild)
      return;
    try {
      for (SimModule *module : roots_)
        module->Build();
      if (!FinalizeBuild()) {
        fail(SimFailurePhase::Build);
        return;
      }
      buildCompleted_ = true;
      state_ = SimSystemState::Built;
    } catch (...) {
      fail(SimFailurePhase::Build);
    }
  }

  SimStepResult Step() {
    if (state_ == SimSystemState::Failed)
      return SimStepResult::Failed;
    if (state_ != SimSystemState::Ready)
      return SimStepResult::InvalidState;

    bool hasWork = false;
    for (const SimModule *module : roots_)
      hasWork |= module->HasWork();
    if (!hasWork)
      return SimStepResult::Quiescent;

    // Time belongs to the system and observation records, not module Work.
    if (cycle_ == std::numeric_limits<std::uint64_t>::max()) {
      fail(SimFailurePhase::Evaluate);
      return SimStepResult::Failed;
    }

    try {
      Work();
    } catch (...) {
      failureInfo_ = DescribeFailure(SimFailurePhase::Evaluate);
      state_ = SimSystemState::Failed;
    }

    return Xfer() ? SimStepResult::Running : SimStepResult::Failed;
  }

  void Reset() noexcept {
    if (state_ != SimSystemState::Built && state_ != SimSystemState::Ready &&
        !(state_ == SimSystemState::Failed && buildCompleted_))
      return;

    discardAll();
    for (SimModule *module : roots_)
      module->Reset();
    for (SimModule *module : roots_)
      module->Xfer();
    if (observations_)
      observations_->Reset();
    cycle_ = 0;
    failureInfo_ = {};
    state_ = SimSystemState::Ready;
  }

  void ReportStat() {
    for (SimModule *module : roots_)
      module->ReportStat();
  }

  SimSystemState state() const noexcept { return state_; }
  std::uint64_t cycle() const noexcept { return cycle_; }
  SimFailureInfo failureInfo() const noexcept { return failureInfo_; }

protected:
  virtual bool FinalizeBuild() noexcept { return true; }
  virtual bool Precheck() noexcept = 0;
  virtual SimFailureInfo DescribeFailure(SimFailurePhase phase) const noexcept {
    switch (phase) {
    case SimFailurePhase::Build:
      return {phase, "runtime_failure", "model Build failed", {}, {}, {}};
    case SimFailurePhase::Evaluate:
      return {phase, "runtime_failure", "model evaluation failed", {}, {}, {}};
    case SimFailurePhase::Check:
      return {phase, "runtime_failure", "model check failed", {}, {}, {}};
    case SimFailurePhase::Drive:
      return {phase, "runtime_failure", "model drive failed", {}, {}, {}};
    case SimFailurePhase::Xfer:
      return {phase, "runtime_failure", "model transfer failed", {}, {}, {}};
    case SimFailurePhase::None:
      return {};
    }
    return {};
  }

private:
  void Work() {
    for (SimModule *module : roots_)
      module->Work();
  }

  bool Xfer() noexcept {
    if (state_ == SimSystemState::Failed) {
      discardAll();
      return false;
    }
    // Check the complete system before any root or nested child commits state.
    bool accepted = Precheck();
    if (observations_)
      accepted &= observations_->Precommit(cycle_);
    if (!accepted) {
      discardAll();
      failureInfo_ = DescribeFailure(SimFailurePhase::Check);
      state_ = SimSystemState::Failed;
      return false;
    }
    for (SimModule *module : roots_)
      module->Xfer();
    ++cycle_;
    if (observations_)
      observations_->Xfer(cycle_);
    return true;
  }

  void discardAll() noexcept {
    for (SimModule *module : roots_)
      module->DiscardNext();
    if (observations_)
      observations_->DiscardNext();
  }

  void fail(SimFailurePhase phase) noexcept {
    discardAll();
    failureInfo_ = DescribeFailure(phase);
    state_ = SimSystemState::Failed;
  }

  std::vector<SimModule *> roots_;
  ObservationSlots *observations_ = nullptr;
  SimSystemState state_ = SimSystemState::PreBuild;
  std::uint64_t cycle_ = 0;
  bool buildCompleted_ = false;
  SimFailureInfo failureInfo_;
};

} // namespace gfsim

#endif // GFSIM_SIM_SYSTEM_H
