#ifndef GFSIM_QUEUE_H
#define GFSIM_QUEUE_H

#include "gfsim/core.h"
#include "gfsim/object.h"
#include "gfsim/packet.h"

#include <cstddef>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gfsim {

// ── SimQueue<T> ───────────────────────────────────────────────────────

/// FIFO data queue with entry capacity, optional byte capacity,
/// ordered read/write proposals, deterministic arbitration,
/// and occupancy/watermark statistics.
template <typename T> class SimQueue : public SimObject {
public:
  SimQueue(std::string name, ObjectId id, SimObject *parent,
           size_t entryCapacity, size_t byteCapacity = SIZE_MAX,
           ObservationSink *observations = nullptr, size_t latency = 1,
           size_t rate = SIZE_MAX)
      : SimObject(ObjectKind::Queue, std::move(name), id, parent, observations),
        entryCapacity_(entryCapacity), byteCapacity_(byteCapacity),
        latency_(latency), rate_(rate == SIZE_MAX ? entryCapacity : rate) {
    if (entryCapacity_ == 0 || latency_ == 0 || rate_ == 0 ||
        rate_ > entryCapacity_)
      throw std::invalid_argument(
          "SimQueue capacity, latency, and rate are inconsistent");
  }

  // ── Capacity ────────────────────────────────────────────────────────

  size_t entryCapacity() const { return entryCapacity_; }
  size_t byteCapacity() const { return byteCapacity_; }
  size_t latency() const { return latency_; }
  size_t rate() const { return rate_; }

  size_t committedSize() const { return committed_.size(); }
  const std::vector<T> &committedValues() const { return committed_; }
  size_t committedBytes() const {
    if constexpr (PacketTraits<T>::serializedSize == 0)
      return 0;
    return committed_.size() * PacketTraits<T>::serializedSize;
  }
  bool isFull() const { return !canProposePush(); }
  bool isEmpty() const { return committed_.empty(); }
  bool canProposePush(size_t count = 1) const {
    return canProposePushWithAdditionalPops(count, 0);
  }
  bool canProposePushAfterPop(size_t count = 1) const {
    return canProposePop() && canProposePushWithAdditionalPops(count, 1);
  }
  bool canProposePop() const {
    return popProposalCount_ < rate_ && popProposalCount_ < committed_.size();
  }

  const T *peekProposable() const {
    return canProposePop() ? &committed_[popProposalCount_] : nullptr;
  }

private:
  bool canProposePushWithAdditionalPops(size_t count,
                                        size_t additionalPops) const {
    if (count > rate_ - std::min(rate_, pushProposals_.size()))
      return false;
    const size_t pops =
        std::min(committed_.size(), popProposalCount_ + additionalPops);
    const size_t occupied =
        committed_.size() - pops + delayed_.size() + pushProposals_.size();
    return count <= entryCapacity_ && occupied <= entryCapacity_ - count &&
           !exceedsByteCapacity(occupied + count);
  }

public:
  // ── Proposal interface ──────────────────────────────────────────────

  /// Propose to enqueue an element. Returns false if capacity exceeded.
  bool proposePush(T element) {
    if (!canProposePush())
      return false;
    pushProposals_.push_back(std::move(element));
    return true;
  }

  /// Propose to dequeue the next element in FIFO order.
  std::optional<T> proposePop() {
    if (popProposalCount_ >= committed_.size())
      return std::nullopt;
    size_t index = popProposalCount_;
    ++popProposalCount_;
    return std::optional<T>(committed_[index]);
  }

  /// Peek at the front without proposing a pop.
  const T *peek() const {
    return committed_.empty() ? nullptr : &committed_.front();
  }

  // ── Arbitration ─────────────────────────────────────────────────────

  void doArbitrate(Epoch) override {
    // Deterministic local arbitration: FIFO order.
    // Push proposals are appended in order.
    // Pop proposals are served from the front.
    // Arbitration is simple FIFO.
    for (size_t index = 0; index < pushProposals_.size(); ++index)
      emitObservation({.category = "transaction",
                       .name = "accepted",
                       .phase = TraceEventPhase::Instant});
    for (size_t index = 0; index < popProposalCount_; ++index)
      emitObservation({.category = "transaction",
                       .name = "completed",
                       .phase = TraceEventPhase::Instant});
    if (!pushProposals_.empty() || popProposalCount_ != 0) {
      const uint64_t occupancy =
          committed_.size() + pushProposals_.size() - popProposalCount_;
      emitObservation({.category = "queue",
                       .name = "occupancy",
                       .phase = TraceEventPhase::Counter,
                       .arguments = {{"occupancy", occupancy}}});
    }
  }

  // ── Xfer ────────────────────────────────────────────────────────────

  void doXfer(Epoch epoch) override {
    bool changed = hasPendingCommit();
    for (auto iterator = delayed_.begin(); iterator != delayed_.end();) {
      if (iterator->first > epoch.time) {
        ++iterator;
        continue;
      }
      committed_.push_back(std::move(iterator->second));
      iterator = delayed_.erase(iterator);
      ++totalPushes_;
    }
    for (auto &elem : pushProposals_) {
      if (latency_ == 1) {
        committed_.push_back(std::move(elem));
        ++totalPushes_;
      } else {
        delayed_.emplace_back(epoch.time + latency_ - 1, std::move(elem));
      }
    }
    pushProposals_.clear();

    // Commit pop proposals
    for (size_t i = 0; i < popProposalCount_ && !committed_.empty(); ++i) {
      committed_.erase(committed_.begin());
      ++totalPops_;
    }
    popProposalCount_ = 0;

    // Update statistics
    if (committedSize() > highWatermark_)
      highWatermark_ = committedSize();
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override {
    return !pushProposals_.empty() || !delayed_.empty() ||
           popProposalCount_ != 0;
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.queueOccupancy = committedSize();
    state.pendingOffers = pushProposals_.size() + delayed_.size();
    state.quiescent = committed_.empty() && !hasPendingCommit();
    if (!state.quiescent)
      state.reason = hasPendingCommit() ? "pending_commit" : "queue_not_empty";
    return state;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    auto append = [&](std::string suffix, uint64_t value, StatisticKind kind) {
      out.push_back({.name = std::move(suffix),
                     .objectPath = std::string(path()),
                     .kind = kind,
                     .value = value,
                     .lastUpdate = lastUpdate_});
    };
    append("queue_occupancy", committedSize(), StatisticKind::Gauge);
    append("queue_occupancy_peak", highWatermark_, StatisticKind::Gauge);
    append("accepted_transactions", totalPushes_, StatisticKind::Counter);
    append("completed_transactions", totalPops_, StatisticKind::Counter);
  }

  // ── Statistics ──────────────────────────────────────────────────────

  size_t highWatermark() const { return highWatermark_; }
  uint64_t totalPushes() const { return totalPushes_; }
  uint64_t totalPops() const { return totalPops_; }

  void reset() override {
    committed_.clear();
    delayed_.clear();
    pushProposals_.clear();
    popProposalCount_ = 0;
    highWatermark_ = 0;
    totalPushes_ = 0;
    totalPops_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  bool exceedsByteCapacity(size_t elementCount) const {
    if constexpr (PacketTraits<T>::serializedSize == 0)
      return false;
    return elementCount > byteCapacity_ / PacketTraits<T>::serializedSize;
  }

  size_t entryCapacity_;
  size_t byteCapacity_;
  size_t latency_;
  size_t rate_;
  std::vector<T> committed_;
  std::vector<std::pair<uint64_t, T>> delayed_;
  std::vector<T> pushProposals_;
  size_t popProposalCount_ = 0;
  size_t highWatermark_ = 0;
  uint64_t totalPushes_ = 0;
  uint64_t totalPops_ = 0;
  Epoch lastUpdate_;
};

/// Standard-library finite FIFO component. The distinct name is the public
/// component contract; SimQueue remains the underlying runtime primitive.
template <typename T> class Queue final : public SimQueue<T> {
public:
  static constexpr std::string_view contractName = "ac.Queue";
  static constexpr ObjectKind componentKind = ObjectKind::Queue;
  using SimQueue<T>::SimQueue;
};

// ── EventQueue ────────────────────────────────────────────────────────

/// Time-ordered event queue. Events are ordered by exact epoch, target object
/// ID, event kind, and payload.
class EventQueue : public SimObject {
public:
  EventQueue(std::string name, ObjectId id, SimObject *parent,
             size_t capacity = 1024)
      : SimObject(ObjectKind::EventQueue, std::move(name), id, parent),
        capacity_(capacity) {}

  // ── Capacity ────────────────────────────────────────────────────────

  size_t capacity() const { return capacity_; }
  size_t size() const { return committed_.size(); }
  bool isFull() const {
    return committed_.size() + pushProposals_.size() >= capacity_;
  }

  // ── Proposal ────────────────────────────────────────────────────────

  bool proposeSchedule(Event event) {
    if (committed_.size() + pushProposals_.size() >= capacity_)
      return false;
    pushProposals_.insert(event);
    return true;
  }

  // ── Xfer ────────────────────────────────────────────────────────────

  void doXfer(Epoch epoch) override {
    for (auto &event : pushProposals_)
      committed_.insert(event);
    pushProposals_.clear();
  }

  bool hasPendingCommit() const override { return !pushProposals_.empty(); }

  // ── Query ───────────────────────────────────────────────────────────

  std::optional<Event> nextEvent() const {
    if (committed_.empty())
      return std::nullopt;
    return *committed_.begin();
  }

  /// Pop the earliest event and return it.
  std::optional<Event> popNext() {
    if (committed_.empty())
      return std::nullopt;
    auto it = committed_.begin();
    Event e = *it;
    committed_.erase(it);
    return e;
  }

  bool hasEventAt(Epoch epoch) const {
    for (const auto &e : committed_)
      if (e.readyTime == epoch)
        return true;
    return false;
  }

  void reset() override {
    committed_.clear();
    pushProposals_.clear();
  }

private:
  size_t capacity_;
  std::multiset<Event> committed_;
  std::multiset<Event> pushProposals_;
};

} // namespace gfsim

#endif // GFSIM_QUEUE_H
