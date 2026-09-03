#include "davincioo.generated.cpp"
#include "davincioo_trace_fixture.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Span {
  std::size_t sequence = 0;
  std::string stage;
  std::uint64_t begin = 0;
  std::uint64_t end = 0;
};

class Tracker {
public:
  explicit Tracker(std::size_t count)
      : current_(count, "source_wait"), begin_(count, 0), seen_(count, false),
        completionSeen_(count, false), retirementSeen_(count, false) {}

  void transition(std::size_t sequence, std::string_view stage,
                  std::uint64_t cycle) {
    if (sequence >= current_.size() || current_[sequence] == stage)
      return;
    close(sequence, cycle);
    current_[sequence] = stage;
    begin_[sequence] = cycle;
    seen_[sequence] = true;
  }

  void infer(std::size_t sequence, std::uint64_t cycle) {
    if (!seen_[sequence])
      return;
    const std::string &stage = current_[sequence];
    if (stage == "completed" || stage == "rob_wait")
      transition(sequence, "rob_wait", cycle);
    else if (stage == "dispatched" || stage == "schedule_execute")
      transition(sequence, "schedule_execute", cycle);
  }

  void completion(std::size_t sequence) {
    if (sequence < completionSeen_.size() && !completionSeen_[sequence]) {
      completionSeen_[sequence] = true;
      completionOrder_.push_back(sequence);
    }
  }

  void retirement(std::size_t sequence) {
    if (sequence < retirementSeen_.size() && !retirementSeen_[sequence]) {
      retirementSeen_[sequence] = true;
      retirementOrder_.push_back(sequence);
    }
  }

  bool isRetired(std::size_t sequence) const {
    return sequence < retirementSeen_.size() && retirementSeen_[sequence];
  }

  void finish(std::uint64_t cycle) {
    for (std::size_t sequence = 0; sequence < current_.size(); ++sequence)
      close(sequence, cycle);
  }

  const std::vector<Span> &spans() const { return spans_; }
  const std::vector<std::size_t> &completionOrder() const {
    return completionOrder_;
  }
  const std::vector<std::size_t> &retirementOrder() const {
    return retirementOrder_;
  }

private:
  void close(std::size_t sequence, std::uint64_t cycle) {
    if (current_[sequence] != "done" && cycle > begin_[sequence])
      spans_.push_back({sequence, current_[sequence], begin_[sequence], cycle});
  }

  std::vector<std::string> current_;
  std::vector<std::uint64_t> begin_;
  std::vector<bool> seen_;
  std::vector<bool> completionSeen_;
  std::vector<bool> retirementSeen_;
  std::vector<std::size_t> completionOrder_;
  std::vector<std::size_t> retirementOrder_;
  std::vector<Span> spans_;
};

template <typename Queue>
void observeQueue(const Queue &queue, std::string_view stage,
                  std::vector<std::optional<std::string>> &observed,
                  Tracker *tracker = nullptr, bool completion = false,
                  bool retirement = false) {
  for (const auto &item : queue.committedValues()) {
    const std::size_t sequence = item.sequence;
    if (sequence >= observed.size())
      continue;
    observed[sequence] = std::string(stage);
    if (tracker && completion)
      tracker->completion(sequence);
    if (tracker && retirement)
      tracker->retirement(sequence);
  }
}

void printOrder(std::string_view name, const std::vector<std::size_t> &order) {
  std::cout << name;
  for (std::size_t sequence : order)
    std::cout << ' ' << sequence;
  std::cout << '\n';
}

} // namespace

int main() {
  ac_generated::Davincioo model;
  auto rows = model.dispatch_rows();
  Tracker tracker(davincioo_fixture::kTokens.size());
  std::size_t nextInput = 0;
  std::uint64_t cycles = 0;

  for (std::uint64_t tick = 0; tick < 4096; ++tick) {
    while (nextInput < davincioo_fixture::kTokens.size() &&
           model.incoming().canProposePush()) {
      if (!model.incoming().proposePush(davincioo_fixture::kTokens[nextInput]))
        break;
      ++nextInput;
    }

    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);

    std::vector<std::optional<std::string>> observed(
        davincioo_fixture::kTokens.size());
    observeQueue(model.incoming(), "incoming", observed);
    observeQueue(model.decoded(), "decoded", observed);
    observeQueue(model.pipelined(), "pipelined", observed);
    observeQueue(model.scalar(), "dispatch_scalar", observed);
    observeQueue(model.vector(), "dispatch_vector", observed);
    observeQueue(model.cube(), "dispatch_cube", observed);
    observeQueue(model.tma(), "dispatch_tma", observed);
    observeQueue(model.dispatched(), "dispatched", observed);
    observeQueue(model.completed(), "completed", observed, &tracker, true);
    for (const auto &item : model.retired().committedValues()) {
      if (item.sequence >= observed.size())
        continue;
      const bool alreadyRetired = tracker.isRetired(item.sequence);
      tracker.retirement(item.sequence);
      observed[item.sequence] = alreadyRetired ? "done" : "retired";
    }
    for (const auto &item : model.sink_0_values()) {
      if (item.sequence < observed.size()) {
        const bool alreadyRetired = tracker.isRetired(item.sequence);
        tracker.retirement(item.sequence);
        observed[item.sequence] = alreadyRetired ? "done" : "retired";
      }
    }

    for (std::size_t sequence = 0; sequence < observed.size(); ++sequence) {
      if (observed[sequence])
        tracker.transition(sequence, *observed[sequence], tick);
      else
        tracker.infer(sequence, tick);
    }

    if (model.sink_0_values().size() == davincioo_fixture::kTokens.size()) {
      cycles = tick + 1;
      break;
    }
  }

  if (cycles == 0)
    return 2;
  tracker.finish(cycles);

  std::cout << "SUMMARY " << cycles << ' ' << model.sink_0_values().size()
            << '\n';
  printOrder("COMPLETION", tracker.completionOrder());
  printOrder("RETIREMENT", tracker.retirementOrder());
  std::cout << "VALUES";
  for (const auto &item : model.sink_0_values())
    std::cout << ' ' << item.value;
  std::cout << '\n';
  for (const Span &span : tracker.spans())
    std::cout << "SPAN " << span.sequence << ' '
              << davincioo_fixture::kOpcodes[span.sequence] << ' ' << span.stage
              << ' ' << span.begin << ' ' << span.end << '\n';
  return 0;
}
