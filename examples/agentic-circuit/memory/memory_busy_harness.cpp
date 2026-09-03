#include "memory_busy.generated.cpp"

#include <cstddef>
#include <iomanip>
#include <iostream>

namespace {

void step(ac_generated::MemoryBusy &model, std::size_t tick) {
  const gfsim::Epoch epoch{tick, 0};
  auto rows = model.dispatch_rows();
  for (auto &row : rows)
    row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
}

} // namespace

int main() {
  ac_generated::MemoryBusy model;
  const ac_generated::ReadRequest first{3, 0, 1};
  const ac_generated::ReadRequest second{7, 0, 2};

  std::cout << "epoch  req_q  received  event\n";
  auto record = [&](std::size_t tick, const char *event) {
    step(model, tick);
    std::cout << std::setw(5) << tick << "  " << std::setw(5)
              << model.requests().committedSize() << "  " << std::setw(8)
              << model.sink_0_values().size() << "  " << event << "\n";
  };

  // Epoch 0 publishes the first request into the source Queue.
  if (!model.requests().proposePush(first))
    return 1;
  record(0, "request A queued");
  if (model.requests().committedSize() != 1)
    return 2;

  // Offer the second request at a later simulated time. Epoch 1 accepts the
  // first request and commits the second, leaving exactly one queued request.
  if (!model.requests().proposePush(second))
    return 3;
  record(1, "A accepted; request B queued");
  if (model.requests().committedSize() != 1)
    return 4;

  // With instance latency 3, B stays queued throughout epochs 2 and 3. A's
  // response matures in epoch 4, and the no-same-epoch rule still keeps B.
  record(2, "B blocked by memory latency");
  record(3, "B blocked by memory latency");
  record(4, "A response accepted; busy released");
  if (model.requests().committedSize() != 1)
    return 5;

  // B can fire only in the epoch after A releases busy.
  record(5, "B accepted");
  if (model.requests().committedSize() != 0)
    return 6;

  record(6, "B response pending");
  record(7, "B response pending");
  record(8, "B response accepted; busy released");
  record(9, "sink received B");
  const auto &responses = model.sink_0_values();
  if (responses.size() != 2 || responses[0].tag != 1 || responses[1].tag != 2 ||
      responses[0].data != 0 || responses[1].data != 0)
    return 7;

  std::cout << "latency_blocked=1 accepted_after_release=1 responses="
            << responses.size() << "\n";
  return 0;
}
