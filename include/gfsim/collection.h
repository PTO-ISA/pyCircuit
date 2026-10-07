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

  void work(std::size_t lane, typename Kernel::Inputs inputs,
            typename Kernel::Outputs outputs) {
    try {
      if (lane >= size())
        throw std::out_of_range("collection lane out of range");
      Kernel::work(current_[lane], inputs, pending_[lane], outputs);
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
    if (completedCount_ != size()) {
      discard();
      return;
    }
    for (std::size_t lane = 0; lane < size(); ++lane)
      Kernel::xfer(current_[lane], pending_[lane], outputs(lane));
    discard();
  }
  void discard() noexcept {
    for (std::size_t lane = 0; lane < size(); ++lane) {
      Kernel::discard(current_[lane], pending_[lane]);
      completed_[lane] = false;
    }
    completedCount_ = 0;
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
  std::vector<typename Kernel::Current> current_;
  std::vector<typename Kernel::Pending> pending_;
  std::vector<bool> completed_;
  std::size_t completedCount_ = 0;
};

} // namespace gfsim
