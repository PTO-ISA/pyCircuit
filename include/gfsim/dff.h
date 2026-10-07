// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/SimModule.h"
#include "gfsim/wire.h"

#include <string>
#include <utility>

namespace gfsim {

// This is the sole register state transition implementation. Both scalar
// modules and bulk owners pass plain state and borrowed pins to the same
// kernel.
template <class T = Bits<1>> struct dffe_kernel {
  struct Current {
    wire<T> q;
    bool clock = false;
  };
  struct Pending {
    wire<T> q;
    bool clock = false;
    bool valid = false;
  };
  struct Inputs {
    const wire<Bits<1>> &clk, &rst, &en;
    const wire<T> &d, &init;
  };
  struct Outputs {
    wire<T> &q;
  };

  static void discard(const Current &current, Pending &pending) noexcept {
    pending.q = current.q;
    pending.clock = current.clock;
    pending.valid = false;
  }
  static void work(const Current &current, Inputs inputs, Pending &pending,
                   Outputs) {
    discard(current, pending);
    try {
      if (!inputs.clk.isFullyKnown())
        throw FourStateViolation("dffe clock must be known");
      pending.clock = inputs.clk.value().toBool();
      pending.valid = true;
      if (current.clock || !pending.clock)
        return;
      if (!inputs.rst.isFullyKnown())
        throw FourStateViolation("dffe reset must be known at posedge");
      if (inputs.rst.value().toBool()) {
        pending.q = inputs.init;
        return;
      }
      if (!inputs.en.isFullyKnown())
        throw FourStateViolation("dffe enable must be known at posedge");
      if (inputs.en.value().toBool())
        pending.q = inputs.d;
    } catch (...) {
      discard(current, pending);
      throw;
    }
  }
  static void xfer(Current &current, Pending &pending,
                   Outputs outputs) noexcept {
    if (!pending.valid)
      return;
    current.q = pending.q;
    current.clock = pending.clock;
    outputs.q = current.q;
    discard(current, pending);
  }
  static void reset(const Current &, Inputs inputs, Pending &pending) noexcept {
    pending.q = inputs.init;
    pending.clock = false;
    pending.valid = true;
  }
};

template <class T = Bits<1>> struct dff_kernel {
  using Current = typename dffe_kernel<T>::Current;
  using Pending = typename dffe_kernel<T>::Pending;
  struct Inputs {
    const wire<Bits<1>> &clk, &rst;
    const wire<T> &d, &init;
  };
  using Outputs = typename dffe_kernel<T>::Outputs;
  static void discard(const Current &current, Pending &pending) noexcept {
    dffe_kernel<T>::discard(current, pending);
  }
  static void work(const Current &current, Inputs inputs, Pending &pending,
                   Outputs outputs) {
    const auto enabled = wire<Bits<1>>::known(Bits<1>{1});
    dffe_kernel<T>::work(
        current, {inputs.clk, inputs.rst, enabled, inputs.d, inputs.init},
        pending, outputs);
  }
  static void xfer(Current &current, Pending &pending,
                   Outputs outputs) noexcept {
    dffe_kernel<T>::xfer(current, pending, outputs);
  }
  static void reset(const Current &current, Inputs inputs,
                    Pending &pending) noexcept {
    const auto enabled = wire<Bits<1>>::known(Bits<1>{1});
    dffe_kernel<T>::reset(
        current, {inputs.clk, inputs.rst, enabled, inputs.d, inputs.init},
        pending);
  }
};

template <class T = Bits<1>> class dffe final : public SimModule {
public:
  using Kernel = dffe_kernel<T>;
  explicit dffe(std::string name) : SimModule(std::move(name)) {}
  wire<Bits<1>> clk, rst, en;
  wire<T> d, init, q;
  void Work() override { Kernel::work(current_, pins(), pending_, {q}); }
  void Xfer() noexcept override { Kernel::xfer(current_, pending_, {q}); }
  void DiscardNext() noexcept override { Kernel::discard(current_, pending_); }
  void Reset() noexcept override { Kernel::reset(current_, pins(), pending_); }
  bool HasWork() const noexcept override { return true; }

private:
  typename Kernel::Inputs pins() const noexcept {
    return {clk, rst, en, d, init};
  }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

template <class T = Bits<1>> class dff final : public SimModule {
public:
  using Kernel = dff_kernel<T>;
  explicit dff(std::string name) : SimModule(std::move(name)) {}
  wire<Bits<1>> clk, rst;
  wire<T> d, init, q;
  void Work() override { Kernel::work(current_, pins(), pending_, {q}); }
  void Xfer() noexcept override { Kernel::xfer(current_, pending_, {q}); }
  void DiscardNext() noexcept override { Kernel::discard(current_, pending_); }
  void Reset() noexcept override { Kernel::reset(current_, pins(), pending_); }
  bool HasWork() const noexcept override { return true; }

private:
  typename Kernel::Inputs pins() const noexcept { return {clk, rst, d, init}; }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

} // namespace gfsim
