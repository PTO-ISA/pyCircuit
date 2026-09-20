#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "pyc_bits.hpp"
#include "pyc_four_state.hpp"

namespace pyc::cpp {

// Synchronous 1R1W memory with registered read output.
//
// - `DepthEntries` is in entries (not bytes).
// - Read output updates on the next posedge of `clk` when `ren` is asserted.
// - Write occurs on posedge when `wvalid` is asserted, with byte enables `wstrb`.
// - Read-during-write to the same address returns the pre-write data (old-data).
// - Addresses are low-bit indexed into host `size_t`; out-of-range indices read as 0
//   and writes are dropped.
template <unsigned AddrWidth, unsigned DataWidth, std::size_t DepthEntries,
          unsigned LiveWindow = 1>
class pyc_sync_mem {
public:
  static_assert(DataWidth > 0, "pyc_sync_mem requires DataWidth > 0");
  static_assert(DepthEntries > 0, "pyc_sync_mem DepthEntries must be > 0");
  static_assert(LiveWindow == 1,
                "pyc_sync_mem aggressive verification requires N=1");
  static constexpr unsigned StrbWidth = (DataWidth + 7) / 8;
  using ReadState = FourState<DataWidth>;

  pyc_sync_mem(Wire<1> &clk,
               Wire<1> &rst,
               Wire<1> &ren,
               Wire<AddrWidth> &raddr,
               Wire<DataWidth> &rdata,
               Wire<1> &wvalid,
               Wire<AddrWidth> &waddr,
               Wire<DataWidth> &wdata,
               Wire<StrbWidth> &wstrb)
      : clk(clk), rst(rst), ren(ren), raddr(raddr), rdata(rdata), wvalid(wvalid), waddr(waddr), wdata(wdata),
        wstrb(wstrb) {}

  const ReadState &rdataState() const { return rdataState_; }
  bool rdataLive() const { return rdataLive_; }

  void setVerificationInputs(
      FourState<1> rstState, FourState<1> renState,
      FourState<AddrWidth> raddrState, FourState<1> wvalidState,
      FourState<AddrWidth> waddrState, FourState<DataWidth> wdataState,
      FourState<StrbWidth> wstrbState) {
    verifyRst_ = std::move(rstState);
    verifyRen_ = std::move(renState);
    verifyRaddr_ = std::move(raddrState);
    verifyWvalid_ = std::move(wvalidState);
    verifyWaddr_ = std::move(waddrState);
    verifyWdata_ = std::move(wdataState);
    verifyWstrb_ = std::move(wstrbState);
  }

  void clearVerificationInputs() {
    verifyRst_.reset();
    verifyRen_.reset();
    verifyRaddr_.reset();
    verifyWvalid_.reset();
    verifyWaddr_.reset();
    verifyWdata_.reset();
    verifyWstrb_.reset();
  }

  struct MemWatchEvent {
    enum class Kind : std::uint8_t { Read = 0, Write = 1 };
    Kind kind = Kind::Read;
    std::uint8_t port = 0; // reserved for multi-port memories
    std::size_t addr = 0;
    Wire<DataWidth> data{};
    Wire<StrbWidth> strb{};
  };

  // Decision 0006: memory observability supports hash/watch/dump.
  //
  // Notes:
  // - For sync memories, `addr` is in entries (not bytes).
  // - Watch events are recorded on the active clock edge:
  //   - read events when `ren` is asserted
  //   - write events when `wvalid` is asserted (data is the committed value)
  void mem_watch(std::size_t lo, std::size_t hi) {
    if (lo > hi)
      std::swap(lo, hi);
    watch_enabled_ = true;
    watch_lo_ = lo;
    watch_hi_ = hi;
    watch_events_.clear();
  }
  void mem_watch_disable() {
    watch_enabled_ = false;
    watch_events_.clear();
  }
  bool mem_watch_enabled() const { return watch_enabled_; }
  void mem_watch_clear() { watch_events_.clear(); }
  const std::vector<MemWatchEvent> &mem_watch_events() const { return watch_events_; }

  std::uint64_t mem_hash(std::size_t lo = 0, std::size_t hi = (DepthEntries > 0 ? DepthEntries - 1 : 0)) const {
    if (DepthEntries == 0)
      return 0;
    if (lo > hi)
      std::swap(lo, hi);
    if (hi >= DepthEntries)
      hi = DepthEntries - 1;

    std::uint64_t h = 1469598103934665603ull; // FNV-1a offset basis
    for (std::size_t i = lo; i <= hi; ++i) {
      for (unsigned w = 0; w < Wire<DataWidth>::kWords; ++w) {
        h ^= mem_[i].word(w);
        h *= 1099511628211ull; // FNV-1a prime
      }
    }
    return h;
  }

  void mem_dump(std::ostream &os, std::size_t lo = 0, std::size_t hi = (DepthEntries > 0 ? DepthEntries - 1 : 0)) const {
    if (DepthEntries == 0)
      return;
    if (lo > hi)
      std::swap(lo, hi);
    if (hi >= DepthEntries)
      hi = DepthEntries - 1;

    auto dumpHex = [&](Wire<DataWidth> v) {
      os << "0x";
      const unsigned hexDigits = (DataWidth + 3u) / 4u;
      const unsigned words = Wire<DataWidth>::kWords;
      for (int wi = static_cast<int>(words) - 1; wi >= 0; --wi) {
        const std::uint64_t word = v.word(static_cast<unsigned>(wi));
        const unsigned wordDigits = (wi == static_cast<int>(words) - 1) ? ((hexDigits - 1) % 16u + 1u) : 16u;
        os << std::hex << std::setw(static_cast<int>(wordDigits)) << std::setfill('0') << word << std::dec;
      }
    };

    for (std::size_t i = lo; i <= hi; ++i) {
      os << "{\"addr\":" << i << ",\"data\":\"";
      dumpHex(mem_[i]);
      os << "\"}\n";
    }
  }

  void tick_compute() {
    bool clkNow = clk.toBool();
    bool posedge = (!clkPrev) && clkNow;
    clkPrev = clkNow;
    pendingWrite = false;
    pendingRead = false;
    pendingInvalidate = false;
    if (!posedge)
      return;

    const FourState<1> rstState =
        verifyRst_.value_or(FourState<1>::known(rst));
    if (!rstState.isFullyKnown())
      throw FourStateViolation("pyc_sync_mem reset must be known at posedge");
    if (rstState.value().toBool()) {
      pendingInvalidate = true;
      return;
    }

    const FourState<1> renState =
        verifyRen_.value_or(FourState<1>::known(ren));
    const FourState<1> wvalidState =
        verifyWvalid_.value_or(FourState<1>::known(wvalid));
    if (!renState.isFullyKnown() || !wvalidState.isFullyKnown())
      throw FourStateViolation(
          "pyc_sync_mem enabled controls must be known at posedge");

    if (wvalidState.value().toBool()) {
      const auto waddrState = verifyWaddr_.value_or(
          FourState<AddrWidth>::known(waddr));
      const auto wdataState = verifyWdata_.value_or(
          FourState<DataWidth>::known(wdata));
      const auto wstrbState = verifyWstrb_.value_or(
          FourState<StrbWidth>::known(wstrb));
      if (!waddrState.isFullyKnown() || !wdataState.isFullyKnown() ||
          !wstrbState.isFullyKnown())
        throw FourStateViolation(
            "pyc_sync_mem enabled write address/data/strobe must be known");
      pendingWrite = true;
      latchedWaddr = toIndex(waddr);
      latchedWdata = wdata;
      latchedWstrb = wstrb;
    }

    if (renState.value().toBool()) {
      const auto raddrState = verifyRaddr_.value_or(
          FourState<AddrWidth>::known(raddr));
      if (!raddrState.isFullyKnown())
        throw FourStateViolation(
            "pyc_sync_mem enabled read address must be known");
      pendingRead = true;
      latchedRaddr = toIndex(raddr);
      Wire<DataWidth> v = Wire<DataWidth>(0);
      if (latchedRaddr < DepthEntries)
        v = mem_[latchedRaddr];
      rdataNext = v;
      rdataStateNext = ReadState::known(v);
      if (watch_enabled_ && latchedRaddr >= watch_lo_ && latchedRaddr <= watch_hi_) {
        MemWatchEvent ev;
        ev.kind = MemWatchEvent::Kind::Read;
        ev.port = 0;
        ev.addr = latchedRaddr;
        ev.data = v;
        ev.strb = Wire<StrbWidth>(0);
        watch_events_.push_back(ev);
      }
    } else if (rdataLive_) {
      pendingInvalidate = true;
    }
  }

  void tick_commit() {
    if (pendingWrite && (latchedWaddr < DepthEntries)) {
      Wire<DataWidth> committed = applyStrb(mem_[latchedWaddr], latchedWdata, latchedWstrb);
      mem_[latchedWaddr] = committed;
      if (watch_enabled_ && latchedWaddr >= watch_lo_ && latchedWaddr <= watch_hi_) {
        MemWatchEvent ev;
        ev.kind = MemWatchEvent::Kind::Write;
        ev.port = 0;
        ev.addr = latchedWaddr;
        ev.data = committed;
        ev.strb = latchedWstrb;
        watch_events_.push_back(ev);
      }
    }
    if (pendingRead) {
      rdata = rdataNext;
      rdataState_ = rdataStateNext;
      rdataLive_ = true;
    } else if (pendingInvalidate) {
      rdataState_ = ReadState::unknown(rdata);
      rdataLive_ = false;
    }
    pendingWrite = false;
    pendingRead = false;
    pendingInvalidate = false;
  }

  // Convenience for testbenches.
  void pokeEntry(std::size_t addr, Wire<DataWidth> value) {
    if (addr < DepthEntries)
      mem_[addr] = value;
  }
  void pokeEntry(std::size_t addr, std::uint64_t value) { pokeEntry(addr, Wire<DataWidth>(value)); }
  Wire<DataWidth> peekEntryBits(std::size_t addr) const {
    return (addr < DepthEntries) ? mem_[addr] : Wire<DataWidth>(0);
  }
  std::uint64_t peekEntry(std::size_t addr) const { return peekEntryBits(addr).value(); }

public:
  Wire<1> &clk;
  Wire<1> &rst;

  Wire<1> &ren;
  Wire<AddrWidth> &raddr;
  Wire<DataWidth> &rdata;

  Wire<1> &wvalid;
  Wire<AddrWidth> &waddr;
  Wire<DataWidth> &wdata;
  Wire<StrbWidth> &wstrb;

  bool clkPrev = false;
  bool pendingWrite = false;
  bool pendingRead = false;
  bool pendingInvalidate = false;
  bool rdataLive_ = false;
  std::size_t latchedWaddr = 0;
  std::size_t latchedRaddr = 0;
  Wire<DataWidth> latchedWdata{};
  Wire<StrbWidth> latchedWstrb{};
  Wire<DataWidth> rdataNext{};
  ReadState rdataState_ = ReadState::unknown();
  ReadState rdataStateNext = ReadState::unknown();

  std::optional<FourState<1>> verifyRst_;
  std::optional<FourState<1>> verifyRen_;
  std::optional<FourState<AddrWidth>> verifyRaddr_;
  std::optional<FourState<1>> verifyWvalid_;
  std::optional<FourState<AddrWidth>> verifyWaddr_;
  std::optional<FourState<DataWidth>> verifyWdata_;
  std::optional<FourState<StrbWidth>> verifyWstrb_;

private:
  static constexpr std::size_t toIndex(Wire<AddrWidth> addr) {
    if constexpr (AddrWidth <= (sizeof(std::size_t) * 8u))
      return static_cast<std::size_t>(addr.value());
    constexpr unsigned hostBits = static_cast<unsigned>(sizeof(std::size_t) * 8u);
    constexpr unsigned useBits = (AddrWidth < hostBits) ? AddrWidth : hostBits;
    std::size_t out = 0;
    for (unsigned i = 0; i < useBits; i++) {
      if (addr.bit(i))
        out |= (std::size_t{1} << i);
    }
    return out;
  }

  static constexpr unsigned lastLaneBits() {
    unsigned rem = DataWidth % 8;
    return (rem == 0) ? 8 : rem;
  }

  static constexpr Wire<DataWidth> applyStrb(Wire<DataWidth> oldV, Wire<DataWidth> newV, Wire<StrbWidth> strb) {
    if constexpr (DataWidth <= 64) {
      std::uint64_t out = oldV.value();
      std::uint64_t src = newV.value();
      for (unsigned i = 0; i < StrbWidth; i++) {
        if (!strb.bit(i))
          continue;
        unsigned bitsInLane = (i == StrbWidth - 1) ? lastLaneBits() : 8u;
        std::uint64_t mask = ((1ull << bitsInLane) - 1ull) << (8u * i);
        out = (out & ~mask) | (src & mask);
      }
      return Wire<DataWidth>(out);
    }
    Wire<DataWidth> v = oldV;
    for (unsigned i = 0; i < StrbWidth; i++) {
      if (!strb.bit(i))
        continue;
      unsigned bitsInLane = (i == StrbWidth - 1) ? lastLaneBits() : 8u;
      for (unsigned b = 0; b < bitsInLane; ++b) {
        unsigned bitIdx = 8u * i + b;
        if (newV.bit(bitIdx))
          v = v | shl<DataWidth>(Wire<DataWidth>(1), bitIdx);
        else
          v = v & ~shl<DataWidth>(Wire<DataWidth>(1), bitIdx);
      }
    }
    return v;
  }

  std::array<Wire<DataWidth>, DepthEntries> mem_{};

  bool watch_enabled_ = false;
  std::size_t watch_lo_ = 0;
  std::size_t watch_hi_ = 0;
  std::vector<MemWatchEvent> watch_events_{};
};

// Synchronous 2R1W memory (dual read ports) with registered read outputs.
template <unsigned AddrWidth, unsigned DataWidth, std::size_t DepthEntries,
          unsigned LiveWindow = 1>
class pyc_sync_mem_dp {
public:
  static_assert(DataWidth > 0, "pyc_sync_mem_dp requires DataWidth > 0");
  static_assert(DepthEntries > 0, "pyc_sync_mem_dp DepthEntries must be > 0");
  static_assert(LiveWindow == 1,
                "pyc_sync_mem_dp aggressive verification requires N=1");
  static constexpr unsigned StrbWidth = (DataWidth + 7) / 8;
  using ReadState = FourState<DataWidth>;

  pyc_sync_mem_dp(Wire<1> &clk,
                  Wire<1> &rst,
                  Wire<1> &ren0,
                  Wire<AddrWidth> &raddr0,
                  Wire<DataWidth> &rdata0,
                  Wire<1> &ren1,
                  Wire<AddrWidth> &raddr1,
                  Wire<DataWidth> &rdata1,
                  Wire<1> &wvalid,
                  Wire<AddrWidth> &waddr,
                  Wire<DataWidth> &wdata,
                  Wire<StrbWidth> &wstrb)
      : clk(clk), rst(rst), ren0(ren0), raddr0(raddr0), rdata0(rdata0), ren1(ren1), raddr1(raddr1), rdata1(rdata1),
        wvalid(wvalid), waddr(waddr), wdata(wdata), wstrb(wstrb) {}

  const ReadState &rdataState(unsigned port) const {
    if (port == 0)
      return rdata0State_;
    if (port == 1)
      return rdata1State_;
    throw std::out_of_range("pyc_sync_mem_dp read port must be 0 or 1");
  }
  bool rdataLive(unsigned port) const {
    if (port == 0)
      return rdata0Live_;
    if (port == 1)
      return rdata1Live_;
    throw std::out_of_range("pyc_sync_mem_dp read port must be 0 or 1");
  }

  struct MemWatchEvent {
    enum class Kind : std::uint8_t { Read = 0, Write = 1 };
    Kind kind = Kind::Read;
    std::uint8_t port = 0; // 0/1 for read ports; 0 for writes
    std::size_t addr = 0;
    Wire<DataWidth> data{};
    Wire<StrbWidth> strb{};
  };

  void mem_watch(std::size_t lo, std::size_t hi) {
    if (lo > hi)
      std::swap(lo, hi);
    watch_enabled_ = true;
    watch_lo_ = lo;
    watch_hi_ = hi;
    watch_events_.clear();
  }
  void mem_watch_disable() {
    watch_enabled_ = false;
    watch_events_.clear();
  }
  bool mem_watch_enabled() const { return watch_enabled_; }
  void mem_watch_clear() { watch_events_.clear(); }
  const std::vector<MemWatchEvent> &mem_watch_events() const { return watch_events_; }

  std::uint64_t mem_hash(std::size_t lo = 0, std::size_t hi = (DepthEntries > 0 ? DepthEntries - 1 : 0)) const {
    if (DepthEntries == 0)
      return 0;
    if (lo > hi)
      std::swap(lo, hi);
    if (hi >= DepthEntries)
      hi = DepthEntries - 1;

    std::uint64_t h = 1469598103934665603ull;
    for (std::size_t i = lo; i <= hi; ++i) {
      for (unsigned w = 0; w < Wire<DataWidth>::kWords; ++w) {
        h ^= mem_[i].word(w);
        h *= 1099511628211ull;
      }
    }
    return h;
  }

  void mem_dump(std::ostream &os, std::size_t lo = 0, std::size_t hi = (DepthEntries > 0 ? DepthEntries - 1 : 0)) const {
    if (DepthEntries == 0)
      return;
    if (lo > hi)
      std::swap(lo, hi);
    if (hi >= DepthEntries)
      hi = DepthEntries - 1;

    auto dumpHex = [&](Wire<DataWidth> v) {
      os << "0x";
      const unsigned hexDigits = (DataWidth + 3u) / 4u;
      const unsigned words = Wire<DataWidth>::kWords;
      for (int wi = static_cast<int>(words) - 1; wi >= 0; --wi) {
        const std::uint64_t word = v.word(static_cast<unsigned>(wi));
        const unsigned wordDigits = (wi == static_cast<int>(words) - 1) ? ((hexDigits - 1) % 16u + 1u) : 16u;
        os << std::hex << std::setw(static_cast<int>(wordDigits)) << std::setfill('0') << word << std::dec;
      }
    };

    for (std::size_t i = lo; i <= hi; ++i) {
      os << "{\"addr\":" << i << ",\"data\":\"";
      dumpHex(mem_[i]);
      os << "\"}\n";
    }
  }

  void tick_compute() {
    bool clkNow = clk.toBool();
    bool posedge = (!clkPrev) && clkNow;
    clkPrev = clkNow;
    pendingWrite = false;
    pendingRead0 = false;
    pendingRead1 = false;
    pendingInvalidate0 = false;
    pendingInvalidate1 = false;
    if (!posedge)
      return;

    if (rst.toBool()) {
      pendingInvalidate0 = true;
      pendingInvalidate1 = true;
      return;
    }

    if (wvalid.toBool()) {
      pendingWrite = true;
      latchedWaddr = toIndex(waddr);
      latchedWdata = wdata;
      latchedWstrb = wstrb;
    }

    if (ren0.toBool()) {
      pendingRead0 = true;
      latchedRaddr0 = toIndex(raddr0);
      Wire<DataWidth> v = Wire<DataWidth>(0);
      if (latchedRaddr0 < DepthEntries)
        v = mem_[latchedRaddr0];
      rdata0Next = v;
      rdata0StateNext = ReadState::known(v);
      if (watch_enabled_ && latchedRaddr0 >= watch_lo_ && latchedRaddr0 <= watch_hi_) {
        MemWatchEvent ev;
        ev.kind = MemWatchEvent::Kind::Read;
        ev.port = 0;
        ev.addr = latchedRaddr0;
        ev.data = v;
        ev.strb = Wire<StrbWidth>(0);
        watch_events_.push_back(ev);
      }
    } else if (rdata0Live_) {
      pendingInvalidate0 = true;
    }

    if (ren1.toBool()) {
      pendingRead1 = true;
      latchedRaddr1 = toIndex(raddr1);
      Wire<DataWidth> v = Wire<DataWidth>(0);
      if (latchedRaddr1 < DepthEntries)
        v = mem_[latchedRaddr1];
      rdata1Next = v;
      rdata1StateNext = ReadState::known(v);
      if (watch_enabled_ && latchedRaddr1 >= watch_lo_ && latchedRaddr1 <= watch_hi_) {
        MemWatchEvent ev;
        ev.kind = MemWatchEvent::Kind::Read;
        ev.port = 1;
        ev.addr = latchedRaddr1;
        ev.data = v;
        ev.strb = Wire<StrbWidth>(0);
        watch_events_.push_back(ev);
      }
    } else if (rdata1Live_) {
      pendingInvalidate1 = true;
    }
  }

  void tick_commit() {
    if (pendingWrite && (latchedWaddr < DepthEntries)) {
      Wire<DataWidth> committed = applyStrb(mem_[latchedWaddr], latchedWdata, latchedWstrb);
      mem_[latchedWaddr] = committed;
      if (watch_enabled_ && latchedWaddr >= watch_lo_ && latchedWaddr <= watch_hi_) {
        MemWatchEvent ev;
        ev.kind = MemWatchEvent::Kind::Write;
        ev.port = 0;
        ev.addr = latchedWaddr;
        ev.data = committed;
        ev.strb = latchedWstrb;
        watch_events_.push_back(ev);
      }
    }
    if (pendingRead0) {
      rdata0 = rdata0Next;
      rdata0State_ = rdata0StateNext;
      rdata0Live_ = true;
    } else if (pendingInvalidate0) {
      rdata0State_ = ReadState::unknown(rdata0);
      rdata0Live_ = false;
    }
    if (pendingRead1) {
      rdata1 = rdata1Next;
      rdata1State_ = rdata1StateNext;
      rdata1Live_ = true;
    } else if (pendingInvalidate1) {
      rdata1State_ = ReadState::unknown(rdata1);
      rdata1Live_ = false;
    }
    pendingWrite = false;
    pendingRead0 = false;
    pendingRead1 = false;
    pendingInvalidate0 = false;
    pendingInvalidate1 = false;
  }

  void pokeEntry(std::size_t addr, Wire<DataWidth> value) {
    if (addr < DepthEntries)
      mem_[addr] = value;
  }
  void pokeEntry(std::size_t addr, std::uint64_t value) { pokeEntry(addr, Wire<DataWidth>(value)); }
  Wire<DataWidth> peekEntryBits(std::size_t addr) const {
    return (addr < DepthEntries) ? mem_[addr] : Wire<DataWidth>(0);
  }
  std::uint64_t peekEntry(std::size_t addr) const { return peekEntryBits(addr).value(); }

public:
  Wire<1> &clk;
  Wire<1> &rst;

  Wire<1> &ren0;
  Wire<AddrWidth> &raddr0;
  Wire<DataWidth> &rdata0;

  Wire<1> &ren1;
  Wire<AddrWidth> &raddr1;
  Wire<DataWidth> &rdata1;

  Wire<1> &wvalid;
  Wire<AddrWidth> &waddr;
  Wire<DataWidth> &wdata;
  Wire<StrbWidth> &wstrb;

  bool clkPrev = false;
  bool pendingWrite = false;
  bool pendingRead0 = false;
  bool pendingRead1 = false;
  bool pendingInvalidate0 = false;
  bool pendingInvalidate1 = false;
  bool rdata0Live_ = false;
  bool rdata1Live_ = false;
  std::size_t latchedWaddr = 0;
  std::size_t latchedRaddr0 = 0;
  std::size_t latchedRaddr1 = 0;
  Wire<DataWidth> latchedWdata{};
  Wire<StrbWidth> latchedWstrb{};
  Wire<DataWidth> rdata0Next{};
  Wire<DataWidth> rdata1Next{};
  ReadState rdata0State_ = ReadState::unknown();
  ReadState rdata1State_ = ReadState::unknown();
  ReadState rdata0StateNext = ReadState::unknown();
  ReadState rdata1StateNext = ReadState::unknown();

private:
  static constexpr std::size_t toIndex(Wire<AddrWidth> addr) {
    if constexpr (AddrWidth <= (sizeof(std::size_t) * 8u))
      return static_cast<std::size_t>(addr.value());
    constexpr unsigned hostBits = static_cast<unsigned>(sizeof(std::size_t) * 8u);
    constexpr unsigned useBits = (AddrWidth < hostBits) ? AddrWidth : hostBits;
    std::size_t out = 0;
    for (unsigned i = 0; i < useBits; i++) {
      if (addr.bit(i))
        out |= (std::size_t{1} << i);
    }
    return out;
  }

  static constexpr unsigned lastLaneBits() {
    unsigned rem = DataWidth % 8;
    return (rem == 0) ? 8 : rem;
  }

  static constexpr Wire<DataWidth> applyStrb(Wire<DataWidth> oldV, Wire<DataWidth> newV, Wire<StrbWidth> strb) {
    if constexpr (DataWidth <= 64) {
      std::uint64_t out = oldV.value();
      std::uint64_t src = newV.value();
      for (unsigned i = 0; i < StrbWidth; i++) {
        if (!strb.bit(i))
          continue;
        unsigned bitsInLane = (i == StrbWidth - 1) ? lastLaneBits() : 8u;
        std::uint64_t mask = ((1ull << bitsInLane) - 1ull) << (8u * i);
        out = (out & ~mask) | (src & mask);
      }
      return Wire<DataWidth>(out);
    }
    Wire<DataWidth> v = oldV;
    for (unsigned i = 0; i < StrbWidth; i++) {
      if (!strb.bit(i))
        continue;
      unsigned bitsInLane = (i == StrbWidth - 1) ? lastLaneBits() : 8u;
      for (unsigned b = 0; b < bitsInLane; ++b) {
        unsigned bitIdx = 8u * i + b;
        if (newV.bit(bitIdx))
          v = v | shl<DataWidth>(Wire<DataWidth>(1), bitIdx);
        else
          v = v & ~shl<DataWidth>(Wire<DataWidth>(1), bitIdx);
      }
    }
    return v;
  }

  std::array<Wire<DataWidth>, DepthEntries> mem_{};

  bool watch_enabled_ = false;
  std::size_t watch_lo_ = 0;
  std::size_t watch_hi_ = 0;
  std::vector<MemWatchEvent> watch_events_{};
};

} // namespace pyc::cpp
