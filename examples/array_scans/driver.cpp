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
#include <vector>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "array_scans oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

struct Request {
  std::uint8_t first = 0, second = 0, third = 0;
  bool operator==(const Request &) const = default;
};

std::uint32_t packRequest(Request request) {
  return (std::uint32_t{request.first} << 16) |
         (std::uint32_t{request.second} << 8) | request.third;
}

struct Pair {
  std::uint8_t first = 0, second = 0;
};

struct Prefixes {
  std::array<std::uint8_t, 3> subtract{};
  std::array<std::uint8_t, 3> add{};
  std::array<std::uint8_t, 3> reset{};
  std::array<Pair, 3> pairs{};
};

// Independent imperative reconstruction of every ordered prefix. Arithmetic
// narrows after each step, and Pair.second is copied through each snapshot.
Prefixes scan(Request request) {
  const std::array<std::uint8_t, 3> values{request.first, request.second,
                                           request.third};
  Prefixes result;
  std::uint8_t subtractAccumulator = 0;
  std::uint8_t addAccumulator = 10;
  std::uint8_t resetAccumulator = 5;
  Pair pairAccumulator{request.first, request.first};
  for (unsigned index = 0; index < values.size(); ++index) {
    subtractAccumulator =
        static_cast<std::uint8_t>(subtractAccumulator - values[index]);
    addAccumulator = static_cast<std::uint8_t>(addAccumulator + values[index]);
    resetAccumulator = values[index] == 0
                           ? 0
                           : static_cast<std::uint8_t>(resetAccumulator +
                                                       values[index]);
    pairAccumulator =
        Pair{values[index] == 0
                 ? std::uint8_t{0}
                 : static_cast<std::uint8_t>(pairAccumulator.first +
                                             values[index]),
             pairAccumulator.second};
    result.subtract[index] = subtractAccumulator;
    result.add[index] = addAccumulator;
    result.reset[index] = resetAccumulator;
    result.pairs[index] = pairAccumulator;
    require(result.pairs[index].second == request.first);
  }
  return result;
}

std::array<std::uint8_t, 8> outputFields(Request request) {
  const auto prefixes = scan(request);
  return {prefixes.subtract[0], prefixes.subtract[1], prefixes.subtract[2],
          prefixes.add[2],      prefixes.reset[1],    prefixes.reset[2],
          prefixes.pairs[0].first, prefixes.pairs[2].first};
}

std::uint64_t expected(Request request) {
  std::uint64_t packed = 0;
  for (const auto value : outputFields(request))
    packed = (packed << 8) | value;
  return packed;
}

std::vector<Request> coverageVectors() {
  constexpr std::array<std::uint8_t, 7> boundaries{0, 1, 2, 127,
                                                    128, 254, 255};
  std::vector<Request> result;
  result.reserve(485);
  for (const auto first : boundaries)
    for (const auto second : boundaries)
      for (const auto third : boundaries)
        result.push_back({first, second, third});
  result.insert(result.end(),
                {{0, 17, 33}, {17, 0, 33}, {17, 33, 0},
                 {0, 255, 1}, {255, 0, 1}, {255, 1, 0},
                 {3, 17, 241}, {241, 17, 3}, {1, 128, 255},
                 {255, 128, 1}, {254, 255, 1}, {255, 255, 255},
                 {127, 128, 129}, {128, 127, 126}});
  for (unsigned index = 0; index < 128; ++index)
    result.push_back(
        {static_cast<std::uint8_t>(index * 73u + 19u),
         static_cast<std::uint8_t>(index * 151u + 7u),
         static_cast<std::uint8_t>(index * 199u + 251u)});
  require(result.size() == 485);
  return result;
}

struct Row {
  unsigned clock = 0, reset = 0, valid = 0, take = 0;
  Request request{};
};

std::vector<Row> stimulus() {
  constexpr std::array<Request, 4> historical{
      Request{0, 0, 0}, Request{1, 2, 3}, Request{255, 1, 2},
      Request{7, 9, 11}};
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, unsigned valid,
                 Request request, unsigned take) {
    rows.push_back({clock, reset, valid, take, request});
  };
  auto edge = [&](Request request, unsigned valid = 1, unsigned take = 1,
                  unsigned reset = 0) {
    add(1, reset, valid, request, take);
    add(0, reset, valid, request, take);
  };
  const Request zero{};
  add(0, 1, 0, zero, 0);
  edge(zero, 0, 0, 1);

  // Preserve the original bounded one-request/one-result history.
  for (const auto request : historical) {
    edge(request);
    edge(zero, 0);
    edge(zero, 0);
  }

  // Fill both stages, prove a blocked request remains stable, hold the physical
  // clock high while controls change, then reset the full pipeline.
  edge({31, 47, 63}, 1, 0);
  edge({32, 48, 64}, 1, 0);
  edge({33, 49, 65}, 1, 0);
  add(1, 0, 1, {34, 50, 66}, 0);
  add(1, 0, 1, {35, 51, 67}, 1);
  add(1, 1, 0, zero, 1);
  add(0, 0, 0, zero, 1);
  edge(zero, 0, 1, 1);

  // Reset with only the request stage occupied.
  edge({36, 52, 68});
  edge(zero, 0, 1, 1);

  // Full output pop, internal transfer and input replacement in one edge.
  edge({37, 53, 69});
  edge({38, 54, 70}, 1, 0);
  edge({39, 55, 71});
  edge(zero, 0);
  edge(zero, 0);

  for (const auto request : coverageVectors())
    edge(request);
  for (unsigned cycle = 0; cycle < 4; ++cycle)
    edge(zero, 0);
  require(rows.size() < 1100);
  return rows;
}

struct Golden {
  std::optional<Request> first;
  std::optional<std::uint64_t> second;
  bool lastClock = false;

  struct Output {
    bool ready = true, valid = false;
    std::uint64_t data = 0;
  };

  Output output(bool take) const {
    const bool room = !second || take;
    return {!first || room, second.has_value(), second.value_or(0)};
  }

  void commit(const Row &row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        first.reset();
        second.reset();
      } else {
        const auto oldFirst = first;
        const bool popSecond = second && row.take;
        const bool room = !second || popSecond;
        const bool popFirst = first && room;
        const bool inputRoom = !first || popFirst;
        if (oldFirst && room)
          second = expected(*oldFirst);
        else if (popSecond)
          second.reset();
        if (row.valid && inputRoom)
          first = row.request;
        else if (popFirst)
          first.reset();
      }
    }
    lastClock = row.clock;
  }
};

template <class Packed>
std::uint64_t extract(const Packed &packed, unsigned low, unsigned width) {
  std::uint64_t result = 0;
  for (unsigned bit = 0; bit < width; ++bit)
    result |= std::uint64_t(packed.value().bit(low + bit)) << bit;
  return result;
}

struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  std::deque<std::uint64_t> history;
  unsigned sampled = 0, accepted = 0, retired = 0, dropped = 0,
           stalled = 0, replacements = 0, resetFull = 0,
           resetPartial = 0, heldHigh = 0, peak = 0;
  bool lastClock = false;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }

  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == self.rows.size())
      return false;
    const Row &row = self.rows[epoch];
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(row.clock);
    inputs.pyc_7079635f727374 = known<1>(row.reset);
    inputs.valid = known<1>(row.valid);
    inputs.data = decltype(inputs.data)::fromPacked(
        known<24>(packRequest(row.request)).packed());
    inputs.take = known<1>(row.take);
    self.dut.drive(inputs);
    return true;
  }

  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    self.observe();
  }

  void observe() {
    const Row &row = rows[sampled];
    const auto reference = golden.output(row.take);
    const auto packed = dut.sample().result.packed();
    require(packed.isFullyKnown());
    require(packed.value().bit(65) == reference.ready);
    require(packed.value().bit(64) == reference.valid);
    require(extract(packed, 0, 64) == reference.data);
    for (unsigned field = 0; field < 8; ++field)
      require(extract(packed, (7 - field) * 8, 8) ==
              ((reference.data >> ((7 - field) * 8)) & 0xff));
    std::cout << "WORK " << sampled << ' ' << reference.ready << ' '
              << reference.valid << ' ' << reference.data << '\n';

    heldHigh += row.clock && lastClock;
    if (row.clock && !lastClock) {
      if (row.reset) {
        resetFull += history.size() == 2;
        resetPartial += history.size() == 1;
        dropped += history.size();
        history.clear();
      } else {
        stalled += row.valid && !reference.ready;
        const bool wasFull = history.size() == 2;
        if (reference.valid && row.take) {
          require(!history.empty() && reference.data == history.front());
          history.pop_front();
          ++retired;
        }
        if (reference.ready && row.valid) {
          history.push_back(expected(row.request));
          ++accepted;
          replacements += wasFull;
        }
      }
      peak = std::max(peak, unsigned(history.size()));
      require(history.size() <= 2);
    }
    lastClock = row.clock;
    golden.commit(row);
    ++sampled;
  }

  void finish() const {
    constexpr std::array<Request, 4> original{
        Request{0, 0, 0}, Request{1, 2, 3}, Request{255, 1, 2},
        Request{7, 9, 11}};
    constexpr std::array<std::uint64_t, 4> goldenValues{
        42949672960ULL, 18446174595540779527ULL, 72336921615400449ULL,
        18010146857285586466ULL};
    for (unsigned index = 0; index < original.size(); ++index)
      require(expected(original[index]) == goldenValues[index]);
    require(sampled == rows.size() && sampled < 1100 && history.empty());
    require(accepted >= 495 && accepted == retired + dropped && peak == 2 &&
            stalled >= 2 && replacements >= 480 && resetFull >= 1 &&
            resetPartial >= 1 && heldHigh >= 2);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << " 0 " << peak << ' ' << replacements << '\n';
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
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  context.finish();
  return status;
}
