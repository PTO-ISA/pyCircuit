#include MODEL_FILE
#include <cassert>

int main() {
  ac_generated::BlockingStateGuard model;
  auto rows = model.dispatch_rows();
  gfsim::Tick tick = 0;
  auto cycle = [&] {
    const gfsim::Epoch epoch{tick++, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto phase : {gfsim::XferPhase::Arbitrate, gfsim::XferPhase::Probe,
                       gfsim::XferPhase::Commit})
      for (auto &row : rows)
        row.xfer(row.object, epoch, phase);
  };
  auto offer = [&](auto &queue) {
    assert(queue.proposePush(gfsim::UInt<3>{1}));
    queue.doXfer({tick++, 0});
  };
  auto state = [&](unsigned expected) {
    assert(static_cast<unsigned long long>(model.table_count().at(0)) ==
           expected);
    assert(static_cast<unsigned long long>(model.table_mirror().at(0)) ==
           expected);
  };

  // A decrement at one must commit zero, not stall on its proposed value.
  offer(model.up());
  cycle();
  state(1);
  cycle();
  offer(model.down());
  cycle();
  state(0);
  cycle();
  assert(model.sink_1_values().size() == 1);

  for (unsigned count = 1; count <= 4; ++count) {
    offer(model.up());
    cycle();
    state(count);
    cycle();
  }
  assert(model.sink_0_values().size() == 5);
  offer(model.up());
  cycle();
  state(4);
  assert(model.up().committedSize() == 1);

  // Free capacity through the ordinary competing transaction; retry commits
  // both owners and the retained input exactly once.
  offer(model.down());
  cycle();
  state(3);
  cycle();
  state(4);
  cycle();
  assert(model.up().isEmpty());
  assert(model.sink_0_values().size() == 6);
  assert(model.sink_1_values().size() == 2);
}
