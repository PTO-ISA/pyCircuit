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
#include <vector>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "array_reductions oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

struct Request {
  std::uint8_t first = 0, second = 0, third = 0;
};

std::uint32_t packRequest(Request value) {
  return (std::uint32_t{value.first} << 16) |
         (std::uint32_t{value.second} << 8) | value.third;
}

std::uint64_t reduce(Request request) {
  const std::array<std::uint8_t, 3> values{request.first, request.second,
                                           request.third};
  const std::array<bool, 3> flags{request.first != 0, request.second != 0,
                                  request.third != 0};
  const unsigned count =
      unsigned(flags[0]) + unsigned(flags[1]) + unsigned(flags[2]);
  unsigned firstIndex = 0;
  unsigned bestIndex = 0;
  bool selected = false;
  for (unsigned index = 0; index < values.size(); ++index) {
    if (!flags[index])
      continue;
    if (!selected) {
      firstIndex = index;
      bestIndex = index;
      selected = true;
    } else if (values[index] < values[bestIndex]) {
      bestIndex = index;
    }
  }
  unsigned rangeBestIndex = 0;
  for (unsigned index = 1; index < values.size(); ++index)
    if (values[index] % 5 < values[rangeBestIndex] % 5)
      rangeBestIndex = index;

  const auto append = [](std::uint64_t packed, unsigned width,
                         std::uint64_t value) {
    return (packed << width) | (value & ((std::uint64_t{1} << width) - 1));
  };
  std::uint64_t packed = 0;
  packed = append(packed, 1, count == 3);
  packed = append(packed, 1, count != 0);
  packed = append(packed, 2, count);
  packed = append(packed, 2, count);
  packed = append(packed, 8, request.first + request.second + request.third);
  packed = append(
      packed, 8, std::uint64_t{request.first} * request.second * request.third);
  packed = append(packed, 8, *std::min_element(values.begin(), values.end()));
  packed = append(packed, 8, *std::max_element(values.begin(), values.end()));
  packed = append(packed, 1, flags[0] ^ flags[1] ^ flags[2]);
  packed = append(packed, 2, firstIndex);
  packed = append(packed, 1, selected);
  packed = append(packed, 2, bestIndex);
  packed = append(packed, 1, selected);
  packed = append(packed, 2, rangeBestIndex);
  return packed;
}

std::string binary(std::uint64_t value, unsigned width) {
  std::string result;
  result.reserve(width);
  for (unsigned bit = width; bit; --bit)
    result.push_back((value >> (bit - 1)) & 1u ? '1' : '0');
  return result;
}

std::vector<Request> vectors() {
  std::vector<Request> result = {
      {0, 0, 0}, {0, 0, 5}, {7, 7, 9},  {3, 1, 2},     {255, 2, 3},
      {0, 5, 0}, {9, 7, 7}, {6, 1, 11}, {255, 255, 2}, {4, 5, 6},
  };
  std::uint32_t state = 0x83a7d529u;
  for (unsigned index = 0; index < 64; ++index) {
    state = state * 1664525u + 1013904223u;
    const auto first = static_cast<std::uint8_t>(state >> 24);
    state = state * 1664525u + 1013904223u;
    const auto second = static_cast<std::uint8_t>(state >> 24);
    state = state * 1664525u + 1013904223u;
    const auto third = static_cast<std::uint8_t>(state >> 24);
    result.push_back({first, second, third});
  }
  constexpr std::array<std::uint64_t, 5> historical{
      0, 46222438042548ULL, 138742242087716ULL, 138590206166829ULL,
      138581213839148ULL};
  for (unsigned index = 0; index < historical.size(); ++index)
    require(reduce(result[index]) == historical[index]);
  require(result.size() == 74);
  return result;
}

struct Row {
  unsigned clock = 0, reset = 0, valid = 0;
  Request data;
  unsigned take = 0;
};

std::vector<Row> stimulus() {
  const auto values = vectors();
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, unsigned valid, Request data,
                 unsigned take) {
    rows.push_back({clock, reset, valid, data, take});
  };
  auto edge = [&](Request data, unsigned valid = 1, unsigned take = 1,
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
  add(1, 0, 1, values[4], 0);
  add(1, 0, 1, values[5], 1);
  add(0, 0, 1, values[6], 1);
  add(0, 0, 1, values[7], 1);
  edge(values[4]);
  edge({}, 0, 1);
  edge({}, 0, 1);
  edge(values[5]);
  edge(values[6], 1, 0);
  edge(values[7], 1, 0);
  edge(values[8], 1, 1, 1);
  for (const Request value : values)
    edge(value);
  edge({}, 0, 1);
  edge({}, 0, 1);
  edge(values[0]);
  edge(values[1]);
  edge(values[2], 1, 1, 1);
  require(rows.size() == 187);
  return rows;
}

struct Expected {
  bool ready = true, valid = false;
  std::uint64_t data = 0;
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
          second = reduce(*oldFirst);
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
  std::optional<Request> first;
  std::optional<std::uint64_t> second;
  bool lastClock = false;
};

pyc_dut::Inputs inputs(const Row &row) {
  pyc_dut::Inputs result;
  result.pyc_7079635f636c6b = known<1>(row.clock);
  result.pyc_7079635f727374 = known<1>(row.reset);
  result.valid = known<1>(row.valid);
  result.data = decltype(result.data)::fromPacked(
      known<24>(packRequest(row.data)).packed());
  result.take = known<1>(row.take);
  return result;
}

template <class Output>
void check(const Output &output, Expected expected, unsigned index, bool emit) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  require(packed.value().bit(48) == expected.ready);
  require(packed.value().bit(47) == expected.valid);
  require(gfsim::extract<47>(packed, 0).value() ==
          gfsim::Bits<47>{expected.data});
  if (emit)
    std::cout << "WORK " << index << ' ' << unsigned(expected.ready) << ' '
              << unsigned(expected.valid) << ' ' << binary(expected.data, 47)
              << '\n';
}

struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  std::deque<std::uint64_t> history;
  unsigned sampled = 0, stalled = 0, replacements = 0, resetFull = 0;
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
    const bool actualReady = packed.value().bit(48);
    const bool actualValid = packed.value().bit(47);
    const std::uint64_t actualData =
        gfsim::extract<47>(packed, 0).value().value();
    self.stalled += row.valid && !expected.ready;
    if (row.clock && !self.lastClock) {
      if (row.reset) {
        self.resetFull += self.history.size() == 2;
        self.dropped += self.history.size();
        self.history.clear();
      } else {
        const bool retire = actualValid && row.take;
        const bool accept = actualReady && row.valid;
        if (retire) {
          require(!self.history.empty() && actualData == self.history.front());
          self.history.pop_front();
          ++self.retired;
        }
        if (accept) {
          self.history.push_back(reduce(row.data));
          ++self.accepted;
        }
        self.replacements += retire && accept && self.history.size() == 2;
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
  require(context.stalled >= 3 && context.peak == 2 && context.resetFull == 2);
  require(context.replacements > 20 && context.history.empty());
  require(context.accepted == context.retired + context.dropped);
  std::cout << "HISTORY " << context.accepted << ' ' << context.retired << ' '
            << context.dropped << ' ' << context.history.size() << ' '
            << context.peak << ' ' << context.replacements << '\n';
  return result;
}
