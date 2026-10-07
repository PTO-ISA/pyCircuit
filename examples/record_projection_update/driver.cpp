#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
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
    std::cerr << "record_projection_update oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

constexpr std::uint32_t packetMask = (std::uint32_t{1} << 29) - 1;

std::uint32_t update(std::uint32_t packet) {
  return (packet | 1u) & packetMask;
}

std::string binary(std::uint32_t value, unsigned width) {
  std::string result;
  result.reserve(width);
  for (unsigned bit = width; bit; --bit)
    result.push_back((value >> (bit - 1)) & 1u ? '1' : '0');
  return result;
}

std::vector<std::uint32_t> vectors() {
  std::vector<std::uint32_t> result = {
      0,
      packetMask,
      (0xfu << 25) | (0x80u << 17) | (0x8000u << 1),
      (0x5u << 25) | (0xa5u << 17) | (0x5aa5u << 1) | 1u,
      (0x8u << 25) | (0x01u << 17) | (0x0001u << 1),
  };
  for (unsigned bit = 0; bit < 29; ++bit)
    result.push_back(std::uint32_t{1} << bit);
  require(result.size() == 34);
  return result;
}

struct Row {
  unsigned clock = 0, reset = 0, valid = 0;
  std::uint32_t data = 0;
  unsigned take = 0;
};

std::vector<Row> stimulus() {
  const auto values = vectors();
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, unsigned valid,
                 std::uint32_t data, unsigned take) {
    rows.push_back({clock, reset, valid, data & packetMask, take});
  };
  auto edge = [&](std::uint32_t data, unsigned valid = 1, unsigned take = 1,
                  unsigned reset = 0) {
    add(1, reset, valid, data, take);
    add(0, reset, valid, data, take);
  };
  add(0, 1, 0, 0, 0);
  edge(0, 0, 0, 1);
  edge(values[0]);
  edge(values[1]);
  edge(values[2]);
  edge(values[3], 1, 0);
  add(1, 0, 1, values[4], 0);
  add(1, 0, 1, values[0], 1);
  add(0, 0, 1, values[1], 1);
  add(0, 0, 1, values[2], 1);
  edge(values[4]);
  edge(0, 0, 1);
  edge(0, 0, 1);
  edge(0, 1, 1, 1);
  for (std::uint32_t value : values)
    edge(value);
  edge(0, 0, 1);
  edge(0, 0, 1);
  edge(values[0]);
  edge(values[1]);
  edge(values[2], 1, 1, 1);
  require(rows.size() == 101);
  return rows;
}

struct Expected {
  bool ready = true, valid = false;
  std::uint32_t data = 0;
};

class Golden {
public:
  Expected output(bool take) const {
    const bool popSecond = second.has_value() && take;
    const bool readySecond = !second.has_value() || popSecond;
    const bool popFirst = first.has_value() && readySecond;
    return {!first.has_value() || popFirst, second.has_value(),
            second.value_or(0)};
  }

  void commit(const Row &row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        first.reset();
        second.reset();
      } else {
        const auto oldFirst = first;
        const bool popSecond = second.has_value() && row.take;
        const bool readySecond = !second.has_value() || popSecond;
        const bool popFirst = first.has_value() && readySecond;
        const bool readyFirst = !first.has_value() || popFirst;
        if (oldFirst.has_value() && readySecond)
          second = update(*oldFirst);
        else if (popSecond)
          second.reset();
        if (row.valid && readyFirst)
          first = row.data;
        else if (popFirst)
          first.reset();
      }
    }
    lastClock = row.clock;
  }

private:
  std::optional<std::uint32_t> first;
  std::optional<std::uint32_t> second;
  bool lastClock = false;
};

pyc_dut::Inputs inputs(const Row &row) {
  pyc_dut::Inputs result;
  result.pyc_7079635f636c6b = known<1>(row.clock);
  result.pyc_7079635f727374 = known<1>(row.reset);
  result.valid = known<1>(row.valid);
  result.data = decltype(result.data)::fromPacked(known<29>(row.data).packed());
  result.take = known<1>(row.take);
  return result;
}

template <class Output>
void check(const Output &output, Expected expected, unsigned index, bool emit) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  require(packed.value().bit(30) == expected.ready);
  require(packed.value().bit(29) == expected.valid);
  require(gfsim::extract<29>(packed, 0).value() ==
          gfsim::Bits<29>{expected.data});
  if (emit)
    std::cout << "WORK " << index << ' ' << unsigned(expected.ready) << ' '
              << unsigned(expected.valid) << ' ' << binary(expected.data, 29)
              << '\n';
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

void nativeFourState(unsigned workers) {
  struct FourCase {
    std::string_view input, expected;
  };
  constexpr FourCase cases[] = {
      {"1x0zzzzzzzzzxxxxxxxxxxxxxxxx0", "1x0zzzzzzzzzxxxxxxxxxxxxxxxx1"},
      {"zx10xzxzxzxzzxzxzxzxzxzxzxzx1", "zx10xzxzxzxzzxzxzxzxzxzxzxzx1"},
      {"x0z1111111110000000000000000x", "x0z11111111100000000000000001"},
      {"0z1x00000000zzzzzzzzzzzzzzzzz", "0z1x00000000zzzzzzzzzzzzzzzz1"},
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
    input.valid = known<1>(1);
    input.data = decltype(input.data)::fromPacked(
        symbolic<29>(cases[ordinal].input).packed());
    input.take = known<1>(1);
    dut.drive(input);
    require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    auto step = [&](unsigned clock, unsigned valid) {
      input.pyc_7079635f636c6b = known<1>(clock);
      input.valid = known<1>(valid);
      dut.drive(input);
      PycircuitModelStepResultV1 status{sizeof(status)};
      require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    };
    step(0, 1);
    step(1, 1);
    step(0, 0);
    step(1, 0);
    step(0, 0);
    const auto output = dut.sample().result.packed();
    require(output.knownMask().bit(29) && output.value().bit(29));
    const auto actual = gfsim::extract<29>(output, 0);
    const auto expected = symbolic<29>(cases[ordinal].expected).packed();
    require(actual.knownMask() == expected.knownMask());
    require(actual.zMask() == expected.zMask());
    require((actual.value() & actual.knownMask()) ==
            (expected.value() & expected.knownMask()));
    std::cout << "FOUR " << ordinal << '\n';
  }
}

struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  std::deque<std::uint32_t> history;
  unsigned sampled = 0, stalled = 0;
  std::uint64_t accepted = 0, retired = 0, dropped = 0;
  std::size_t peak = 0;
  bool lastClock = false;

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
    const Expected expected = self.golden.output(row.take);
    const auto output = self.dut.sample();
    check(output, expected, self.sampled, true);
    const auto &packed = output.result.packed();
    const bool actualReady = packed.value().bit(30);
    const bool actualValid = packed.value().bit(29);
    const auto actualData = static_cast<std::uint32_t>(
        gfsim::extract<29>(packed, 0).value().value());
    self.stalled += row.valid && !expected.ready;
    if (row.clock && !self.lastClock) {
      if (row.reset) {
        self.dropped += self.history.size();
        self.history.clear();
      } else {
        if (actualValid && row.take) {
          require(!self.history.empty() && actualData == self.history.front());
          self.history.pop_front();
          ++self.retired;
        }
        if (actualReady && row.valid) {
          self.history.push_back(update(row.data));
          ++self.accepted;
        }
      }
      self.peak = std::max(self.peak, self.history.size());
      require(self.history.size() <= 2);
    }
    self.lastClock = row.clock;
    self.golden.commit(row);
    ++self.sampled;
  }
};

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
  require(context.stalled >= 3 && context.peak == 2);
  require(context.accepted ==
          context.retired + context.dropped + context.history.size());
  std::cout << "HISTORY " << context.accepted << ' ' << context.retired << ' '
            << context.dropped << ' ' << context.history.size() << ' '
            << context.peak << '\n';
  nativeFourState(runner.workers());
  return result;
}
