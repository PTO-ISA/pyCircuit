#include "gfsim/SimSystem.h"
#include "parallel_gfsim/SimState.h"

namespace {
class System final : public gfsim::SimSystem {
  bool Precheck() noexcept override { return true; }
};
} // namespace

int main() {
  System system;
  system.Build();
  if (system.state() != gfsim::SimSystemState::Built)
    return 1;

  parallel_gfsim::SimSystem graph;
  parallel_gfsim::SimState<int> counter("counter", 3);
  counter.Build();
  counter.Write(5);
  if (counter.Read() != 3)
    return 1;
  counter.Xfer();
  return counter.Read() == 5 && graph.Cycles() == 0 ? 0 : 1;
}
