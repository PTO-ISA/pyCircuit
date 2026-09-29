#ifndef GFSIM_SIMDFF_H
#define GFSIM_SIMDFF_H

#include <type_traits>
#include <utility>

namespace gfsim {

template <typename T> class SimDFFE {
  static_assert(std::is_nothrow_copy_assignable_v<T>,
                "SimDFFE Xfer requires no-throw copy assignment");

public:
  explicit SimDFFE(T initial)
      : initial_(initial), current_(initial), next_(std::move(initial)) {}

  const T &Read() const noexcept { return current_; }

  bool Write(const T &data, bool enable) noexcept {
    if (writePending_ || resetPending_)
      return false;
    next_ = data;
    writePending_ = true;
    writeEnabled_ = enable;
    return true;
  }

  bool HasPending() const noexcept { return writePending_ || resetPending_; }
  bool CanWrite() const noexcept { return !HasPending(); }
  bool HasWritePending() const noexcept { return writePending_; }
  bool HasResetPending() const noexcept { return resetPending_; }

  void Reset() noexcept {
    writePending_ = false;
    writeEnabled_ = false;
    resetPending_ = true;
  }

  void DiscardNext() noexcept {
    writePending_ = false;
    writeEnabled_ = false;
    resetPending_ = false;
  }

  void Xfer() noexcept {
    if (resetPending_)
      current_ = initial_;
    else if (writePending_ && writeEnabled_)
      current_ = next_;
    writePending_ = false;
    writeEnabled_ = false;
    resetPending_ = false;
  }

private:
  T initial_;
  T current_;
  T next_;
  bool writePending_ = false;
  bool writeEnabled_ = false;
  bool resetPending_ = false;
};

} // namespace gfsim

#endif // GFSIM_SIMDFF_H
