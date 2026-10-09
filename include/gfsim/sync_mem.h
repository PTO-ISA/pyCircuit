// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/SimModule.h"
#include "gfsim/memory_detail.h"
#include <array>
#include <cstddef>
#include <string>
#include <utility>

namespace gfsim {

// Sole 1R1W/2R1W state kernel, shared by scalar leaves and bulk owners. Reads
// sample old storage; Q expires after two idle rising edges. Reset invalidates
// Q and clock history without clearing RAM. Pending contains no RAM copy.
template <class T = Bits<64>, unsigned ADDR_WIDTH = 64,
          std::size_t DEPTH = 1024, std::size_t READ_PORTS = 1>
struct sync_mem_kernel {
  static constexpr unsigned DATA_BITS = hardware_traits<T>::width;
  static constexpr unsigned STRB_WIDTH = DATA_BITS / 8 + (DATA_BITS % 8 != 0);
  static_assert(ADDR_WIDTH > 0 && DATA_BITS > 0 && DEPTH > 0,
                "memory dimensions must be positive");
  static_assert(READ_PORTS == 1 || READ_PORTS == 2,
                "sync_mem supports one or two read ports");
  struct Current {
    std::array<Bits<DATA_BITS>, DEPTH> memory{};
    std::array<wire<T>, READ_PORTS> q{};
    std::array<bool, READ_PORTS> live{};
    std::array<unsigned, READ_PORTS> idle{};
    bool clock = false;
  };
  struct Pending {
    std::array<wire<T>, READ_PORTS> q{};
    std::array<bool, READ_PORTS> read{}, invalidate{};
    std::array<unsigned, READ_PORTS> idle{};
    Bits<DATA_BITS> data{};
    std::size_t address = 0;
    bool write = false, clock = false, valid = false;
  };
  struct Inputs {
    const wire<Bits<1>> &clk, &rst;
    std::array<const wire<Bits<1>> *, READ_PORTS> ren;
    std::array<const wire<Bits<ADDR_WIDTH>> *, READ_PORTS> raddr;
    const wire<Bits<1>> &wvalid;
    const wire<Bits<ADDR_WIDTH>> &waddr;
    const wire<T> &wdata;
    const wire<Bits<STRB_WIDTH>> &wstrb;
  };
  struct Outputs {
    std::array<wire<T> *, READ_PORTS> rdata;
  };

  static void discard(const Current &current, Pending &pending) noexcept {
    pending.valid = false;
    pending.clock = current.clock;
    pending.write = false;
    pending.read.fill(false);
    pending.invalidate.fill(false);
    pending.idle = current.idle;
  }
  static void reset(const Current &current, Inputs, Pending &pending) noexcept {
    discard(current, pending);
    pending.valid = true;
    pending.clock = false;
    invalidate(pending);
  }
  static void work(const Current &current, Inputs inputs, Pending &pending,
                   Outputs) {
    discard(current, pending);
    try {
      if (!inputs.clk.isFullyKnown())
        throw FourStateViolation("memory clock must be known");
      pending.clock = inputs.clk.value().toBool();
      pending.valid = true;
      if (current.clock || !pending.clock)
        return;
      if (!inputs.rst.isFullyKnown())
        throw FourStateViolation(
            READ_PORTS == 1 ? "sync_mem reset must be known at posedge"
                            : "sync_mem_dp reset must be known at posedge");
      if (inputs.rst.value().toBool()) {
        invalidate(pending);
        return;
      }
      if (!inputs.wvalid.isFullyKnown())
        throw FourStateViolation(
            READ_PORTS == 1 ? "sync_mem controls must be known at posedge"
                            : "sync_mem_dp controls must be known at posedge");
      for (auto ren : inputs.ren)
        if (!ren->isFullyKnown())
          throw FourStateViolation(
              READ_PORTS == 1
                  ? "sync_mem controls must be known at posedge"
                  : "sync_mem_dp controls must be known at posedge");
      if (inputs.wvalid.value().toBool()) {
        if (!inputs.waddr.isFullyKnown() || !inputs.wdata.isFullyKnown() ||
            !inputs.wstrb.isFullyKnown())
          throw FourStateViolation(
              READ_PORTS == 1
                  ? "sync_mem enabled write address/data/strobe must be known"
                  : "sync_mem_dp enabled write address/data/strobe must be "
                    "known");
        pending.address = detail::memory_index(inputs.waddr.value());
        if (pending.address < DEPTH) {
          pending.data = detail::memory_strobe(current.memory[pending.address],
                                               inputs.wdata.packed().value(),
                                               inputs.wstrb.value());
          pending.write = true;
        }
      }
      for (std::size_t port = 0; port < READ_PORTS; ++port) {
        if (inputs.ren[port]->value().toBool()) {
          if (!inputs.raddr[port]->isFullyKnown())
            throw FourStateViolation(
                READ_PORTS == 1
                    ? "sync_mem enabled read address must be known"
                    : "sync_mem_dp enabled read address must be known");
          auto address = detail::memory_index(inputs.raddr[port]->value());
          pending.q[port] = wire<T>::fromPacked(FourState<DATA_BITS>::known(
              address < DEPTH ? current.memory[address] : Bits<DATA_BITS>{0}));
          pending.read[port] = true;
          pending.idle[port] = 0;
        } else if (current.live[port]) {
          ++pending.idle[port];
          if (pending.idle[port] >= 2) {
            pending.invalidate[port] = true;
            pending.q[port] = wire<T>::unknown();
          }
        }
      }
    } catch (...) {
      discard(current, pending);
      throw;
    }
  }
  static void xfer(Current &current, Pending &pending,
                   Outputs outputs) noexcept {
    if (!pending.valid)
      return;
    current.clock = pending.clock;
    if (pending.write)
      current.memory[pending.address] = pending.data;
    for (std::size_t port = 0; port < READ_PORTS; ++port) {
      if (pending.read[port] || pending.invalidate[port]) {
        current.q[port] = pending.q[port];
        *outputs.rdata[port] = current.q[port];
        current.live[port] = pending.read[port];
      }
    }
    current.idle = pending.idle;
    discard(current, pending);
  }

private:
  static void invalidate(Pending &pending) noexcept {
    pending.invalidate.fill(true);
    pending.idle.fill(0);
    for (auto &q : pending.q)
      q = wire<T>::unknown();
  }
};

template <class T = Bits<64>, unsigned ADDR_WIDTH = 64,
          std::size_t DEPTH = 1024>
class sync_mem final : public SimModule {
public:
  using Kernel = sync_mem_kernel<T, ADDR_WIDTH, DEPTH, 1>;
  static constexpr unsigned DATA_BITS = Kernel::DATA_BITS;
  static constexpr unsigned STRB_WIDTH = Kernel::STRB_WIDTH;
  using ReadState = wire<T>;
  explicit sync_mem(std::string name) : SimModule(std::move(name)) {}
  wire<Bits<1>> clk, rst, ren;
  wire<Bits<ADDR_WIDTH>> raddr;
  wire<T> rdata;
  wire<Bits<1>> wvalid;
  wire<Bits<ADDR_WIDTH>> waddr;
  wire<T> wdata;
  wire<Bits<STRB_WIDTH>> wstrb;
  void Work() override { Kernel::work(current_, pins(), pending_, {{&rdata}}); }
  void Xfer() noexcept override {
    Kernel::xfer(current_, pending_, {{&rdata}});
  }
  void DiscardNext() noexcept override { Kernel::discard(current_, pending_); }
  void Reset() noexcept override { Kernel::reset(current_, pins(), pending_); }
  void pokeEntry(std::size_t address, const T &value) {
    if (address < DEPTH)
      current_.memory[address] = hardware_traits<T>::pack(value);
  }
  T peekEntry(std::size_t address) const {
    return hardware_traits<T>::unpack(address < DEPTH ? current_.memory[address]
                                                      : Bits<DATA_BITS>{0});
  }
  bool HasWork() const noexcept override { return true; }

private:
  typename Kernel::Inputs pins() const noexcept {
    return {clk, rst, {&ren}, {&raddr}, wvalid, waddr, wdata, wstrb};
  }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

template <class T = Bits<64>, unsigned ADDR_WIDTH = 64,
          std::size_t DEPTH = 1024>
class sync_mem_dp final : public SimModule {
public:
  using Kernel = sync_mem_kernel<T, ADDR_WIDTH, DEPTH, 2>;
  static constexpr unsigned DATA_BITS = Kernel::DATA_BITS;
  static constexpr unsigned STRB_WIDTH = Kernel::STRB_WIDTH;
  using ReadState = wire<T>;
  explicit sync_mem_dp(std::string name) : SimModule(std::move(name)) {}
  wire<Bits<1>> clk, rst, ren0, ren1;
  wire<Bits<ADDR_WIDTH>> raddr0, raddr1;
  wire<T> rdata0, rdata1;
  wire<Bits<1>> wvalid;
  wire<Bits<ADDR_WIDTH>> waddr;
  wire<T> wdata;
  wire<Bits<STRB_WIDTH>> wstrb;
  void Work() override {
    Kernel::work(current_, pins(), pending_, {{&rdata0, &rdata1}});
  }
  void Xfer() noexcept override {
    Kernel::xfer(current_, pending_, {{&rdata0, &rdata1}});
  }
  void DiscardNext() noexcept override { Kernel::discard(current_, pending_); }
  void Reset() noexcept override { Kernel::reset(current_, pins(), pending_); }
  void pokeEntry(std::size_t address, const T &value) {
    if (address < DEPTH)
      current_.memory[address] = hardware_traits<T>::pack(value);
  }
  T peekEntry(std::size_t address) const {
    return hardware_traits<T>::unpack(address < DEPTH ? current_.memory[address]
                                                      : Bits<DATA_BITS>{0});
  }
  bool HasWork() const noexcept override { return true; }

private:
  typename Kernel::Inputs pins() const noexcept {
    return {clk,    rst,   {&ren0, &ren1}, {&raddr0, &raddr1},
            wvalid, waddr, wdata,          wstrb};
  }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

} // namespace gfsim
