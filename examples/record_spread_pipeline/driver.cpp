#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "record_spread_pipeline oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

constexpr std::uint32_t packetMask = (std::uint32_t{1} << 25) - 1;

struct Inputs {
  std::uint32_t base = 0;
  std::uint8_t header = 0;
  std::uint16_t payload = 0;
  std::uint8_t patch = 0;
};

std::uint32_t compose(std::uint8_t header, std::uint16_t payload) {
  return (static_cast<std::uint32_t>(header >> 4) << 21) |
         (static_cast<std::uint32_t>(header & 0xfu) << 17) |
         (std::uint32_t{payload} << 1) | 1u;
}

std::uint32_t applyPatch(std::uint32_t packet, std::uint8_t patch) {
  return (packet & ~((std::uint32_t{0xfu} << 17) | 1u)) |
         (static_cast<std::uint32_t>(patch >> 1) << 17) | (patch & 1u);
}

std::uint32_t finalValue(std::uint8_t header, std::uint16_t payload,
                         std::uint8_t patch) {
  return (static_cast<std::uint32_t>(header >> 4) << 21) |
         (static_cast<std::uint32_t>(patch >> 1) << 17) |
         (std::uint32_t{payload} << 1) | (patch & 1u);
}

std::string binary(std::uint32_t value, unsigned width) {
  std::string result;
  for (unsigned bit = width; bit; --bit)
    result.push_back((value >> (bit - 1)) & 1u ? '1' : '0');
  return result;
}

std::vector<Inputs> vectors() {
  std::vector<Inputs> result = {
      {},
      {packetMask, 0xff, 0xffff, 0x1f},
      {(0xfu << 21) | (0x8u << 17) | (0x8000u << 1), 0xf0, 0x8000, 0x1e},
      {(0x5u << 21) | (0xau << 17) | (0x5aa5u << 1) | 1u, 0x5a, 0x5aa5, 0x0b},
      {(0x8u << 21) | (0x1u << 17) | 2u, 0x81, 1, 0x10},
  };
  for (unsigned bit = 0; bit < 25; ++bit)
    result.push_back({std::uint32_t{1} << bit,
                      static_cast<std::uint8_t>(1u << (bit % 8)),
                      static_cast<std::uint16_t>(1u << (bit % 16)),
                      static_cast<std::uint8_t>(1u << (bit % 5))});
  require(result.size() == 30);
  return result;
}

struct Row {
  unsigned clock = 0, reset = 0;
  std::array<unsigned, 4> valid{};
  Inputs data;
  unsigned take = 0;
};

std::vector<Row> stimulus() {
  const auto values = vectors();
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, std::array<unsigned, 4> valid,
                 Inputs data, unsigned take) {
    rows.push_back({clock, reset, valid, data, take});
  };
  auto edge = [&](std::array<unsigned, 4> valid, Inputs data, unsigned take = 1,
                  unsigned reset = 0) {
    add(1, reset, valid, data, take);
    add(0, reset, valid, data, take);
  };
  const std::array<unsigned, 4> all{1, 1, 1, 1}, none{0, 0, 0, 0};
  add(0, 1, none, {}, 0);
  edge(none, {}, 0, 1);
  // Header/payload/patch arrive without base. No compose/output may occur.
  edge({0, 1, 1, 1}, values[0]);
  edge({0, 1, 1, 1}, values[1]);
  edge({1, 0, 0, 0}, values[0]);
  edge(none, {}); // Compose after base joins the three waiting peers.
  edge(none, {}); // Apply waiting patch.
  edge(none, {}); // Consume result.
  edge(none, {}, 0, 1);

  // Independently omit header, payload and patch. Peers wait in their own
  // queues; the late stream releases the FIFO group atomically.
  for (unsigned missing = 1; missing < 4; ++missing) {
    std::array<unsigned, 4> peers = all;
    peers[missing] = 0;
    std::array<unsigned, 4> onlyMissing = none;
    onlyMissing[missing] = 1;
    edge(peers, values[missing]);
    edge(peers, values[missing + 1]);
    edge(onlyMissing, values[missing]);
    edge(none, {});
    edge(none, {});
    edge(none, {});
    edge(none, {}, 0, 1);
  }

  // Fill all six physical slots while the sink is stalled.
  edge(all, values[0], 0);
  edge({1, 1, 1, 1},
       {values[1].base, values[1].header, values[1].payload, values[0].patch},
       0);
  edge({1, 1, 1, 1},
       {values[2].base, values[2].header, values[2].payload, values[1].patch},
       0);
  add(1, 0, all, values[3], 0);
  add(1, 0, all, values[4], 1); // Active repeated high, no transfer.
  add(0, 0, all, values[3], 1);
  add(0, 0, all, values[4], 1); // Active repeated low, no transfer.
  edge(all,
       {values[3].base, values[3].header, values[3].payload, values[2].patch});
  edge(all,
       {values[4].base, values[4].header, values[4].payload, values[3].patch});
  edge(none, {}, 1, 1); // Reset drops every outstanding component.

  // Drain each independent vector transaction before the next. This covers
  // every bit of all four input record types without relying on DUT results.
  for (const Inputs &value : values) {
    edge(all, value);
    edge(none, {});
    edge(none, {});
    edge(none, {});
  }
  edge(none, {}, 1, 1);
  require(rows.size() == 317);
  return rows;
}

struct Decisions {
  bool popUpdated = false, readyUpdated = true;
  bool fireApply = false, readyComposed = true;
  bool fireCompose = false;
  std::array<bool, 4> ready{true, true, true, true};
};

class Golden {
public:
  Decisions decisions(bool take) const {
    Decisions result;
    result.popUpdated = updated.has_value() && take;
    result.readyUpdated = !updated.has_value() || result.popUpdated;
    result.fireApply =
        composed.has_value() && patch.has_value() && result.readyUpdated;
    result.readyComposed = !composed.has_value() || result.fireApply;
    result.fireCompose = base.has_value() && header.has_value() &&
                         payload.has_value() && result.readyComposed;
    result.ready = {!base.has_value() || result.fireCompose,
                    !header.has_value() || result.fireCompose,
                    !payload.has_value() || result.fireCompose,
                    !patch.has_value() || result.fireApply};
    return result;
  }

  std::optional<std::uint32_t> output() const { return updated; }

  void commit(const Row &row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        base.reset();
        header.reset();
        payload.reset();
        patch.reset();
        composed.reset();
        updated.reset();
      } else {
        const Decisions d = decisions(row.take);
        const auto oldHeader = header;
        const auto oldPayload = payload;
        const auto oldComposed = composed;
        const auto oldPatch = patch;
        if (d.fireApply)
          updated = applyPatch(*oldComposed, *oldPatch);
        else if (d.popUpdated)
          updated.reset();
        if (d.fireCompose)
          composed = compose(*oldHeader, *oldPayload);
        else if (d.fireApply)
          composed.reset();
        updateInput(base, row.valid[0], d.ready[0], d.fireCompose,
                    row.data.base);
        updateInput(header, row.valid[1], d.ready[1], d.fireCompose,
                    row.data.header);
        updateInput(payload, row.valid[2], d.ready[2], d.fireCompose,
                    row.data.payload);
        updateInput(patch, row.valid[3], d.ready[3], d.fireApply,
                    row.data.patch);
      }
    }
    lastClock = row.clock;
  }

private:
  template <class T>
  static void updateInput(std::optional<T> &slot, unsigned valid, bool ready,
                          bool pop, T value) {
    if (valid && ready)
      slot = value;
    else if (pop)
      slot.reset();
  }
  std::optional<std::uint32_t> base, composed, updated;
  std::optional<std::uint8_t> header, patch;
  std::optional<std::uint16_t> payload;
  bool lastClock = false;
};

pyc_dut::Inputs inputs(const Row &row) {
  pyc_dut::Inputs result;
  result.pyc_7079635f636c6b = known<1>(row.clock);
  result.pyc_7079635f727374 = known<1>(row.reset);
  result.base_valid = known<1>(row.valid[0]);
  result.base =
      decltype(result.base)::fromPacked(known<25>(row.data.base).packed());
  result.header_valid = known<1>(row.valid[1]);
  result.header =
      decltype(result.header)::fromPacked(known<8>(row.data.header).packed());
  result.payload_valid = known<1>(row.valid[2]);
  result.payload = decltype(result.payload)::fromPacked(
      known<16>(row.data.payload).packed());
  result.patch_valid = known<1>(row.valid[3]);
  result.patch =
      decltype(result.patch)::fromPacked(known<5>(row.data.patch).packed());
  result.take = known<1>(row.take);
  return result;
}

template <class Output>
void check(const Output &output, const Decisions &d,
           std::optional<std::uint32_t> expected, unsigned index, bool emit) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  for (unsigned n = 0; n < 4; ++n)
    require(packed.value().bit(29 - n) == d.ready[n]);
  require(packed.value().bit(25) == expected.has_value());
  require(gfsim::extract<25>(packed, 0).value() ==
          gfsim::Bits<25>{expected.value_or(0)});
  if (emit)
    std::cout << "WORK " << index << ' ' << d.ready[0] << d.ready[1]
              << d.ready[2] << d.ready[3] << ' ' << expected.has_value() << ' '
              << binary(expected.value_or(0), 25) << '\n';
}

template <unsigned Width> auto symbolic(std::string_view text) {
  require(text.size() == Width);
  gfsim::Bits<Width> value{0}, knownMask{0}, zMask{0};
  for (unsigned bit = 0; bit < Width; ++bit) {
    const char symbol = text[Width - bit - 1];
    require(symbol == '0' || symbol == '1' || symbol == 'x' || symbol == 'z');
    const unsigned word = bit / 64;
    const std::uint64_t one = std::uint64_t{1} << (bit % 64);
    if (symbol == '1')
      value.setWord(word, value.word(word) | one);
    if (symbol == '0' || symbol == '1')
      knownMask.setWord(word, knownMask.word(word) | one);
    if (symbol == 'z')
      zMask.setWord(word, zMask.word(word) | one);
  }
  return gfsim::wire<gfsim::Bits<Width>>::fromPacked(
      gfsim::FourState<Width>::fromMasks(value, knownMask, zMask));
}

struct Ledger {
  std::deque<std::uint32_t> base, composed, updated;
  std::deque<std::uint8_t> header, patch;
  std::deque<std::uint16_t> payload;
  std::array<std::uint64_t, 4> accepted{}, consumed{};
  std::uint64_t retired = 0, dropped = 0, composeCount = 0, applyCount = 0;
  std::size_t peakSlots = 0;
  bool lastClock = false;

  std::size_t slots() const {
    return base.size() + header.size() + payload.size() + patch.size() +
           composed.size() + updated.size();
  }
  std::uint64_t outstandingComponents() const {
    return base.size() + header.size() + payload.size() + patch.size() +
           3 * composed.size() + 4 * updated.size();
  }
  std::uint64_t acceptedComponents() const {
    return accepted[0] + accepted[1] + accepted[2] + accepted[3];
  }
};

struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  Ledger ledger;
  unsigned sampled = 0, stalled = 0;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == self.rows.size())
      return false;
    self.dut.drive(inputs(self.rows[epoch]));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    const Row &row = self.rows[self.sampled];
    const Decisions d = self.golden.decisions(row.take);
    const auto expected = self.golden.output();
    const auto output = self.dut.sample();
    check(output, d, expected, self.sampled, true);
    const auto &packed = output.result.packed();
    std::array<bool, 4> actualReady{};
    for (unsigned n = 0; n < 4; ++n)
      actualReady[n] = packed.value().bit(29 - n);
    const bool actualValid = packed.value().bit(25);
    const auto actualData = static_cast<std::uint32_t>(
        gfsim::extract<25>(packed, 0).value().value());
    self.stalled += row.valid[0] && !actualReady[0];
    if (row.clock && !self.ledger.lastClock) {
      if (row.reset) {
        self.ledger.dropped += self.ledger.outstandingComponents();
        self.ledger.base.clear();
        self.ledger.header.clear();
        self.ledger.payload.clear();
        self.ledger.patch.clear();
        self.ledger.composed.clear();
        self.ledger.updated.clear();
      } else {
        const bool popUpdated = actualValid && row.take;
        const bool readyUpdated = self.ledger.updated.empty() || popUpdated;
        const bool fireApply = !self.ledger.composed.empty() &&
                               !self.ledger.patch.empty() && readyUpdated;
        const bool readyComposed = self.ledger.composed.empty() || fireApply;
        const bool fireCompose = !self.ledger.base.empty() &&
                                 !self.ledger.header.empty() &&
                                 !self.ledger.payload.empty() && readyComposed;
        if (popUpdated) {
          require(!self.ledger.updated.empty() &&
                  actualData == self.ledger.updated.front());
          self.ledger.updated.pop_front();
          ++self.ledger.retired;
        }
        if (fireApply) {
          self.ledger.updated.push_back(applyPatch(self.ledger.composed.front(),
                                                   self.ledger.patch.front()));
          self.ledger.composed.pop_front();
          self.ledger.patch.pop_front();
          ++self.ledger.consumed[3];
          ++self.ledger.applyCount;
        }
        if (fireCompose) {
          self.ledger.composed.push_back(
              compose(self.ledger.header.front(), self.ledger.payload.front()));
          self.ledger.base.pop_front();
          self.ledger.header.pop_front();
          self.ledger.payload.pop_front();
          ++self.ledger.consumed[0];
          ++self.ledger.consumed[1];
          ++self.ledger.consumed[2];
          ++self.ledger.composeCount;
        }
        if (actualReady[0] && row.valid[0]) {
          self.ledger.base.push_back(row.data.base);
          ++self.ledger.accepted[0];
        }
        if (actualReady[1] && row.valid[1]) {
          self.ledger.header.push_back(row.data.header);
          ++self.ledger.accepted[1];
        }
        if (actualReady[2] && row.valid[2]) {
          self.ledger.payload.push_back(row.data.payload);
          ++self.ledger.accepted[2];
        }
        if (actualReady[3] && row.valid[3]) {
          self.ledger.patch.push_back(row.data.patch);
          ++self.ledger.accepted[3];
        }
      }
      self.ledger.peakSlots =
          std::max(self.ledger.peakSlots, self.ledger.slots());
      require(
          self.ledger.base.size() <= 1 && self.ledger.header.size() <= 1 &&
          self.ledger.payload.size() <= 1 && self.ledger.patch.size() <= 1 &&
          self.ledger.composed.size() <= 1 && self.ledger.updated.size() <= 1);
    }
    self.ledger.lastClock = row.clock;
    self.golden.commit(row);
    ++self.sampled;
  }
};

void nativeFourState(unsigned workers) {
  struct FourCase {
    std::string_view base, header, payload, patch, expected;
  };
  constexpr FourCase cases[] = {
      {"zzzzzzzzzzzzzzzzzzzzzzzzz", "1x0zzzzz", "10xz10xz10xz10xz", "zx100",
       "1x0zzx1010xz10xz10xz10xz0"},
      {"xxxxxxxxxxxxxxxxxxxxxxxxx", "zx10xzxz", "zxzxzxzxzxzxzxzx", "01xzz",
       "zx1001xzzxzxzxzxzxzxzxzxz"},
      {"10100011zzzzzzzzzzzzzzzzx", "x0z10000", "xxxxxxxxxxxxxxxx", "11111",
       "x0z11111xxxxxxxxxxxxxxxx1"},
      {"0000000000000000000000000", "0z1x1111", "zzzzzzzzzzzzzzzz", "x0z1x",
       "0z1xx0z1zzzzzzzzzzzzzzzzx"},
  };
  for (unsigned ordinal = 0; ordinal < std::size(cases); ++ordinal) {
    pyc_dut dut(workers);
    gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
    constexpr std::string_view config =
        R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":16,"schema":"pycircuit-model-config","version":"1"})";
    require(executor.ConfigureJson(
                reinterpret_cast<const std::uint8_t *>(config.data()),
                config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    pyc_dut::Inputs input;
    input.pyc_7079635f636c6b = known<1>(0);
    input.pyc_7079635f727374 = known<1>(0);
    input.base_valid = input.header_valid = input.payload_valid =
        input.patch_valid = known<1>(1);
    input.base = decltype(input.base)::fromPacked(
        symbolic<25>(cases[ordinal].base).packed());
    input.header = decltype(input.header)::fromPacked(
        symbolic<8>(cases[ordinal].header).packed());
    input.payload = decltype(input.payload)::fromPacked(
        symbolic<16>(cases[ordinal].payload).packed());
    input.patch = decltype(input.patch)::fromPacked(
        symbolic<5>(cases[ordinal].patch).packed());
    input.take = known<1>(1);
    dut.drive(input);
    require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    auto step = [&](unsigned clock, unsigned valid) {
      input.pyc_7079635f636c6b = known<1>(clock);
      input.base_valid = input.header_valid = input.payload_valid =
          input.patch_valid = known<1>(valid);
      dut.drive(input);
      PycircuitModelStepResultV1 status{sizeof(status)};
      require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    };
    step(0, 1);
    step(1, 1); // E0 inputs.
    step(0, 0);
    step(1, 0); // E1 compose.
    step(0, 0);
    step(1, 0); // E2 update.
    step(0, 0);
    const auto output = dut.sample().result.packed();
    require(output.knownMask().bit(25) && output.value().bit(25));
    const auto actual = gfsim::extract<25>(output, 0);
    const auto expected = symbolic<25>(cases[ordinal].expected).packed();
    require(actual.knownMask() == expected.knownMask());
    require(actual.zMask() == expected.zMask());
    require((actual.value() & actual.knownMask()) ==
            (expected.value() & expected.knownMask()));
    std::cout << "FOUR " << ordinal << '\n';
  }
}

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == context.rows.size());
  require(context.stalled >= 3 && context.ledger.peakSlots == 6);
  require(context.ledger.composeCount == context.ledger.consumed[0] &&
          context.ledger.composeCount == context.ledger.consumed[1] &&
          context.ledger.composeCount == context.ledger.consumed[2]);
  require(context.ledger.applyCount == context.ledger.consumed[3] &&
          context.ledger.retired <= context.ledger.applyCount);
  require(context.ledger.acceptedComponents() ==
          4 * context.ledger.retired + context.ledger.dropped +
              context.ledger.outstandingComponents());
  std::cout << "HISTORY " << context.ledger.acceptedComponents() << ' '
            << context.ledger.retired << ' ' << context.ledger.dropped << ' '
            << context.ledger.outstandingComponents() << ' '
            << context.ledger.peakSlots << ' ' << context.ledger.composeCount
            << ' ' << context.ledger.applyCount << ' '
            << context.ledger.accepted[0] << ' ' << context.ledger.accepted[1]
            << ' ' << context.ledger.accepted[2] << ' '
            << context.ledger.accepted[3] << '\n';
  nativeFourState(runner.workers());
  return result;
}
