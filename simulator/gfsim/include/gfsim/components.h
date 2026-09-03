#ifndef GFSIM_COMPONENTS_H
#define GFSIM_COMPONENTS_H

#include "gfsim/core.h"
#include "gfsim/object.h"
#include "gfsim/queue.h"
#include "gfsim/resource.h"
#include "gfsim/trace.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace gfsim {

// ── Baseline component concepts ───────────────────────────────────────

/// A component that can be registered in the SimSystem.
template <typename T>
concept Component = std::derived_from<T, SimObject> && requires(T &t, Epoch e) {
  { T::contractName } -> std::convertible_to<std::string_view>;
  { T::componentKind } -> std::convertible_to<ObjectKind>;
  { t.doWork(e) } -> std::same_as<void>;
  { t.doXfer(e) } -> std::same_as<void>;
  { std::as_const(t).hasPendingCommit() } -> std::same_as<bool>;
};

// ── Compute ───────────────────────────────────────────────────────────

/// A stateless compute component that transforms inputs to outputs.
/// Pure, zero-delay, effect-free.
template <typename T> struct IdentityComputePolicy {
  T operator()(const T &input) const { return input; }
};

template <typename Input = uint64_t, typename Output = Input,
          typename FunctionalPolicy = IdentityComputePolicy<Input>>
  requires std::invocable<const FunctionalPolicy &, const Input &> &&
           std::convertible_to<
               std::invoke_result_t<const FunctionalPolicy &, const Input &>,
               Output>
class Compute : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.Compute";
  static constexpr ObjectKind componentKind = ObjectKind::Compute;

  Compute(std::string name, ObjectId id, SimObject *parent,
          FunctionalPolicy policy = {}, ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Compute, std::move(name), id, parent,
                  observations),
        policy_(std::move(policy)) {}

  void setInput(Input value) {
    inputProposal_ = std::move(value);
    hasInput_ = true;
  }

  void doWork(Epoch) override {
    if (hasInput_) {
      outputProposal_ = std::invoke(std::as_const(policy_), inputProposal_);
      hasOutput_ = true;
      emitObservation({.category = "transaction",
                       .name = "completed",
                       .phase = TraceEventPhase::Complete,
                       .duration = 0});
    }
  }

  void doXfer(Epoch epoch) override {
    if (hasOutput_) {
      output_ = outputProposal_;
      hasOutput_ = false;
      ++totalComputations_;
      lastUpdate_ = epoch;
    }
    hasInput_ = false;
  }

  bool hasPendingCommit() const override { return hasOutput_; }

  const Output &output() const { return output_; }
  bool isRunnable(Epoch) const override { return hasInput_; }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "completed_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = totalComputations_,
                   .lastUpdate = lastUpdate_});
  }

  void reset() override {
    output_ = {};
    outputProposal_ = {};
    inputProposal_ = {};
    hasInput_ = false;
    hasOutput_ = false;
    totalComputations_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  [[no_unique_address]] FunctionalPolicy policy_;
  Output output_{};
  Output outputProposal_{};
  Input inputProposal_{};
  bool hasInput_ = false;
  bool hasOutput_ = false;
  uint64_t totalComputations_ = 0;
  Epoch lastUpdate_;
};

// ── Sink ──────────────────────────────────────────────────────────────

/// A terminal component that consumes data and produces statistics.
template <typename T = uint64_t> class Sink : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.Sink";
  static constexpr ObjectKind componentKind = ObjectKind::Sink;

  Sink(std::string name, ObjectId id, SimObject *parent,
       ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Sink, std::move(name), id, parent, observations) {
  }

  void receive(T value) { receivedProposals_.push_back(std::move(value)); }

  void doArbitrate(Epoch) override {
    for (size_t index = 0; index < receivedProposals_.size(); ++index)
      emitObservation({.category = "transaction",
                       .name = "accepted",
                       .phase = TraceEventPhase::Instant});
  }

  void doXfer(Epoch epoch) override {
    bool changed = !receivedProposals_.empty();
    for (auto v : receivedProposals_) {
      received_.push_back(v);
      ++totalReceived_;
    }
    receivedProposals_.clear();
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override { return !receivedProposals_.empty(); }

  const std::vector<T> &received() const { return received_; }
  uint64_t totalReceived() const { return totalReceived_; }

  bool isRunnable(Epoch) const override { return false; }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "accepted_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = totalReceived_,
                   .lastUpdate = lastUpdate_});
  }

  void reset() override {
    received_.clear();
    receivedProposals_.clear();
    totalReceived_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  std::vector<T> received_;
  std::vector<T> receivedProposals_;
  uint64_t totalReceived_ = 0;
  Epoch lastUpdate_;
};

// ── Link ──────────────────────────────────────────────────────────────

/// A connector that forwards data between components.
template <typename T = uint64_t> class Link : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.Link";
  static constexpr ObjectKind componentKind = ObjectKind::Link;

  Link(std::string name, ObjectId id, SimObject *parent,
       ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Link, std::move(name), id, parent, observations) {
  }

  void forward(T value) {
    forwardedProposal_ = std::move(value);
    hasProposal_ = true;
  }

  void doArbitrate(Epoch) override {
    if (hasProposal_)
      emitObservation({.category = "transaction",
                       .name = "completed",
                       .phase = TraceEventPhase::Instant});
  }

  void doXfer(Epoch epoch) override {
    if (hasProposal_) {
      forwarded_ = forwardedProposal_;
      hasForwarded_ = true;
      hasProposal_ = false;
      ++totalTransfers_;
      lastUpdate_ = epoch;
    }
  }

  bool hasPendingCommit() const override { return hasProposal_; }

  const T &value() const { return forwarded_; }
  bool hasValue() const { return hasForwarded_; }

  bool isRunnable(Epoch) const override { return hasProposal_; }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "completed_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = totalTransfers_,
                   .lastUpdate = lastUpdate_});
  }

  void reset() override {
    forwarded_ = {};
    forwardedProposal_ = {};
    hasProposal_ = false;
    hasForwarded_ = false;
    totalTransfers_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  T forwarded_{};
  T forwardedProposal_{};
  bool hasProposal_ = false;
  bool hasForwarded_ = false;
  uint64_t totalTransfers_ = 0;
  Epoch lastUpdate_;
};

// ── Memory ────────────────────────────────────────────────────────────

/// A storage component with read/write proposals and capacity.
template <typename T = uint64_t> class Memory : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.Memory";
  static constexpr ObjectKind componentKind = ObjectKind::Memory;

  Memory(std::string name, ObjectId id, SimObject *parent, size_t capacity,
         ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Memory, std::move(name), id, parent,
                  observations),
        storage_(capacity) {}

  size_t capacity() const { return storage_.size(); }

  bool proposeWrite(size_t addr, T value) {
    if (addr >= storage_.size())
      return false;
    writeProposals_[addr] = std::move(value);
    return true;
  }

  T read(size_t addr) const {
    if (addr >= storage_.size())
      return {};
    return storage_[addr];
  }

  void doArbitrate(Epoch) override {
    for (const auto &proposal : writeProposals_)
      emitObservation(
          {.category = "memory",
           .name = "write",
           .phase = TraceEventPhase::Instant,
           .arguments = {{"address", static_cast<uint64_t>(proposal.first)}}});
  }

  void doXfer(Epoch epoch) override {
    size_t committedWrites = writeProposals_.size();
    for (auto &[addr, value] : writeProposals_)
      storage_[addr] = value;
    writeProposals_.clear();
    totalWrites_ += committedWrites;
    if (committedWrites != 0)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override { return !writeProposals_.empty(); }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "accepted_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = totalWrites_,
                   .lastUpdate = lastUpdate_});
  }

  void reset() override {
    std::fill(storage_.begin(), storage_.end(), T{});
    writeProposals_.clear();
    totalWrites_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  std::vector<T> storage_;
  std::map<size_t, T> writeProposals_;
  uint64_t totalWrites_ = 0;
  Epoch lastUpdate_;
};

// ── Scheduler ────────────────────────────────────────────────────────

/// A finite deterministic scheduler. Proposal admission checks identity;
/// arbitration selects by stable keys and never by insertion order.
template <typename T> class Scheduler final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.Scheduler";
  static constexpr ObjectKind componentKind = ObjectKind::Scheduler;

  Scheduler(std::string name, ObjectId id, SimObject *parent, size_t capacity,
            ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Scheduler, std::move(name), id, parent,
                  observations),
        capacity_(capacity) {}

  bool proposeSchedule(T value, uint32_t priority, uint32_t portIndex,
                       uint32_t instanceIndex, ObjectId ownerId,
                       uint64_t transactionId) {
    if (ownerId == kInvalidObjectId || containsIdentity(ownerId, transactionId))
      return false;
    proposals_.push_back({std::move(value),
                          priority,
                          {},
                          portIndex,
                          instanceIndex,
                          ownerId,
                          transactionId});
    return true;
  }

  std::optional<T> proposePop() {
    if (popProposalCount_ >= committed_.size())
      return std::nullopt;
    return committed_[popProposalCount_++].value;
  }

  const T *peek() const {
    return committed_.empty() ? nullptr : &committed_.front().value;
  }

  void doArbitrate(Epoch epoch) override {
    for (Entry &entry : proposals_)
      entry.issueEpoch = epoch;
    std::sort(proposals_.begin(), proposals_.end(), stableLess);
    size_t occupied = committed_.size() + accepted_.size();
    size_t available = capacity_ > occupied ? capacity_ - occupied : 0;
    size_t acceptedCount = std::min(available, proposals_.size());
    accepted_.insert(
        accepted_.end(), std::make_move_iterator(proposals_.begin()),
        std::make_move_iterator(proposals_.begin() + acceptedCount));
    rejected_.insert(
        rejected_.end(),
        std::make_move_iterator(proposals_.begin() + acceptedCount),
        std::make_move_iterator(proposals_.end()));
    proposals_.clear();
    auto observe = [&](const Entry &entry, std::string category,
                       std::string name) {
      emitObservation({.category = std::move(category),
                       .name = std::move(name),
                       .phase = TraceEventPhase::Instant,
                       .rootSequenceId = entry.transactionId,
                       .arguments = {{"child_owner_id",
                                      static_cast<uint64_t>(entry.ownerId)},
                                     {"transaction_id", entry.transactionId}}});
    };
    for (const Entry &entry : accepted_)
      observe(entry, "transaction", "accepted");
    for (const Entry &entry : rejected_)
      observe(entry, "stall", "capacity");
    if (!accepted_.empty() || !rejected_.empty() || popProposalCount_ != 0) {
      const uint64_t occupancy =
          committed_.size() - std::min(popProposalCount_, committed_.size()) +
          accepted_.size();
      emitObservation({.category = "queue",
                       .name = "occupancy",
                       .phase = TraceEventPhase::Counter,
                       .arguments = {{"occupancy", occupancy}}});
    }
  }

  void doXfer(Epoch epoch) override {
    bool changed = hasPendingCommit();
    size_t popped = std::min(popProposalCount_, committed_.size());
    committed_.erase(committed_.begin(), committed_.begin() + popped);
    totalPops_ += popped;
    popProposalCount_ = 0;

    totalScheduled_ += accepted_.size();
    committed_.insert(committed_.end(),
                      std::make_move_iterator(accepted_.begin()),
                      std::make_move_iterator(accepted_.end()));
    accepted_.clear();
    std::sort(committed_.begin(), committed_.end(), stableLess);

    for (const Entry &entry : rejected_)
      rejectedTransactions_.push_back(entry.transactionId);
    rejected_.clear();
    highWatermark_ = std::max(highWatermark_, committed_.size());
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override {
    return !proposals_.empty() || !accepted_.empty() || !rejected_.empty() ||
           popProposalCount_ != 0;
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.queueOccupancy = committed_.size();
    state.pendingOffers =
        proposals_.size() + accepted_.size() + rejected_.size();
    state.quiescent = committed_.empty() && !hasPendingCommit();
    if (!state.quiescent)
      state.reason = hasPendingCommit() ? "scheduler_pending_proposal"
                                        : "scheduler_not_empty";
    return state;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    auto append = [&](std::string name, uint64_t value, StatisticKind kind) {
      out.push_back({.name = std::move(name),
                     .objectPath = std::string(path()),
                     .kind = kind,
                     .value = value,
                     .lastUpdate = lastUpdate_});
    };
    append("queue_occupancy", committed_.size(), StatisticKind::Gauge);
    append("queue_occupancy_peak", highWatermark_, StatisticKind::Gauge);
    append("accepted_transactions", totalScheduled_, StatisticKind::Counter);
    append("completed_transactions", totalPops_, StatisticKind::Counter);
  }

  bool isRunnable(Epoch) const override { return !proposals_.empty(); }
  size_t capacity() const { return capacity_; }
  size_t size() const { return committed_.size(); }
  bool empty() const { return committed_.empty(); }
  size_t highWatermark() const { return highWatermark_; }
  uint64_t totalScheduled() const { return totalScheduled_; }
  uint64_t totalPops() const { return totalPops_; }
  const std::vector<uint64_t> &rejectedTransactions() const {
    return rejectedTransactions_;
  }

  bool validate() const {
    if (committed_.size() > capacity_)
      return false;
    for (size_t left = 0; left < committed_.size(); ++left)
      for (size_t right = left + 1; right < committed_.size(); ++right)
        if (sameIdentity(committed_[left], committed_[right]))
          return false;
    return true;
  }

  void reset() override {
    proposals_.clear();
    accepted_.clear();
    rejected_.clear();
    committed_.clear();
    rejectedTransactions_.clear();
    popProposalCount_ = 0;
    highWatermark_ = 0;
    totalScheduled_ = 0;
    totalPops_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  struct Entry {
    T value;
    uint32_t priority;
    Epoch issueEpoch;
    uint32_t portIndex;
    uint32_t instanceIndex;
    ObjectId ownerId;
    uint64_t transactionId;
  };

  static auto stableKey(const Entry &entry) {
    return std::tie(entry.priority, entry.issueEpoch, entry.portIndex,
                    entry.instanceIndex, entry.ownerId, entry.transactionId);
  }
  static bool stableLess(const Entry &left, const Entry &right) {
    return stableKey(left) < stableKey(right);
  }
  static bool sameIdentity(const Entry &left, const Entry &right) {
    return left.ownerId == right.ownerId &&
           left.transactionId == right.transactionId;
  }
  bool containsIdentity(ObjectId ownerId, uint64_t transactionId) const {
    auto matches = [&](const Entry &entry) {
      return entry.ownerId == ownerId && entry.transactionId == transactionId;
    };
    return std::any_of(proposals_.begin(), proposals_.end(), matches) ||
           std::any_of(accepted_.begin(), accepted_.end(), matches) ||
           std::any_of(committed_.begin(), committed_.end(), matches);
  }

  size_t capacity_;
  std::vector<Entry> proposals_;
  std::vector<Entry> accepted_;
  std::vector<Entry> rejected_;
  std::vector<Entry> committed_;
  std::vector<uint64_t> rejectedTransactions_;
  size_t popProposalCount_ = 0;
  size_t highWatermark_ = 0;
  uint64_t totalScheduled_ = 0;
  uint64_t totalPops_ = 0;
  Epoch lastUpdate_;
};

// ── ReadyValid ────────────────────────────────────────────────────────

template <typename T> class ReadyValid : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.ready_valid";
  static constexpr ObjectKind componentKind = ObjectKind::Link;

  ReadyValid(std::string name, ObjectId id, SimObject *parent,
             ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Link, std::move(name), id, parent, observations) {
  }

  bool proposeOffer(T data) {
    if (offer_ || offerProposal_)
      return false;
    offerProposal_ = std::move(data);
    return true;
  }

  void proposeReady(bool ready) { readyProposal_ = ready; }
  bool isReady() const { return ready_; }
  bool hasOffer() const { return offer_.has_value(); }
  const T *peekOffer() const { return offer_ ? &*offer_ : nullptr; }

  void doArbitrate(Epoch) override {
    const bool nextReady = readyProposal_.value_or(ready_);
    const bool nextOffer = offer_.has_value() || offerProposal_.has_value();
    if (nextOffer && nextReady) {
      emitObservation({.category = "transaction",
                       .name = "completed",
                       .phase = TraceEventPhase::Instant});
    } else if (offerProposal_ && !nextReady) {
      emitObservation({.category = "stall",
                       .name = "backpressure",
                       .phase = TraceEventPhase::Instant,
                       .arguments = {{"pending_offers", uint64_t{1}}}});
    }
  }

  void doXfer(Epoch epoch) override {
    bool changed = hasPendingCommit() || (offer_ && ready_);
    if (readyProposal_)
      ready_ = *readyProposal_;
    if (offerProposal_)
      offer_ = std::move(offerProposal_);
    readyProposal_.reset();
    offerProposal_.reset();

    if (offer_ && ready_) {
      lastTransferred_ = *offer_;
      offer_.reset();
      ++transferCount_;
    }
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override {
    return offerProposal_.has_value() || readyProposal_.has_value();
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.pendingOffers = offer_.has_value() + offerProposal_.has_value();
    state.protocolState = ready_ ? "ready" : "backpressure";
    state.quiescent = !offer_ && !hasPendingCommit();
    if (!state.quiescent)
      state.reason =
          offer_ ? "ready_valid_offer_blocked" : "ready_valid_pending_proposal";
    return state;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "completed_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = transferCount_,
                   .lastUpdate = lastUpdate_});
  }

  const T &lastTransferred() const { return lastTransferred_; }
  uint64_t transferCount() const { return transferCount_; }
  bool isRunnable(Epoch) const override { return hasPendingCommit(); }
  bool validate() const { return !(offer_ && ready_); }

  void reset() override {
    ready_ = false;
    offer_.reset();
    offerProposal_.reset();
    readyProposal_.reset();
    lastTransferred_ = {};
    transferCount_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  bool ready_ = false;
  std::optional<T> offer_;
  std::optional<T> offerProposal_;
  std::optional<bool> readyProposal_;
  T lastTransferred_{};
  uint64_t transferCount_ = 0;
  Epoch lastUpdate_;
};

// ── RequestResponse ──────────────────────────────────────────────────

template <typename Req, typename Resp>
class RequestResponse : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.request_response";
  static constexpr ObjectKind componentKind = ObjectKind::Link;

  RequestResponse(std::string name, ObjectId id, SimObject *parent,
                  size_t maxInFlight = 16,
                  ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Link, std::move(name), id, parent, observations),
        maxInFlight_(maxInFlight) {}

  struct RequestEnvelope {
    Req payload;
    uint64_t correlationId;
  };
  struct ResponseEnvelope {
    Resp payload;
    uint64_t correlationId;
  };

  bool proposeRequest(Req request, uint64_t correlationId) {
    if (active_.size() + requestProposals_.size() >= maxInFlight_ ||
        containsCorrelation(correlationId))
      return false;
    requestProposals_.push_back({std::move(request), correlationId});
    return true;
  }

  const RequestEnvelope *peekRequest() const {
    return committedRequests_.empty() ? nullptr : &committedRequests_.front();
  }

  std::optional<RequestEnvelope> proposePopRequest() {
    if (requestPopCount_ >= committedRequests_.size())
      return std::nullopt;
    return committedRequests_[requestPopCount_++];
  }

  bool proposeResponse(Resp response, uint64_t correlationId) {
    if (!active_.contains(correlationId) ||
        !deliveredRequests_.contains(correlationId) ||
        std::any_of(responseProposals_.begin(), responseProposals_.end(),
                    [&](const ResponseEnvelope &entry) {
                      return entry.correlationId == correlationId;
                    }))
      return false;
    responseProposals_.push_back({std::move(response), correlationId});
    return true;
  }

  bool hasResponse() const { return !committedResponses_.empty(); }
  const ResponseEnvelope *peekResponse() const {
    return committedResponses_.empty() ? nullptr : &committedResponses_.front();
  }
  std::optional<ResponseEnvelope> proposePopResponse() {
    if (responsePopCount_ >= committedResponses_.size())
      return std::nullopt;
    return committedResponses_[responsePopCount_++];
  }

  void doArbitrate(Epoch) override {
    auto observe = [&](uint64_t correlationId, std::string name) {
      emitObservation({.category = "transaction",
                       .name = std::move(name),
                       .phase = TraceEventPhase::Instant,
                       .rootSequenceId = correlationId,
                       .arguments = {{"correlation_id", correlationId}}});
    };
    for (const RequestEnvelope &request : requestProposals_)
      observe(request.correlationId, "accepted");
    for (size_t index = 0;
         index < std::min(requestPopCount_, committedRequests_.size()); ++index)
      observe(committedRequests_[index].correlationId, "request");
    for (const ResponseEnvelope &response : responseProposals_)
      observe(response.correlationId, "completed");
    for (size_t index = 0;
         index < std::min(responsePopCount_, committedResponses_.size());
         ++index)
      observe(committedResponses_[index].correlationId, "response");
  }

  void doXfer(Epoch epoch) override {
    bool changed = hasPendingCommit();
    size_t requestPops = std::min(requestPopCount_, committedRequests_.size());
    for (size_t index = 0; index < requestPops; ++index)
      deliveredRequests_.insert(committedRequests_[index].correlationId);
    committedRequests_.erase(committedRequests_.begin(),
                             committedRequests_.begin() + requestPops);
    requestPopCount_ = 0;

    size_t responsePops =
        std::min(responsePopCount_, committedResponses_.size());
    committedResponses_.erase(committedResponses_.begin(),
                              committedResponses_.begin() + responsePops);
    responsePopCount_ = 0;

    for (auto &request : requestProposals_) {
      active_.insert(request.correlationId);
      committedRequests_.push_back(std::move(request));
    }
    requestProposals_.clear();

    for (auto &response : responseProposals_) {
      active_.erase(response.correlationId);
      deliveredRequests_.erase(response.correlationId);
      committedResponses_.push_back(std::move(response));
      ++totalCompleted_;
    }
    responseProposals_.clear();
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override {
    return !requestProposals_.empty() || !responseProposals_.empty() ||
           requestPopCount_ != 0 || responsePopCount_ != 0;
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.pendingOffers = requestProposals_.size() + responseProposals_.size() +
                          committedRequests_.size() +
                          committedResponses_.size();
    state.correlationChain.assign(active_.begin(), active_.end());
    state.protocolState = active_.empty() ? "idle" : "in_flight";
    state.quiescent =
        active_.empty() && state.pendingOffers == 0 && !hasPendingCommit();
    if (!state.quiescent)
      state.reason = "request_response_blocked";
    return state;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "completed_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = totalCompleted_,
                   .lastUpdate = lastUpdate_});
    out.push_back({.name = "active_correlations",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Gauge,
                   .value = active_.size(),
                   .lastUpdate = lastUpdate_});
  }

  size_t inFlight() const { return active_.size(); }
  size_t maxInFlight() const { return maxInFlight_; }
  uint64_t totalCompleted() const { return totalCompleted_; }

  bool validate() const {
    if (active_.size() > maxInFlight_ ||
        active_.size() + requestProposals_.size() > maxInFlight_)
      return false;
    for (uint64_t correlationId : deliveredRequests_)
      if (!active_.contains(correlationId))
        return false;
    for (const RequestEnvelope &request : committedRequests_)
      if (!active_.contains(request.correlationId))
        return false;
    for (const ResponseEnvelope &response : responseProposals_)
      if (!active_.contains(response.correlationId) ||
          !deliveredRequests_.contains(response.correlationId))
        return false;
    return true;
  }

  void reset() override {
    requestProposals_.clear();
    responseProposals_.clear();
    committedRequests_.clear();
    committedResponses_.clear();
    active_.clear();
    deliveredRequests_.clear();
    requestPopCount_ = 0;
    responsePopCount_ = 0;
    totalCompleted_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  bool containsCorrelation(uint64_t correlationId) const {
    auto requestMatches = [&](const RequestEnvelope &entry) {
      return entry.correlationId == correlationId;
    };
    auto responseMatches = [&](const ResponseEnvelope &entry) {
      return entry.correlationId == correlationId;
    };
    return active_.contains(correlationId) ||
           std::any_of(requestProposals_.begin(), requestProposals_.end(),
                       requestMatches) ||
           std::any_of(committedResponses_.begin(), committedResponses_.end(),
                       responseMatches);
  }

  size_t maxInFlight_;
  uint64_t totalCompleted_ = 0;
  std::vector<RequestEnvelope> requestProposals_;
  std::vector<RequestEnvelope> committedRequests_;
  std::vector<ResponseEnvelope> responseProposals_;
  std::vector<ResponseEnvelope> committedResponses_;
  std::set<uint64_t> active_;
  std::set<uint64_t> deliveredRequests_;
  size_t requestPopCount_ = 0;
  size_t responsePopCount_ = 0;
  Epoch lastUpdate_;
};

// ── Protocol state ────────────────────────────────────────────────────

enum class ProtocolPhase : uint8_t {
  Idle,
  Request,
  Response,
  Transfer,
  Backpressure,
};

class ProtocolState {
public:
  explicit ProtocolState(size_t maxCredits = 1)
      : maxCredits_(maxCredits), credits_(maxCredits) {}
  ProtocolPhase phase() const { return phase_; }
  size_t credits() const { return credits_; }
  size_t inFlight() const { return inFlight_; }
  bool canSend() const {
    return phase_ != ProtocolPhase::Backpressure && credits_ > 0 &&
           inFlight_ < maxCredits_;
  }
  bool canReceive() const { return inFlight_ > 0; }
  bool startRequest() {
    if (!canSend())
      return false;
    --credits_;
    ++inFlight_;
    phase_ = ProtocolPhase::Request;
    return true;
  }
  bool beginResponse() {
    if (phase_ == ProtocolPhase::Backpressure || inFlight_ == 0)
      return false;
    phase_ = ProtocolPhase::Response;
    return true;
  }
  bool completeResponse() {
    if (phase_ != ProtocolPhase::Response || inFlight_ == 0)
      return false;
    --inFlight_;
    ++credits_;
    phase_ = inFlight_ > 0 ? ProtocolPhase::Transfer : ProtocolPhase::Idle;
    return true;
  }
  bool setBackpressure(bool enabled) {
    if (enabled) {
      if (phase_ == ProtocolPhase::Backpressure)
        return false;
      resumePhase_ = phase_;
      phase_ = ProtocolPhase::Backpressure;
      return true;
    }
    if (phase_ != ProtocolPhase::Backpressure)
      return false;
    phase_ = resumePhase_;
    return true;
  }
  bool validate() const {
    if (credits_ > maxCredits_ || inFlight_ > maxCredits_ ||
        credits_ + inFlight_ != maxCredits_)
      return false;
    if (phase_ == ProtocolPhase::Idle)
      return inFlight_ == 0;
    if (phase_ == ProtocolPhase::Backpressure)
      return resumePhase_ == ProtocolPhase::Idle || inFlight_ > 0;
    return inFlight_ > 0;
  }
  void reset() {
    phase_ = ProtocolPhase::Idle;
    resumePhase_ = ProtocolPhase::Idle;
    credits_ = maxCredits_;
    inFlight_ = 0;
  }

private:
  ProtocolPhase phase_ = ProtocolPhase::Idle;
  ProtocolPhase resumePhase_ = ProtocolPhase::Idle;
  size_t maxCredits_, credits_;
  size_t inFlight_ = 0;
};

// ── No-progress diagnostics ──────────────────────────────────────────

} // namespace gfsim

#endif // GFSIM_COMPONENTS_H
