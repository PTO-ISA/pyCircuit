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
    std::cerr << "bit_widths oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

struct Tag {
  std::uint16_t value = 0;
  std::uint16_t mask = 0;
  std::uint16_t rotated = 0;
  std::uint64_t sequence = 0;
};

bool operator==(const Tag &left, const Tag &right) {
  return left.value == right.value && left.mask == right.mask &&
         left.rotated == right.rotated && left.sequence == right.sequence;
}

constexpr std::uint16_t u13 = (1u << 13) - 1;
constexpr std::uint64_t u37 = (std::uint64_t{1} << 37) - 1;

Tag transform(Tag input) {
  const auto value = static_cast<std::uint16_t>(input.value & u13);
  return {static_cast<std::uint16_t>(((value & input.mask) ^ 1u) & u13),
          static_cast<std::uint16_t>(input.mask & u13),
          static_cast<std::uint16_t>(((value << 1) | (value >> 12)) & u13),
          input.sequence & u37};
}

gfsim::Bits<76> pack(Tag input) {
  gfsim::Bits<76> result{0};
  auto field = [&](std::uint64_t value, unsigned width, unsigned low) {
    for (unsigned bit = 0; bit < width; ++bit)
      if ((value >> bit) & 1u) {
        const unsigned index = low + bit;
        result.setWord(index / 64, result.word(index / 64) |
                                       (std::uint64_t{1} << (index % 64)));
      }
  };
  field(input.sequence & u37, 37, 0);
  field(input.rotated & u13, 13, 37);
  field(input.mask & u13, 13, 50);
  field(input.value & u13, 13, 63);
  return result;
}

Tag unpack(const gfsim::Bits<76> &input) {
  auto field = [&](unsigned width, unsigned low) {
    std::uint64_t result = 0;
    for (unsigned bit = 0; bit < width; ++bit)
      if (input.bit(low + bit))
        result |= std::uint64_t{1} << bit;
    return result;
  };
  return {static_cast<std::uint16_t>(field(13, 63)),
          static_cast<std::uint16_t>(field(13, 50)),
          static_cast<std::uint16_t>(field(13, 37)), field(37, 0)};
}

std::string binary(const gfsim::Bits<76> &value) {
  std::string result;
  result.reserve(76);
  for (unsigned bit = 76; bit; --bit)
    result.push_back(value.bit(bit - 1) ? '1' : '0');
  return result;
}

struct Row {
  unsigned clock = 0;
  unsigned reset = 0;
  unsigned valid = 0;
  Tag data;
  unsigned take = 0;
};

std::vector<Tag> vectors() {
  std::vector<Tag> result = {
      {},
      {u13, u13, u13, u37},
      {std::uint16_t{1} << 12, u13, 0x1555,
       (std::uint64_t{1} << 36) | (std::uint64_t{1} << 32) |
           (std::uint64_t{1} << 31) | 1},
      {0x1555, 0x0f0f, u13, 0x1555555555ULL},
      {u13, 0, 1, std::uint64_t{1} << 36},
  };
  for (unsigned bit = 0; bit < 76; ++bit) {
    gfsim::Bits<76> packed{0};
    packed.setWord(bit / 64, std::uint64_t{1} << (bit % 64));
    result.push_back(unpack(packed));
  }
  require(result.size() == 81);
  return result;
}

std::vector<Row> stimulus() {
  const auto values = vectors();
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, unsigned valid, Tag data,
                 unsigned take) {
    rows.push_back({clock, reset, valid, data, take});
  };
  auto edge = [&](Tag data, unsigned valid = 1, unsigned take = 1,
                  unsigned reset = 0) {
    add(1, reset, valid, data, take);
    add(0, reset, valid, data, take);
  };
  add(0, 1, 0, {}, 0);
  edge({}, 0, 0, 1);
  edge(values[0]);
  edge(values[1]);
  edge(values[2]);
  edge(values[3], 1, 0);
  add(1, 0, 1, values[4], 0); // Rising edge, blocked while both stages full.
  add(1, 0, 1, values[0], 1); // Repeated high: ready changes, no transfer.
  add(0, 0, 1, values[1], 1); // Falling level, changed offer, no transfer.
  add(0, 0, 1, values[2], 1); // Repeated low: changed offer, no transfer.
  edge(values[4], 1, 1);
  edge({}, 0, 1);
  edge({}, 0, 1);
  edge({}, 1, 1, 1);
  for (const Tag &value : values)
    edge(value);
  edge({}, 0, 1);
  edge({}, 0, 1);
  edge(values[0]);
  edge(values[1]);
  edge(values[2], 1, 1, 1);
  require(rows.size() == 195);
  return rows;
}

struct Expected {
  bool ready = true;
  bool valid = false;
  Tag data;
};

class Golden {
public:
  Expected output(bool take) const {
    const bool popSecond = second.has_value() && take;
    const bool readySecond = !second.has_value() || popSecond;
    const bool popFirst = first.has_value() && readySecond;
    return {!first.has_value() || popFirst, second.has_value(),
            second.value_or(Tag{})};
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
        const bool pushFirst = row.valid && readyFirst;
        const bool pushSecond = oldFirst.has_value() && readySecond;
        if (pushSecond)
          second = transform(*oldFirst);
        else if (popSecond)
          second.reset();
        if (pushFirst)
          first = row.data;
        else if (popFirst)
          first.reset();
      }
    }
    lastClock = row.clock;
  }

private:
  std::optional<Tag> first;
  std::optional<Tag> second;
  bool lastClock = false;
};

pyc_dut::Inputs inputs(const Row &row) {
  pyc_dut::Inputs result;
  result.pyc_7079635f636c6b = known<1>(row.clock);
  result.pyc_7079635f727374 = known<1>(row.reset);
  result.valid = known<1>(row.valid);
  result.data = decltype(result.data)::fromPacked(
      gfsim::wire<gfsim::Bits<76>>::known(pack(row.data)).packed());
  result.take = known<1>(row.take);
  return result;
}

template <class Output>
void check(const Output &output, const Expected &expected, unsigned index,
           bool emit) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  require(packed.value().bit(77) == expected.ready);
  require(packed.value().bit(76) == expected.valid);
  require(gfsim::extract<76>(packed, 0).value() == pack(expected.data));
  if (emit)
    std::cout << "WORK " << index << ' ' << unsigned(expected.ready) << ' '
              << unsigned(expected.valid) << ' ' << binary(pack(expected.data))
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
    std::string_view input;
    std::string_view expected;
  };
  constexpr FourCase cases[] = {
      {"xz10xz10xz10x0000000000000zzzzzzzzzzzzzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxz"
       "xzxzx",
       "00000000000010000000000000x10xx10xx10xxxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxz"
       "xzxzx"},
      {"10xz10xz10xz11111111111111xxxxxxxxxxxxxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzx"
       "zxzxz",
       "10xx10xx10xx011111111111110xx10xx10xx11zxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzx"
       "zxzxz"},
      {"0000000000000xzxzxzxzxzxzxzxzxzxzxzxzxzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
       "zzzzz",
       "0000000000001xzxzxzxzxzxzx0000000000000zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
       "zzzzz"},
      {"z00000000000x11111111111111010101010101x000000000000000000000000000000z"
       "00000",
       "x00000000000x111111111111100000000000xxx000000000000000000000000000000z"
       "00000"},
  };
  for (unsigned ordinal = 0; ordinal < std::size(cases); ++ordinal) {
    const FourCase &row = cases[ordinal];
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
    input.data =
        decltype(input.data)::fromPacked(symbolic<76>(row.input).packed());
    input.take = known<1>(1);
    dut.drive(input);
    require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    auto step = [&](unsigned clock, unsigned valid) {
      input.pyc_7079635f636c6b = known<1>(clock);
      input.valid = known<1>(valid);
      dut.drive(input);
      PycircuitModelStepResultV1 status{sizeof(status)};
      require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
      require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    };
    step(0, 1);
    step(1, 1); // E0: input -> q1.
    step(0, 0);
    step(1, 0); // E1: transformed old q1 -> q2.
    step(0, 0); // Following Work exposes q2.
    const auto output = dut.sample().result.packed();
    require(output.knownMask().bit(76) && output.value().bit(76));
    const auto actual = gfsim::extract<76>(output, 0);
    const auto expected = symbolic<76>(row.expected).packed();
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
  unsigned sampled = 0;
  unsigned stalled = 0;
  std::deque<Tag> acceptedHistory;
  std::uint64_t accepted = 0;
  std::uint64_t retired = 0;
  std::uint64_t dropped = 0;
  std::size_t peakOutstanding = 0;
  bool ledgerLastClock = false;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }

  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == self.rows.size())
      return false;
    require(epoch < self.rows.size());
    self.dut.drive(inputs(self.rows[epoch]));
    return true;
  }

  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    const Row &row = self.rows[self.sampled];
    const Expected expected = self.golden.output(row.take);
    if (self.sampled == 11)
      require(!expected.ready);
    if (self.sampled >= 12 && self.sampled <= 15)
      require(expected.ready);
    const auto output = self.dut.sample();
    check(output, expected, self.sampled, true);
    const auto &packed = output.result.packed();
    const bool actualReady = packed.value().bit(77);
    const bool actualValid = packed.value().bit(76);
    const Tag actualData = unpack(gfsim::extract<76>(packed, 0).value());
    self.stalled += row.valid && !expected.ready;
    if (row.clock && !self.ledgerLastClock) {
      if (row.reset) {
        self.dropped += self.acceptedHistory.size();
        self.acceptedHistory.clear();
      } else {
        if (actualValid && row.take) {
          require(!self.acceptedHistory.empty());
          require(actualData == self.acceptedHistory.front());
          self.acceptedHistory.pop_front();
          ++self.retired;
        }
        if (actualReady && row.valid) {
          self.acceptedHistory.push_back(transform(row.data));
          ++self.accepted;
        }
      }
      self.peakOutstanding =
          std::max(self.peakOutstanding, self.acceptedHistory.size());
      require(self.acceptedHistory.size() <= 2);
    }
    self.ledgerLastClock = row.clock;
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
  require(context.stalled >= 3 && context.peakOutstanding == 2);
  require(context.accepted ==
          context.retired + context.dropped + context.acceptedHistory.size());
  std::cout << "HISTORY " << context.accepted << ' ' << context.retired << ' '
            << context.dropped << ' ' << context.acceptedHistory.size() << ' '
            << context.peakOutstanding << '\n';
  nativeFourState(runner.workers());
  return result;
}
