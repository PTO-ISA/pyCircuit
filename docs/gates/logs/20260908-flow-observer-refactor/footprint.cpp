#include "gfsim/queue_blocks.h"
#include <chrono>
#include <cstdio>
__attribute__((noinline)) unsigned run(gfsim::SimQueue<unsigned> &queue) {
  unsigned sum = 0;
  for (unsigned i = 0; i < 2000000; ++i) {
    queue.proposePush(i);
    queue.doXfer({2ULL * i, 0});
    sum += *queue.proposePop();
    queue.doXfer({2ULL * i + 1, 0});
  }
  return sum;
}
int main() {
  gfsim::SimQueue<unsigned> queue("queue", 0, nullptr, 4);
  const auto start = std::chrono::steady_clock::now();
  const auto sum = run(queue);
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now() - start)
                      .count();
  std::printf("%zu %zu %zu %u %lld\n", sizeof(gfsim::SimObject), sizeof(queue),
              sizeof(gfsim::SimTable<unsigned>), sum, (long long)ns);
}
