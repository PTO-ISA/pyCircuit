// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/SimModule.h"
#include "gfsim/memory_detail.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace gfsim {

// Byte-addressed storage with combinational old-data reads and edge-triggered
// writes. Scalar leaves and bulk owners share this one transition algorithm.
template <class T = Bits<64>, unsigned ADDR_WIDTH = 64,
          std::size_t DEPTH = 1024>
struct byte_mem_kernel {
  static constexpr unsigned DATA_BITS = hardware_traits<T>::width;
  static_assert(ADDR_WIDTH > 0 && DATA_BITS > 0 && DEPTH > 0,
                "byte_mem dimensions must be positive");
  static_assert(DATA_BITS % 8 == 0,
                "byte_mem payload must have whole-byte width");
  static constexpr unsigned STRB_WIDTH = DATA_BITS / 8;
  struct Current {
    std::array<std::uint8_t, DEPTH> memory{};
    bool clock = false;
  };
  struct Pending {
    std::array<std::uint8_t, STRB_WIDTH> bytes{};
    std::array<bool, STRB_WIDTH> lanes{};
    std::size_t address = 0;
    bool clock = false, valid = false;
  };
  struct Inputs {
    const wire<Bits<1>> &clk, &rst;
    const wire<Bits<ADDR_WIDTH>> &raddr;
    const wire<Bits<1>> &wvalid;
    const wire<Bits<ADDR_WIDTH>> &waddr;
    const wire<T> &wdata;
    const wire<Bits<STRB_WIDTH>> &wstrb;
  };
  struct Outputs {
    wire<T> &rdata;
  };
  static void discard(const Current &current, Pending &pending) noexcept {
    pending.lanes.fill(false);
    pending.valid = false;
    pending.clock = current.clock;
  }
  static void reset(const Current &current, Inputs, Pending &pending) noexcept {
    discard(current, pending);
    pending.valid = true;
    pending.clock = false;
  }
  // Pure combinational read, also usable when binding reset initializers.
  // Work separately validates an enabled read address before staging state.
  static void read(const Current &current,
                   const wire<Bits<ADDR_WIDTH>> &address,
                   Outputs outputs) noexcept {
    if (!address.isFullyKnown()) {
      outputs.rdata = wire<T>::unknown();
      return;
    }
    auto readBase = detail::memory_index(address.value());
    Bits<DATA_BITS> readValue{};
    for (unsigned lane = 0; lane < STRB_WIDTH; ++lane)
      readValue =
          readValue |
          shl(zext<DATA_BITS>(Bits<8>{peekByteAt(current, readBase, lane)}),
              8u * lane);
    outputs.rdata = wire<T>::fromPacked(FourState<DATA_BITS>::known(readValue));
  }
  static void work(const Current &current, Inputs inputs, Pending &pending,
                   Outputs outputs) {
    discard(current, pending);
    try {
      if (!inputs.raddr.isFullyKnown())
        throw FourStateViolation("byte_mem read address must be known");
      read(current, inputs.raddr, outputs);
      if (!inputs.clk.isFullyKnown())
        throw FourStateViolation("byte_mem clock must be known");
      pending.clock = inputs.clk.value().toBool();
      pending.valid = true;
      if (current.clock || !pending.clock)
        return;
      if (!inputs.rst.isFullyKnown())
        throw FourStateViolation("byte_mem reset must be known at posedge");
      if (inputs.rst.value().toBool())
        return;
      if (!inputs.wvalid.isFullyKnown())
        throw FourStateViolation(
            "byte_mem write control must be known at posedge");
      if (!inputs.wvalid.value().toBool())
        return;
      if (!inputs.waddr.isFullyKnown() || !inputs.wdata.isFullyKnown() ||
          !inputs.wstrb.isFullyKnown())
        throw FourStateViolation(
            "byte_mem enabled write address/data/strobe must be known");
      pending.address = detail::memory_index(inputs.waddr.value());
      if (pending.address >= DEPTH)
        return;
      for (unsigned lane = 0; lane < STRB_WIDTH; ++lane)
        if (lane < DEPTH - pending.address && inputs.wstrb.value().bit(lane)) {
          pending.bytes[lane] = static_cast<std::uint8_t>(
              extract<8>(inputs.wdata.packed().value(), 8u * lane).value());
          pending.lanes[lane] = true;
        }
    } catch (...) {
      discard(current, pending);
      throw;
    }
  }
  static void xfer(Current &current, Pending &pending, Outputs) noexcept {
    if (!pending.valid)
      return;
    current.clock = pending.clock;
    for (unsigned lane = 0; lane < STRB_WIDTH; ++lane)
      if (pending.lanes[lane])
        current.memory[pending.address + lane] = pending.bytes[lane];
    discard(current, pending);
  }
  static std::uint8_t peekByteAt(const Current &current, std::size_t base,
                                 unsigned lane) noexcept {
    if (base >= DEPTH || lane >= DEPTH - base)
      return 0;
    return current.memory[base + lane];
  }
};

template <class T = Bits<64>, unsigned ADDR_WIDTH = 64,
          std::size_t DEPTH = 1024>
class byte_mem final : public SimModule {
public:
  using Kernel = byte_mem_kernel<T, ADDR_WIDTH, DEPTH>;
  static constexpr unsigned DATA_BITS = Kernel::DATA_BITS;
  static constexpr unsigned STRB_WIDTH = Kernel::STRB_WIDTH;
  explicit byte_mem(std::string name) : SimModule(std::move(name)) {}
  wire<Bits<1>> clk, rst;
  wire<Bits<ADDR_WIDTH>> raddr;
  wire<T> rdata;
  wire<Bits<1>> wvalid;
  wire<Bits<ADDR_WIDTH>> waddr;
  wire<T> wdata;
  wire<Bits<STRB_WIDTH>> wstrb;
  void Work() override { Kernel::work(current_, pins(), pending_, {rdata}); }
  void Xfer() noexcept override { Kernel::xfer(current_, pending_, {rdata}); }
  void DiscardNext() noexcept override { Kernel::discard(current_, pending_); }
  void Reset() noexcept override { Kernel::reset(current_, pins(), pending_); }
  void pokeByte(std::size_t address, std::uint8_t value) {
    if (address < DEPTH)
      current_.memory[address] = value;
  }
  std::uint8_t peekByte(std::size_t address) const {
    return Kernel::peekByteAt(current_, address, 0);
  }
  std::uint32_t peek32(std::size_t address) const {
    std::uint32_t value = 0;
    for (unsigned lane = 0; lane < 4; ++lane)
      value |= static_cast<std::uint32_t>(
                   Kernel::peekByteAt(current_, address, lane))
               << (8u * lane);
    return value;
  }
  bool HasWork() const noexcept override { return true; }

private:
  typename Kernel::Inputs pins() const noexcept {
    return {clk, rst, raddr, wvalid, waddr, wdata, wstrb};
  }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

} // namespace gfsim
