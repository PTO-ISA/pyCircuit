#include "gfsim/priority_encode.h"
#include <chrono>
#include <cstdint>
#include <iostream>
int main() {
  constexpr std::uint64_t iterations = 10000000;
  std::uint64_t sum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (std::uint64_t i = 0; i < iterations; ++i) {
    const unsigned bit = static_cast<unsigned>((i * 17) & 63);
    const std::uint64_t mask = std::uint64_t{1} << bit;
    const auto result = gfsim::priorityEncode(gfsim::UInt<64>{mask}, true);
    sum += result.index.value() + result.valid.value();
  }
  const auto end = std::chrono::steady_clock::now();
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cout << "iterations=" << iterations << " ns=" << ns
            << " ns_per=" << static_cast<double>(ns) / iterations
            << " checksum=" << sum << "\n";
}
