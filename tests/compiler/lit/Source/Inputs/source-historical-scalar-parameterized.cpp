#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <type_traits>

using Word = gfsim::wire<gfsim::Bits<17>>;
static_assert(std::is_same_v<decltype(pyc_dut::Inputs::value), Word>);
static_assert(std::is_same_v<decltype(pyc_dut::Outputs::result), Word>);

void require(bool result) {
  if (!result)
    std::abort();
}

int main(int argc, char **argv) {
  require(argc == 2);
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
  pyc_dut dut(workers);
  dut.drive({Word::known(gfsim::Bits<17>{0})});
  dut.system().Build();
  dut.system().Reset();
  auto check = [&](Word input) {
    dut.drive({input});
    require(dut.system().Step() == gfsim::SimStepResult::Running);
    const auto actual = dut.sample().result.packed();
    const auto wanted = input.packed();
    require(actual.invariantHolds());
    require(actual.value() == wanted.value());
    require(actual.knownMask() == wanted.knownMask());
    require(actual.zMask() == wanted.zMask());
    return actual;
  };
  for (unsigned value :
       std::array<unsigned, 8>{0, 1, 65535, 65536, 131071, 87381, 43690, 0}) {
    const auto actual = check(Word::known(gfsim::Bits<17>{value}));
    std::cout << "KNOWN " << actual.value().value() << '\n';
  }
  // Additional host four-state evidence, separate from the historical
  // width-17 generation/type checks and the source system's known stimuli.
  for (Word input :
       std::array<Word, 3>{Word::unknown(),
                           Word::fromPacked(gfsim::FourState<17>::fromMasks(
                               gfsim::Bits<17>{0}, gfsim::Bits<17>{0},
                               gfsim::Bits<17>{131071})),
                           Word::fromPacked(gfsim::FourState<17>::fromMasks(
                               gfsim::Bits<17>{65536}, gfsim::Bits<17>{65537},
                               gfsim::Bits<17>{43690}))}) {
    const auto actual = check(input);
    std::cout << "PLANES " << actual.value().value() << ' '
              << actual.knownMask().value() << ' ' << actual.zMask().value()
              << '\n';
  }
}
