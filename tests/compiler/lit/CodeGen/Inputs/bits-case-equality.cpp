#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "case equality oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned W>
gfsim::wire<gfsim::Bits<W>> input(std::string_view symbols, bool noise) {
  require(symbols.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    const char symbol = symbols[W - 1 - bit];
    const auto word = bit / 64;
    const auto mask = std::uint64_t{1} << (bit % 64);
    if (symbol == '1' || ((symbol == 'x' || symbol == 'z') &&
                          ((bit & 1u) != static_cast<unsigned>(noise))))
      value.setWord(word, value.word(word) | mask);
    if (symbol == '0' || symbol == '1')
      known.setWord(word, known.word(word) | mask);
    if (symbol == 'z')
      z.setWord(word, z.word(word) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}

template <class Wire> char observed(const Wire &wire) {
  const auto packed = wire.packed();
  require(packed.zMask() == gfsim::Bits<1>{0});
  if (packed.knownMask() == gfsim::Bits<1>{0})
    return 'x';
  require(packed.knownMask() == gfsim::Bits<1>{1});
  return packed.value() == gfsim::Bits<1>{1} ? '1' : '0';
}

char ordinaryEquality(std::string_view lhs, std::string_view rhs) {
  require(lhs.size() == rhs.size());
  bool hasUnknown = false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const bool lhsKnown = lhs[i] == '0' || lhs[i] == '1';
    const bool rhsKnown = rhs[i] == '0' || rhs[i] == '1';
    if (lhsKnown && rhsKnown && lhs[i] != rhs[i])
      return '0';
    hasUnknown |= !lhsKnown || !rhsKnown;
  }
  return hasUnknown ? 'x' : '1';
}

struct WideRow {
  std::string lhs;
  std::string rhs;
  bool lhsNoise;
  bool rhsNoise;
};

std::vector<WideRow> wideRows() {
  const auto zeros = [](unsigned count) { return std::string(count, '0'); };
  return {{"1" + zeros(64), "1" + zeros(64), false, true},
          {"1" + zeros(64), "0" + zeros(64), false, true},
          {"x" + zeros(63) + "z", "x" + zeros(63) + "z", false, true},
          {"x" + zeros(63) + "z", "z" + zeros(63) + "z", false, true},
          {"x" + zeros(63) + "0", "x" + zeros(63) + "1", false, true}};
}

struct Context {
  pyc_dut &dut;
  std::vector<WideRow> wide = wideRows();
  unsigned sampled = 0;
  static constexpr std::array symbols{'0', '1', 'x', 'z'};

  static void initialize(void *opaque) { require(drive(opaque, 0)); }

  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch >= 16 + self.wide.size())
      return false;
    pyc_dut::Inputs ports;
    if (epoch < 16) {
      const char lhs = symbols[epoch / 4];
      const char rhs = symbols[epoch % 4];
      ports.lhs1 = input<1>(std::string(1, lhs), false);
      ports.rhs1 = input<1>(std::string(1, rhs), true);
      ports.lhs65 = input<65>(std::string(65, '0'), false);
      ports.rhs65 = input<65>(std::string(65, '0'), true);
    } else {
      const auto &row = self.wide[epoch - 16];
      ports.lhs1 = input<1>("0", false);
      ports.rhs1 = input<1>("0", true);
      ports.lhs65 = input<65>(row.lhs, row.lhsNoise);
      ports.rhs65 = input<65>(row.rhs, row.rhsNoise);
    }
    self.dut.drive(ports);
    return true;
  }

  static void sample(void *opaque, std::uint64_t committedEpoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(committedEpoch == self.sampled + 1);
    const auto outputs = self.dut.sample();
    if (self.sampled < 16) {
      const char lhs = symbols[self.sampled / 4];
      const char rhs = symbols[self.sampled % 4];
      const char caseResult = observed(outputs.case1);
      const char eqResult = observed(outputs.eq1);
      require(caseResult == (lhs == rhs ? '1' : '0'));
      require(eqResult ==
              ordinaryEquality(std::string(1, lhs), std::string(1, rhs)));
      std::cout << "ONE " << self.sampled << ' ' << caseResult << ' '
                << eqResult << '\n';
    } else {
      const auto &row = self.wide[self.sampled - 16];
      const char caseResult = observed(outputs.case65);
      const char eqResult = observed(outputs.eq65);
      require(caseResult == (row.lhs == row.rhs ? '1' : '0'));
      require(eqResult == ordinaryEquality(row.lhs, row.rhs));
      std::cout << "WIDE " << self.sampled - 16 << ' ' << caseResult << ' '
                << eqResult << '\n';
    }
    ++self.sampled;
  }
};

} // namespace

int main(int argc, char **argv) {
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
  gfsim::SystemRunner runner(argc, argv, config);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == 16 + context.wide.size());
  return status;
}
