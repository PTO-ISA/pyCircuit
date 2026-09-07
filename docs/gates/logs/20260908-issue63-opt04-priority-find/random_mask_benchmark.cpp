#include "gfsim/priority_encode.h"
#include <chrono>
#include <cstdint>
#include <iostream>
int main() {
  constexpr std::uint64_t iterations = 10000000;
  std::uint64_t state = 0x9e3779b97f4a7c15ULL;
  std::uint64_t sum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (std::uint64_t i = 0; i < iterations; ++i) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    const auto result = gfsim::priorityEncode(gfsim::UInt<64>{state}, true);
    sum += result.index.value() + result.valid.value();
  }
  const auto end = std::chrono::steady_clock::now();
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cout << "iterations=" << iterations << " ns=" << ns
            << " ns_per=" << static_cast<double>(ns) / iterations
            << " checksum=" << sum << "\n";
}
