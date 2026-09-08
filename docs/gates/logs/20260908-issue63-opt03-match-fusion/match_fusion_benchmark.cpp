#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

struct Entry {
  std::uint64_t key;
  bool valid;
  bool completed;
};

struct Masks {
  std::uint64_t target;
  std::uint64_t completed;
};

[[gnu::noinline]] Masks separate(const std::array<Entry, 8> &entries,
                                 std::uint64_t key) {
  Masks masks{};
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto &entry = entries[index];
    if (entry.valid && !entry.completed && entry.key == key)
      masks.target |= std::uint64_t{1} << index;
  }
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto &entry = entries[index];
    if (entry.valid && entry.completed && entry.key == key)
      masks.completed |= std::uint64_t{1} << index;
  }
  return masks;
}

[[gnu::noinline]] Masks fused(const std::array<Entry, 8> &entries,
                              std::uint64_t key) {
  Masks masks{};
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto &entry = entries[index];
    const bool common = entry.valid && entry.key == key;
    if (common && !entry.completed)
      masks.target |= std::uint64_t{1} << index;
    if (common && entry.completed)
      masks.completed |= std::uint64_t{1} << index;
  }
  return masks;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  const bool useFused = std::string_view(argv[1]) == "fused";
  std::array<Entry, 8> entries{{
      {3, true, false}, {7, true, true},  {3, true, true},
      {5, false, false}, {9, true, false}, {3, true, false},
      {7, true, false}, {3, false, true},
  }};
  constexpr std::uint64_t iterations = 20'000'000;
  std::uint64_t checksum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (std::uint64_t iteration = 0; iteration < iterations; ++iteration) {
    const std::uint64_t key = (iteration & 3) == 0 ? 3 : 7;
    const Masks masks = useFused ? fused(entries, key) : separate(entries, key);
    checksum += masks.target + 3 * masks.completed;
  }
  const auto elapsed = std::chrono::duration<double, std::nano>(
                           std::chrono::steady_clock::now() - start)
                           .count();
  std::cout << (elapsed / static_cast<double>(iterations)) << ' ' << checksum
            << '\n';
}
