#pragma once
#include "sources/rv32i_core/core.hpp"
using pyc_root = ::rv32i_core::core::Core;
#include <gfsim/SimSystem.h>
#include <stdexcept>
class pyc_dut final {
public:
  struct Inputs {
    decltype(std::declval<pyc_root>().bus) bus;
    decltype(std::declval<pyc_root>().pyc_7079635f636c6b) pyc_7079635f636c6b;
    decltype(std::declval<pyc_root>().pyc_7079635f727374) pyc_7079635f727374;
  };
  struct Outputs {
    decltype(std::declval<pyc_root>().result_0) result_0;
    decltype(std::declval<pyc_root>().result_1) result_1;
  };
  explicit pyc_dut(std::size_t workers = 1) : executor_(workers), root_(std::make_shared<pyc_root>("root", &executor_)), system_(*root_) {
    if (!observations_.Configure({}, 0) || !system_.AttachObservations(observations_) || !system_.AddModule(*root_)) throw std::logic_error("DUT setup failed");
  }
  pyc_dut(const pyc_dut &) = delete;
  pyc_dut &operator=(const pyc_dut &) = delete;
  // Control-thread access only, between sampling epochs.
  void drive(const Inputs &inputs) {
    root_->bus = inputs.bus;
    root_->pyc_7079635f636c6b = inputs.pyc_7079635f636c6b;
    root_->pyc_7079635f727374 = inputs.pyc_7079635f727374;
  }
  Outputs sample() const {
    if (system_.state() != gfsim::SimSystemState::Ready || system_.cycle() == 0) throw std::logic_error("no successful DUT sample");
    return {root_->result_0, root_->result_1};
  }
  gfsim::SimSystem &system() { return system_; }
  gfsim::ObservationSlots &observations() { return observations_; }
private:
  class System final : public gfsim::SimSystem {
  public:
    explicit System(pyc_root &root) noexcept : root_(root) {}
  private:
    bool Precheck() noexcept override {
      source_failure_ = {};
      return root_.pyc_validate_checks(source_failure_);
    }
    gfsim::SimFailureInfo DescribeFailure(gfsim::SimFailurePhase phase) const noexcept override {
      if (phase == gfsim::SimFailurePhase::Check && !source_failure_.code.empty()) return source_failure_;
      return gfsim::SimSystem::DescribeFailure(phase);
    }
    pyc_root &root_;
    gfsim::SimFailureInfo source_failure_{};
  };
  gfsim::WorkExecutor executor_;
  std::shared_ptr<pyc_root> root_;
  gfsim::ObservationSlots observations_;
  System system_;
};
