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
    std::cerr << "recursive_array_updates oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

struct Request {
  std::uint8_t raw = 0, replacement = 0;
};

struct Inner {
  std::uint8_t tag = 0;
  bool mode = false;
  std::uint8_t ordinal = 0;
};

constexpr std::array<unsigned, 14> Widths{8, 8, 1, 1, 1, 1, 2,
                                          2, 3, 3, 3, 3, 3, 3};

std::array<std::uint8_t, 14> expectedFields(Request request) {
  const unsigned structIndex = request.raw % 3;
  const unsigned enumIndex = request.raw % 5;
  const unsigned wideIndex = request.raw % 65;
  const std::uint8_t sourceRange = request.raw % 3;
  const std::uint8_t replacementRange = request.replacement % 3;
  const std::uint8_t sourceWide = request.raw & 7u;
  const std::uint8_t replacementWide = request.replacement & 7u;

  std::array<Inner, 3> structs{};
  std::array<bool, 5> enums{};
  std::array<std::uint8_t, 3> ranges{};
  std::array<std::uint8_t, 65> wide{};
  for (auto &item : structs)
    item = {request.raw, false, sourceRange};
  enums.fill(false);
  ranges.fill(sourceRange);
  wide.fill(sourceWide);

  auto updatedStructs = structs;
  auto updatedEnums = enums;
  auto updatedRanges = ranges;
  auto updatedWide = wide;
  updatedStructs[structIndex] =
      {request.replacement, true, replacementRange};
  updatedEnums[enumIndex] = true;
  updatedRanges[structIndex] = replacementRange;
  updatedWide[wideIndex] = replacementWide;
  auto chainedWide = updatedWide;
  chainedWide[0] = 7;

  return {structs[structIndex].tag,
          updatedStructs[structIndex].tag,
          static_cast<std::uint8_t>(structs[structIndex].mode),
          static_cast<std::uint8_t>(updatedStructs[structIndex].mode),
          static_cast<std::uint8_t>(enums[enumIndex]),
          static_cast<std::uint8_t>(updatedEnums[enumIndex]),
          ranges[structIndex],
          updatedRanges[structIndex],
          wide[wideIndex],
          updatedWide[wideIndex],
          updatedWide[0],
          updatedWide[64],
          chainedWide[wideIndex],
          updatedWide[wideIndex]};
}

std::uint64_t expectedPacked(Request request) {
  const auto fields = expectedFields(request);
  std::uint64_t packed = 0;
  for (unsigned index = 0; index < fields.size(); ++index)
    packed = (packed << Widths[index]) |
             (fields[index] & ((std::uint64_t{1} << Widths[index]) - 1));
  return packed;
}

struct Row {
  unsigned clock = 0, reset = 0, valid = 0, take = 0;
  Request request{};
};

std::vector<Row> stimulus() {
  constexpr std::array<Request, 4> historical{
      Request{0, 9}, Request{2, 14}, Request{64, 5}, Request{255, 131}};
  constexpr std::array<std::uint8_t, 8> replacements{0, 1, 2, 7,
                                                     8, 127, 128, 255};
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

  // Original behavior: offer one request until accepted, then wait for its
  // result and retirement before presenting the next request.
  for (const auto request : historical) {
    edge(request);
    edge(zero, 0);
    edge(zero, 0);
  }

  // Fill both stages, stall, vary controls while clock-high, then reset full.
  edge({17, 33}, 1, 0);
  edge({18, 34}, 1, 0);
  edge({19, 35}, 1, 0);
  add(1, 0, 1, {20, 36}, 0);
  add(1, 0, 1, {21, 37}, 1);
  add(1, 1, 0, zero, 1);
  add(0, 0, 0, zero, 1);
  edge(zero, 0, 1, 1);

  // Reset with only the input stage occupied.
  edge({22, 38});
  edge(zero, 0, 1, 1);

  // Simultaneous full output pop, internal transfer and input replacement.
  edge({23, 39});
  edge({24, 40}, 1, 0);
  edge({25, 41});
  edge(zero, 0);
  edge(zero, 0);

  // Complete raw/replacement product and the separate identity-update matrix.
  for (unsigned raw = 0; raw < 256; ++raw)
    for (const auto replacement : replacements)
      edge({static_cast<std::uint8_t>(raw), replacement});
  for (unsigned raw = 0; raw < 256; ++raw)
    edge({static_cast<std::uint8_t>(raw), static_cast<std::uint8_t>(raw)});
  for (unsigned cycle = 0; cycle < 4; ++cycle)
    edge(zero, 0);
  return rows;
}

template <class Packed>
std::uint64_t extractDynamic(const Packed &packed, unsigned offset,
                             unsigned width) {
  std::uint64_t result = 0;
  for (unsigned bit = 0; bit < width; ++bit)
    result |= std::uint64_t(packed.value().bit(offset + bit)) << bit;
  return result;
}

unsigned fieldOffset(unsigned field) {
  unsigned offset = 0;
  for (unsigned index = field + 1; index < Widths.size(); ++index)
    offset += Widths[index];
  return offset;
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
          second = expectedPacked(*oldFirst);
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
        known<16>((std::uint16_t{row.request.raw} << 8) |
                  row.request.replacement)
            .packed());
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
    const auto expected = golden.output(row.take);
    const auto packed = dut.sample().result.packed();
    require(packed.isFullyKnown());
    require(packed.value().bit(43) == expected.ready);
    require(packed.value().bit(42) == expected.valid);
    const auto actualData = extractDynamic(packed, 0, 42);
    require(actualData == expected.data);
    std::uint64_t remaining = expected.data;
    for (unsigned field = 0; field < Widths.size(); ++field) {
      const auto mask = (std::uint64_t{1} << Widths[field]) - 1;
      const auto expectedField = (remaining >> fieldOffset(field)) & mask;
      require(extractDynamic(packed, fieldOffset(field), Widths[field]) ==
              expectedField);
    }
    std::cout << "WORK " << sampled << ' ' << expected.ready << ' '
              << expected.valid << ' ' << expected.data << '\n';

    heldHigh += row.clock && lastClock;
    if (row.clock && !lastClock) {
      if (row.reset) {
        resetFull += history.size() == 2;
        resetPartial += history.size() == 1;
        dropped += history.size();
        history.clear();
      } else {
        stalled += row.valid && !expected.ready;
        const bool wasFull = history.size() == 2;
        if (expected.valid && row.take) {
          require(!history.empty() && expected.data == history.front());
          history.pop_front();
          ++retired;
        }
        if (expected.ready && row.valid) {
          history.push_back(expectedPacked(row.request));
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
        Request{0, 9}, Request{2, 14}, Request{64, 5}, Request{255, 131}};
    constexpr std::array<std::uint64_t, 4> goldenValues{
        624955961ULL, 35322946742ULL, 1099869737325ULL, 4389679644635ULL};
    for (unsigned index = 0; index < original.size(); ++index)
      require(expectedPacked(original[index]) == goldenValues[index]);
    require(sampled == rows.size() && sampled < 5000 && history.empty());
    require(accepted >= 2310 && accepted == retired + dropped && peak == 2 &&
            stalled >= 2 && replacements >= 2000 && resetFull >= 1 &&
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
