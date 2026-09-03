#ifndef GFSIM_RESOURCE_H
#define GFSIM_RESOURCE_H

#include "gfsim/core.h"
#include "gfsim/object.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace gfsim {

class Resource : public SimObject {
public:
  struct Reservation {
    ObjectId ownerId = kInvalidObjectId;
    uint32_t amount = 0;
    Epoch issueTime;
    Epoch readyTime;
    uint64_t transactionId = 0;
    uint64_t rootTransactionId = 0;
    uint32_t priority = 0;
    uint32_t portIndex = 0;
    uint32_t instanceIndex = 0;
  };

  Resource(std::string name, ObjectId id, SimObject *parent,
           uint32_t totalCapacity, ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::Resource, std::move(name), id, parent,
                  observations),
        totalCapacity_(totalCapacity) {}

  uint32_t totalCapacity() const { return totalCapacity_; }
  uint32_t availableCapacity() const {
    return totalCapacity_ - activeCapacity_;
  }
  uint32_t activeReservations() const { return activeCapacity_; }
  bool canReserve(uint32_t amount = 1) const {
    return amount != 0 && availableCapacity() >= amount;
  }

  bool proposeReserve(ObjectId owner, uint32_t amount, Epoch issueTime,
                      uint64_t transactionId) {
    return proposeReserve(owner, amount, issueTime, transactionId, issueTime,
                          transactionId);
  }

  bool proposeReserve(ObjectId owner, uint32_t amount, Epoch issueTime,
                      uint64_t transactionId, Epoch readyTime,
                      uint64_t rootTransactionId) {
    return proposeReserve({owner, amount, issueTime, readyTime, transactionId,
                           rootTransactionId});
  }

  bool proposeReserve(Reservation request) {
    if (request.ownerId == kInvalidObjectId || request.amount == 0 ||
        request.amount > totalCapacity_ ||
        request.readyTime < request.issueTime ||
        hasReservation(request.transactionId) ||
        std::ranges::any_of(
            proposals_,
            [&](const Reservation &proposal) {
              return proposal.transactionId == request.transactionId;
            }))
      return false;
    proposals_.push_back(request);
    return true;
  }

  bool proposeRelease(ObjectId owner, uint32_t amount) {
    if (amount == 0)
      return false;
    uint32_t releasable = 0;
    for (const auto &[transactionId, reservation] : reservations_) {
      if (reservation.ownerId == owner && !isCancellationPending(transactionId))
        releasable += reservation.amount;
    }
    for (const ReleaseProposal &release : releaseProposals_)
      if (release.ownerId == owner)
        releasable =
            release.amount > releasable ? 0 : releasable - release.amount;
    if (amount > releasable)
      return false;
    releaseProposals_.push_back({owner, amount});
    return true;
  }

  bool proposeCancel(ObjectId owner, uint64_t transactionId) {
    const Reservation *reservation = findReservation(transactionId);
    if (!reservation || reservation->ownerId != owner ||
        isCancellationPending(transactionId))
      return false;
    cancellationProposals_.push_back(transactionId);
    return true;
  }

  void doArbitrate(Epoch) override {
    acceptedProposals_.clear();
    proposedRejectedTransactions_.clear();
    std::sort(proposals_.begin(), proposals_.end(), reservationLess);
    uint32_t remaining = availableCapacity();
    for (const Reservation &proposal : proposals_) {
      if (proposal.amount <= remaining) {
        acceptedProposals_.push_back(proposal);
        remaining -= proposal.amount;
      } else {
        proposedRejectedTransactions_.push_back(proposal.transactionId);
      }
    }
    for (const Reservation &reservation : acceptedProposals_) {
      auto start = presentationTime(reservation.issueTime);
      auto end = presentationTime(reservation.readyTime);
      if (!start || !end) {
        setRuntimeFailureCode("observation_time_overflow");
        continue;
      }
      emitObservation(
          {.category = "resource",
           .name = "reservation",
           .phase = TraceEventPhase::Complete,
           .rootSequenceId = reservation.rootTransactionId,
           .duration = *end - *start,
           .arguments = {
               {"amount", static_cast<uint64_t>(reservation.amount)},
               {"child_owner_id", static_cast<uint64_t>(reservation.ownerId)},
               {"transaction_id", reservation.transactionId}}});
    }
    for (const Reservation &reservation : proposals_) {
      if (std::ranges::find(proposedRejectedTransactions_,
                            reservation.transactionId) ==
          proposedRejectedTransactions_.end())
        continue;
      emitObservation(
          {.category = "stall",
           .name = "capacity",
           .phase = TraceEventPhase::Instant,
           .rootSequenceId = reservation.rootTransactionId,
           .arguments = {
               {"amount", static_cast<uint64_t>(reservation.amount)},
               {"child_owner_id", static_cast<uint64_t>(reservation.ownerId)},
               {"transaction_id", reservation.transactionId}}});
    }
  }

  void doXfer(Epoch epoch) override {
    bool changed = !acceptedProposals_.empty() || !releaseProposals_.empty() ||
                   !cancellationProposals_.empty() ||
                   !proposedRejectedTransactions_.empty();
    for (const Reservation &reservation : acceptedProposals_) {
      reservations_.emplace(reservation.transactionId, reservation);
      activeCapacity_ += reservation.amount;
      totalReservations_ += reservation.amount;
    }

    for (uint64_t transactionId : cancellationProposals_) {
      auto reservation = reservations_.find(transactionId);
      if (reservation == reservations_.end())
        continue;
      activeCapacity_ -= reservation->second.amount;
      totalCancellations_ += reservation->second.amount;
      reservations_.erase(reservation);
    }

    for (const ReleaseProposal &release : releaseProposals_)
      applyRelease(release);

    rejectedTransactions_ = proposedRejectedTransactions_;

    proposals_.clear();
    acceptedProposals_.clear();
    releaseProposals_.clear();
    cancellationProposals_.clear();
    proposedRejectedTransactions_.clear();
    if (activeCapacity_ > highWatermark_)
      highWatermark_ = activeCapacity_;
    if (changed)
      lastUpdate_ = epoch;
  }

  bool hasPendingCommit() const override {
    return !acceptedProposals_.empty() || !releaseProposals_.empty() ||
           !cancellationProposals_.empty() ||
           !proposedRejectedTransactions_.empty();
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.activeReservations = activeCapacity_;
    state.pendingOffers = proposals_.size() + acceptedProposals_.size() +
                          releaseProposals_.size() +
                          cancellationProposals_.size() +
                          proposedRejectedTransactions_.size();
    state.quiescent = reservations_.empty() && state.pendingOffers == 0;
    if (!state.quiescent)
      state.reason = state.pendingOffers ? "resource_pending_proposal"
                                         : "resource_reservation_live";
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
    append("active_reservations", activeCapacity_, StatisticKind::Gauge);
    append("reservation_peak", highWatermark_, StatisticKind::Gauge);
    append("total_reservations", totalReservations_, StatisticKind::Counter);
    append("total_releases", totalReleases_, StatisticKind::Counter);
    append("total_cancellations", totalCancellations_, StatisticKind::Counter);
  }

  bool hasReservation(uint64_t transactionId) const {
    return reservations_.contains(transactionId);
  }

  const Reservation *findReservation(uint64_t transactionId) const {
    auto reservation = reservations_.find(transactionId);
    return reservation == reservations_.end() ? nullptr : &reservation->second;
  }

  std::vector<const Reservation *> readyReservations(Epoch epoch) const {
    std::vector<const Reservation *> ready;
    for (const auto &[transactionId, reservation] : reservations_)
      if (reservation.readyTime == epoch)
        ready.push_back(&reservation);
    std::sort(ready.begin(), ready.end(),
              [](const Reservation *left, const Reservation *right) {
                return reservationLess(*left, *right);
              });
    return ready;
  }

  const std::vector<uint64_t> &rejectedTransactions() const {
    return rejectedTransactions_;
  }

  bool validate() const {
    uint64_t capacity = 0;
    for (const auto &[transactionId, reservation] : reservations_) {
      if (transactionId != reservation.transactionId ||
          reservation.ownerId == kInvalidObjectId || reservation.amount == 0 ||
          reservation.readyTime < reservation.issueTime)
        return false;
      capacity += reservation.amount;
    }
    return capacity == activeCapacity_ && capacity <= totalCapacity_;
  }

  uint32_t highWatermark() const { return highWatermark_; }
  uint64_t totalReservations() const { return totalReservations_; }
  uint64_t totalReleases() const { return totalReleases_; }
  uint64_t totalCancellations() const { return totalCancellations_; }

  void reset() override {
    activeCapacity_ = 0;
    proposals_.clear();
    acceptedProposals_.clear();
    rejectedTransactions_.clear();
    proposedRejectedTransactions_.clear();
    releaseProposals_.clear();
    cancellationProposals_.clear();
    reservations_.clear();
    highWatermark_ = 0;
    totalReservations_ = 0;
    totalReleases_ = 0;
    totalCancellations_ = 0;
    lastUpdate_ = {};
    clearRuntimeFailureCode();
  }

private:
  struct ReleaseProposal {
    ObjectId ownerId;
    uint32_t amount;
  };

  static bool reservationLess(const Reservation &left,
                              const Reservation &right) {
    return std::tie(left.priority, left.portIndex, left.instanceIndex,
                    left.ownerId, left.rootTransactionId, left.transactionId) <
           std::tie(right.priority, right.portIndex, right.instanceIndex,
                    right.ownerId, right.rootTransactionId,
                    right.transactionId);
  }

  static std::optional<uint64_t> presentationTime(Epoch epoch) {
    if (epoch.time > (std::numeric_limits<uint64_t>::max() - epoch.delta) /
                         kMaxDeltasPerTick)
      return std::nullopt;
    return epoch.time * kMaxDeltasPerTick + epoch.delta;
  }

  bool isCancellationPending(uint64_t transactionId) const {
    return std::ranges::find(cancellationProposals_, transactionId) !=
           cancellationProposals_.end();
  }

  void applyRelease(const ReleaseProposal &release) {
    std::vector<Reservation *> owned;
    for (auto &[transactionId, reservation] : reservations_)
      if (reservation.ownerId == release.ownerId)
        owned.push_back(&reservation);
    std::sort(owned.begin(), owned.end(),
              [](const Reservation *left, const Reservation *right) {
                return reservationLess(*left, *right);
              });

    uint32_t remaining = release.amount;
    std::vector<uint64_t> emptyReservations;
    for (Reservation *reservation : owned) {
      uint32_t released = std::min(remaining, reservation->amount);
      reservation->amount -= released;
      activeCapacity_ -= released;
      totalReleases_ += released;
      remaining -= released;
      if (reservation->amount == 0)
        emptyReservations.push_back(reservation->transactionId);
      if (remaining == 0)
        break;
    }
    for (uint64_t transactionId : emptyReservations)
      reservations_.erase(transactionId);
  }

  uint32_t totalCapacity_ = 0;
  uint32_t activeCapacity_ = 0;
  uint32_t highWatermark_ = 0;
  uint64_t totalReservations_ = 0;
  uint64_t totalReleases_ = 0;
  uint64_t totalCancellations_ = 0;
  std::vector<Reservation> proposals_;
  std::vector<Reservation> acceptedProposals_;
  std::vector<uint64_t> rejectedTransactions_;
  std::vector<uint64_t> proposedRejectedTransactions_;
  std::vector<ReleaseProposal> releaseProposals_;
  std::vector<uint64_t> cancellationProposals_;
  std::map<uint64_t, Reservation> reservations_;
  Epoch lastUpdate_;
};

} // namespace gfsim

#endif // GFSIM_RESOURCE_H
