// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gfsim {

// One owner for a family of independent plain leaf states. Work borrows pins;
// Xfer visits every lane only after the whole family succeeded. No SimModule
// or virtual dispatch is created per lane. Output accessors borrow destination
// wires and must not throw; they do not transfer buffer ownership.
template <class Kernel> class collection_storage final {
public:
  explicit collection_storage(std::size_t count)
      : current_(count), pending_(count), completed_(count, false) {
    if (count == 0)
      throw std::invalid_argument("collection size must be positive");
    discard();
  }
  collection_storage(const collection_storage &) = delete;
  collection_storage &operator=(const collection_storage &) = delete;
  std::size_t size() const noexcept { return current_.size(); }
  const typename Kernel::Current &current(std::size_t lane) const {
    return current_.at(lane);
  }

  // Generated causal-wave execution uses private state and private output
  // wires until the complete root has passed its checks. The ordinary current
  // accessor continues to expose committed state throughout the macrostep.
  const typename Kernel::Current &wave_current(std::size_t lane) const {
    return macrostep_ ? overlay_.at(lane) : current_.at(lane);
  }
  void begin_macrostep() {
    begin_macrostep([](std::size_t, typename Kernel::Current &) {});
  }
  // Derived consumers seed their private clock history from the known old
  // source level, so an initially high divider does not fabricate an edge.
  // The generated root owns source identity and knownness checks.
  template <class SeedAccessor> void begin_macrostep(SeedAccessor &&seed) {
    if (macrostep_)
      throw std::logic_error("collection macrostep already active");
    // Copy before touching pending work: an allocation failure cannot change
    // either the committed state or the caller's existing candidate.
    overlay_ = current_;
    for (std::size_t lane = 0; lane < size(); ++lane)
      seed(lane, overlay_[lane]);
    macrostep_ = true;
    advanced_ = false;
    clear_pending();
  }

  void work(std::size_t lane, typename Kernel::Inputs inputs,
            typename Kernel::Outputs outputs) {
    try {
      if (lane >= size())
        throw std::out_of_range("collection lane out of range");
      Kernel::work(wave_current(lane), inputs, pending_[lane], outputs);
      if (!completed_[lane]) {
        completed_[lane] = true;
        ++completedCount_;
      }
    } catch (...) {
      discard();
      throw;
    }
  }

  template <class OutputAccessor> void xfer(OutputAccessor &&outputs) noexcept {
    if (macrostep_ || completedCount_ != size()) {
      discard();
      return;
    }
    for (std::size_t lane = 0; lane < size(); ++lane)
      Kernel::xfer(current_[lane], pending_[lane], outputs(lane));
    discard();
  }
  // Only call after the whole root's wave checks pass. Outputs must be the
  // generated private overlay wires, never public observations. A failed or
  // incomplete family invalidates the entire macrostep, including prior waves.
  template <class OutputAccessor>
  bool advance_wave(OutputAccessor &&outputs) noexcept {
    if (!macrostep_ || completedCount_ != size()) {
      discard();
      return false;
    }
    for (std::size_t lane = 0; lane < size(); ++lane)
      Kernel::xfer(overlay_[lane], pending_[lane], outputs(lane));
    advanced_ = true;
    clear_pending();
    return true;
  }
  // Root publication is allocation-free. The caller publishes its private
  // output snapshots and sampling histories at the same root barrier.
  bool macrostep_ready() const noexcept {
    return macrostep_ && advanced_ && completedCount_ == 0;
  }
  bool commit_macrostep() noexcept {
    if (!macrostep_ready()) {
      discard();
      return false;
    }
    current_.swap(overlay_);
    discard();
    return true;
  }
  void discard() noexcept {
    macrostep_ = false;
    advanced_ = false;
    overlay_.clear();
    clear_pending();
  }
  template <class InputAccessor> void reset(InputAccessor &&inputs) noexcept {
    discard();
    for (std::size_t lane = 0; lane < size(); ++lane) {
      Kernel::reset(current_[lane], inputs(lane), pending_[lane]);
      completed_[lane] = true;
    }
    completedCount_ = size();
  }

private:
  void clear_pending() noexcept {
    for (std::size_t lane = 0; lane < size(); ++lane) {
      Kernel::discard(wave_current(lane), pending_[lane]);
      completed_[lane] = false;
    }
    completedCount_ = 0;
  }
  std::vector<typename Kernel::Current> current_;
  // Allocated only for explicitly requested macrosteps. Legacy execution
  // retains its existing lane allocation and does not copy memory per Work.
  std::vector<typename Kernel::Current> overlay_;
  std::vector<typename Kernel::Pending> pending_;
  std::vector<bool> completed_;
  std::size_t completedCount_ = 0;
  bool macrostep_ = false;
  bool advanced_ = false;
};

} // namespace gfsim
