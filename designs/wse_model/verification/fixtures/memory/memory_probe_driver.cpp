#include "pycircuit_system.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>

template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  pyc_dut dut(1);
  pyc_dut::Inputs input{};
  input.write = known<1>(0);
  input.address = known<16>(0);
  input.data = known<64>(0);
  input.pyc_7079635f636c6b = known<1>(0);
  input.pyc_7079635f727374 = known<1>(0);
  dut.drive(input);
  dut.system().Build();
  dut.system().Reset();
  auto cycle = [&](bool write, std::uint16_t address, std::uint64_t data) {
    input.write = known<1>(write);
    input.address = known<16>(address);
    input.data = known<64>(data);
    for (unsigned clock : {0, 1}) {
      input.pyc_7079635f636c6b = known<1>(clock);
      dut.drive(input);
      if (dut.system().Step() != gfsim::SimStepResult::Running)
        throw std::runtime_error("native Memory probe failed a checked step");
    }
    auto result = dut.sample().result;
    if (!result.isFullyKnown()) throw std::runtime_error("unknown read data");
    std::uint64_t value = 0;
    for (unsigned bit = 0; bit < 64; ++bit)
      value |= std::uint64_t(result.value().bit(bit)) << bit;
    return value;
  };
  constexpr std::uint64_t value = 0xfedcba9876543210ULL;
  if (cycle(true, 65528, value) != 0 || cycle(false, 65528, 0) != value ||
      cycle(false, 0, 0) != 0)
    throw std::runtime_error("native Memory probe data mismatch");
  std::cout << "PASS native byte_mem 65536 bytes; 64-bit write at 65528; "
               "read-before-write zero; committed readback exact; address zero unchanged\n";
}
