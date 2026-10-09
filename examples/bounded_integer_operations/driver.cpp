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
    std::cerr << "bounded_integer_operations oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

constexpr unsigned ChannelCount = 23;
constexpr std::uint32_t AllTakes = (std::uint32_t{1} << ChannelCount) - 1;
constexpr std::array<unsigned, ChannelCount> Widths{
    3, 4, 3, 1, 3, 8, 1, 3, 8, 8, 8, 4,
    1, 1, 1, 3, 8, 8, 8, 8, 8, 8, 8};

bool takeAt(std::uint32_t takes, unsigned channel) {
  return (takes >> (ChannelCount - 1 - channel)) & 1u;
}

std::uint32_t withoutChannel(unsigned channel) {
  return AllTakes ^ (std::uint32_t{1} << (ChannelCount - 1 - channel));
}

struct Request {
  std::array<std::uint8_t, 5> values{};
  std::uint8_t raw = 0;
};

std::uint64_t pack(Request request) {
  std::uint64_t result = 0;
  for (const auto value : request.values)
    result = (result << 8) | value;
  return (result << 8) | request.raw;
}

// Independent reconstruction of the original immutable-array algorithm. The
// two copies are kept explicit so source-after-update and chained snapshots do
// not collapse into formulas derived from the DUT outputs.
std::array<std::uint8_t, ChannelCount> decode(Request request) {
  const unsigned wrapped = request.raw % 5;
  const bool checkedValid = request.raw < 5;
  const unsigned checked = checkedValid ? request.raw : 0;
  const unsigned saturated = std::min(8u, std::max(4u, unsigned(request.raw)));
  auto updated = request.values;
  updated[checked] = request.raw;
  auto chained = updated;
  chained[0] = 99;
  return {
      static_cast<std::uint8_t>(wrapped),
      static_cast<std::uint8_t>(saturated),
      static_cast<std::uint8_t>(checked),
      static_cast<std::uint8_t>(checkedValid),
      static_cast<std::uint8_t>(wrapped + 1),
      request.values[checked],
      static_cast<std::uint8_t>(request.raw % 2),
      static_cast<std::uint8_t>(request.raw % 8),
      request.raw,
      request.values[request.raw & 3u],
      request.values[0],
      static_cast<std::uint8_t>(request.raw >= 4 && request.raw <= 8
                                    ? request.raw
                                    : 4),
      static_cast<std::uint8_t>(request.raw >= 4 && request.raw <= 8),
      1,
      static_cast<std::uint8_t>(saturated > wrapped),
      static_cast<std::uint8_t>((wrapped + 1) - 1),
      updated[checked],
      updated[0],
      updated[4],
      request.values[checked],
      chained[0],
      chained[checked],
      updated[checked],
  };
}

Request varied(std::uint8_t raw) {
  if (raw == 0)
    return {{0, 255, 0, 255, 0}, raw};
  if (raw == 4)
    return {{9, 9, 9, 9, 250}, raw};
  if (raw == 255)
    return {{255, 0, 128, 0, 255}, raw};
  return {{static_cast<std::uint8_t>(raw ^ 0xa5u),
           static_cast<std::uint8_t>(raw * 3u + 17u), raw,
           static_cast<std::uint8_t>(255u - raw),
           static_cast<std::uint8_t>(raw * 5u + 1u)},
          raw};
}

struct Row {
  unsigned clock = 0, reset = 0, valid = 0;
  Request request{};
  std::uint32_t takes = 0;
};

std::vector<Row> stimulus() {
  const Request zero{};
  const Request fixed{{11, 22, 33, 44, 55}, 0};
  constexpr std::array<std::uint8_t, 7> historical{0, 3, 4, 5, 8, 9, 255};
  std::vector<Row> rows;
  auto add = [&](unsigned clock, unsigned reset, unsigned valid, Request request,
                 std::uint32_t takes) {
    rows.push_back({clock, reset, valid, request, takes});
  };
  auto edge = [&](Request request, unsigned valid = 1,
                  std::uint32_t takes = AllTakes, unsigned reset = 0) {
    add(1, reset, valid, request, takes);
    add(0, reset, valid, request, takes);
  };

  add(0, 1, 0, zero, 0);
  edge(zero, 0, 0, 1);

  // Preserve the original seven requests and 96-cycle ready schedule.
  unsigned offered = 0;
  bool sourceOccupied = false, sinkOccupied = false;
  for (unsigned cycle = 0; cycle < 96; ++cycle) {
    Request request = fixed;
    if (offered < historical.size())
      request.raw = historical[offered];
    const bool ready = cycle % 7 != 2 && cycle % 7 != 3;
    const bool room = !sinkOccupied || ready;
    const bool inputReady = !sourceOccupied || room;
    const bool valid = offered < historical.size();
    edge(request, valid, ready ? AllTakes : 0);
    const bool oldSource = sourceOccupied;
    if (oldSource && room)
      sinkOccupied = true;
    else if (sinkOccupied && ready)
      sinkOccupied = false;
    if (valid && inputReady) {
      sourceOccupied = true;
      ++offered;
    } else if (oldSource && room) {
      sourceOccupied = false;
    }
  }
  require(offered == historical.size());
  for (unsigned i = 0; i < 4; ++i)
    edge(zero, 0);

  // Rotate the sole blocked output through all 23 channels. Other outputs
  // drain independently; no output may refill until the blocked channel pops.
  for (unsigned channel = 0; channel < ChannelCount; ++channel) {
    edge(varied(static_cast<std::uint8_t>(32 + channel * 3)));
    edge(varied(static_cast<std::uint8_t>(33 + channel * 3)));
    edge(varied(static_cast<std::uint8_t>(34 + channel * 3)), 1,
         withoutChannel(channel));
    edge(varied(static_cast<std::uint8_t>(35 + channel * 3)), 1,
         withoutChannel(channel));
    edge(varied(static_cast<std::uint8_t>(36 + channel * 3)));
    edge(zero, 0);
    edge(zero, 0);
  }

  // All raw inputs, with payload changes that exercise both endpoints,
  // repeated elements, narrow selection and immutable source snapshots.
  for (unsigned raw = 0; raw < 256; ++raw)
    edge(varied(static_cast<std::uint8_t>(raw)));
  edge(zero, 0);
  edge(zero, 0);

  // Change controls while the clock remains high; only the first row is an
  // edge. The following rows must preserve state until the clock falls/rises.
  add(1, 0, 1, varied(201), 0);
  add(1, 0, 1, varied(202), AllTakes);
  add(1, 1, 0, zero, AllTakes);
  add(0, 0, 0, zero, AllTakes);
  edge(zero, 0);
  edge(zero, 0);

  // Reset once with both stages full, and once with only a subset of outputs
  // occupied after asymmetric draining.
  edge(varied(210));
  edge(varied(211), 1, 0);
  edge(zero, 0, AllTakes, 1);
  edge(varied(212));
  edge(varied(213));
  edge(varied(214), 1, withoutChannel(7));
  edge(zero, 0, AllTakes, 1);
  edge(varied(255));
  edge(varied(0));
  edge(zero, 0);
  edge(zero, 0);
  return rows;
}

template <class Packed>
std::uint64_t extractDynamic(const Packed &packed, unsigned offset,
                             unsigned width) {
  std::uint64_t value = 0;
  for (unsigned bit = 0; bit < width; ++bit)
    value |= std::uint64_t(packed.value().bit(offset + bit)) << bit;
  return value;
}

unsigned dataOffset(unsigned channel) {
  unsigned offset = 0;
  for (unsigned index = channel + 1; index < ChannelCount; ++index)
    offset += Widths[index];
  return offset;
}

struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  std::optional<Request> source;
  std::array<std::optional<std::uint8_t>, ChannelCount> sinks{};
  std::array<std::deque<std::uint8_t>, ChannelCount> histories{};
  std::array<unsigned, ChannelCount> retired{}, dropped{}, isolated{};
  bool lastClock = false;
  unsigned sampled = 0, accepted = 0, stalled = 0, transfers = 0,
           replacements = 0, noFlowThrough = 0, resetFull = 0,
           resetPartial = 0, heldHigh = 0, peak = 0;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }

  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == self.rows.size())
      return false;
    require(epoch < self.rows.size());
    const Row &row = self.rows[epoch];
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(row.clock);
    inputs.pyc_7079635f727374 = known<1>(row.reset);
    inputs.valid = known<1>(row.valid);
    inputs.data = decltype(inputs.data)::fromPacked(known<48>(pack(row.request)).packed());
    inputs.take = decltype(inputs.take)::fromPacked(known<23>(row.takes).packed());
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
    bool allRoom = true;
    for (unsigned channel = 0; channel < ChannelCount; ++channel)
      allRoom &= !sinks[channel] || takeAt(row.takes, channel);
    const bool ready = !source || allRoom;
    const auto packed = dut.sample().result.packed();
    require(packed.isFullyKnown());
    require(packed.value().bit(139) == ready);
    std::cout << "WORK " << sampled << ' ' << ready;
    for (unsigned channel = 0; channel < ChannelCount; ++channel) {
      const bool actualValid = packed.value().bit(138 - channel);
      const std::uint64_t actual =
          extractDynamic(packed, dataOffset(channel), Widths[channel]);
      require(actualValid == sinks[channel].has_value());
      require(actual == sinks[channel].value_or(0));
      std::cout << ' ' << actualValid << ' ' << actual;
    }
    std::cout << '\n';

    const bool rising = row.clock && !lastClock;
    heldHigh += row.clock && lastClock;
    if (rising) {
      unsigned occupiedSinks = 0;
      for (const auto &sink : sinks)
        occupiedSinks += sink.has_value();
      if (row.reset) {
        resetFull += source.has_value() && occupiedSinks == ChannelCount;
        resetPartial += (source.has_value() || occupiedSinks) &&
                        occupiedSinks != ChannelCount;
        source.reset();
        for (unsigned channel = 0; channel < ChannelCount; ++channel) {
          dropped[channel] += histories[channel].size();
          histories[channel].clear();
          sinks[channel].reset();
        }
      } else {
        stalled += row.valid && !ready;
        noFlowThrough += row.valid && ready && !source && occupiedSinks == 0;
        replacements += source.has_value() && allRoom && row.valid && ready;
        const auto oldSource = source;
        const auto decoded = oldSource ? decode(*oldSource)
                                       : std::array<std::uint8_t, ChannelCount>{};
        const auto acceptedValues = decode(row.request);
        for (unsigned channel = 0; channel < ChannelCount; ++channel) {
          const bool pop = sinks[channel] && takeAt(row.takes, channel);
          if (pop) {
            require(!histories[channel].empty());
            require(*sinks[channel] == histories[channel].front());
            histories[channel].pop_front();
            ++retired[channel];
            isolated[channel] += !allRoom;
          }
          if (oldSource && allRoom)
            sinks[channel] = decoded[channel];
          else if (pop)
            sinks[channel].reset();
          if (row.valid && ready)
            histories[channel].push_back(acceptedValues[channel]);
          peak = std::max(peak, unsigned(histories[channel].size()));
          require(histories[channel].size() <= 2);
        }
        transfers += oldSource.has_value() && allRoom;
        if (row.valid && ready) {
          source = row.request;
          ++accepted;
        } else if (oldSource && allRoom) {
          source.reset();
        }
      }
    }
    lastClock = row.clock;
    ++sampled;
  }

  void finish() const {
    require(sampled == rows.size() && sampled < 1600 && !source && peak == 2);
    require(accepted >= 300 && stalled >= ChannelCount &&
            transfers >= accepted - 8 && replacements >= 250 &&
            noFlowThrough >= 3 && resetFull >= 1 && resetPartial >= 1 &&
            heldHigh >= 2);
    for (unsigned channel = 0; channel < ChannelCount; ++channel) {
      require(!sinks[channel] && histories[channel].empty());
      require(isolated[channel] > 0 && dropped[channel] >= 2);
      require(accepted == retired[channel] + dropped[channel]);
      std::cout << "HISTORY " << channel << ' ' << accepted << ' '
                << retired[channel] << ' ' << dropped[channel] << " 0 " << peak
                << '\n';
    }
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
