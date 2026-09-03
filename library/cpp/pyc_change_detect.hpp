#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include "pyc_bits.hpp"

namespace pyc::cpp {

// ---------------------------------------------------------------------------
// ChangeDetector<Width> — lightweight snapshot-based change detection for
// individual Wire<Width> signals.  Compares current value against a cached
// snapshot taken at the previous observation point.
// ---------------------------------------------------------------------------

template <unsigned Width>
class ChangeDetector {
public:
  explicit ChangeDetector(const Wire<Width> &target) : target_(target) {
    snapshot_ = target;
  }

  bool changed() const { return !(target_ == snapshot_); }

  void capture() { snapshot_ = target_; }

  bool check_and_capture() {
    bool c = changed();
    snapshot_ = target_;
    return c;
  }

private:
  const Wire<Width> &target_;
  Wire<Width> snapshot_{};
};

// ---------------------------------------------------------------------------
// InputFingerprint — tracks whether *any* of a set of primary inputs changed
// since the last capture.  Uses a simple XOR-fold hash over raw words for
// O(1) fast-path rejection, with a full comparison fallback.
//
// Usage (in a CAPI wrapper or testbench):
//   InputFingerprint<80, 5, 40, 320> fp(dut.raddr_bus, dut.wen_bus, ...);
//   ...
//   if (fp.check_and_capture()) { dut.eval(); }
// ---------------------------------------------------------------------------

namespace detail {

template <unsigned Width>
inline void xor_fold(const Wire<Width> &w, std::uint64_t &acc) {
  for (unsigned i = 0; i < Wire<Width>::kWords; i++)
    acc ^= w.word(i) * (0x9E3779B97F4A7C15ULL + i);
}

template <unsigned Width>
inline std::size_t wire_bytes() {
  return Wire<Width>::kWords * sizeof(std::uint64_t);
}

} // namespace detail

template <unsigned... Widths>
class InputFingerprint {
public:
  static constexpr std::size_t kTotalWords = ((Wire<Widths>::kWords + ... + 0));

  explicit InputFingerprint(const Wire<Widths> &...wires)
      : ptrs_{wires.data()...}, sizes_{Wire<Widths>::kWords...} {
    do_capture();
  }

  bool changed() const {
    std::uint64_t h = 0;
    std::size_t idx = 0;
    auto fold = [&](const std::uint64_t *p, unsigned nw) {
      for (unsigned i = 0; i < nw; i++)
        h ^= p[i] * (0x9E3779B97F4A7C15ULL + idx++);
    };
    for (unsigned k = 0; k < sizeof...(Widths); k++)
      fold(ptrs_[k], sizes_[k]);

    if (h != hash_)
      return true;

    idx = 0;
    for (unsigned k = 0; k < sizeof...(Widths); k++) {
      if (std::memcmp(ptrs_[k], &snapshot_[idx],
                      sizes_[k] * sizeof(std::uint64_t)) != 0)
        return true;
      idx += sizes_[k];
    }
    return false;
  }

  void capture() { do_capture(); }

  bool check_and_capture() {
    bool c = changed();
    do_capture();
    return c;
  }

private:
  void do_capture() {
    hash_ = 0;
    std::size_t idx = 0;
    std::size_t fold_idx = 0;
    for (unsigned k = 0; k < sizeof...(Widths); k++) {
      for (unsigned i = 0; i < sizes_[k]; i++) {
        snapshot_[idx] = ptrs_[k][i];
        hash_ ^= ptrs_[k][i] * (0x9E3779B97F4A7C15ULL + fold_idx++);
        idx++;
      }
    }
  }

  const std::uint64_t *ptrs_[sizeof...(Widths)];
  unsigned sizes_[sizeof...(Widths)];
  std::uint64_t hash_ = 0;
  std::uint64_t snapshot_[kTotalWords]{};
};

// ---------------------------------------------------------------------------
// EvalGuard — wraps an eval_comb function call, only executing if at least
// one input Wire changed since the last invocation.
//
// Template parameters:
//   Fn          — callable (lambda / function pointer) for the eval_comb body
//   InputWidths — widths of the input Wires tracked by this guard
//
// Usage:
//   EvalGuard guard([&]{ dut.eval_comb_0(); }, dut.raddr_bus, dut.wen_bus);
//   guard.eval();   // only calls eval_comb_0 if raddr_bus or wen_bus changed
// ---------------------------------------------------------------------------

template <typename Fn, unsigned... InputWidths>
class EvalGuard {
public:
  explicit EvalGuard(Fn fn, const Wire<InputWidths> &...inputs)
      : fn_(fn), fp_(inputs...) {}

  bool eval() {
    if (fp_.check_and_capture()) {
      fn_();
      return true;
    }
    return false;
  }

  void force_eval() {
    fp_.capture();
    fn_();
  }

private:
  Fn fn_;
  InputFingerprint<InputWidths...> fp_;
};

template <typename Fn, unsigned... Ws>
EvalGuard(Fn, const Wire<Ws> &...) -> EvalGuard<Fn, Ws...>;

} // namespace pyc::cpp
