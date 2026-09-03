#ifndef GFSIM_STATISTICS_H
#define GFSIM_STATISTICS_H

#include "gfsim/object.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace gfsim {

/// Deterministic counter, gauge, or histogram. Mutations are private proposals
/// and become visible only at the Xfer barrier.
class Statistic final : public SimObject {
public:
  Statistic(std::string name, ObjectId id, SimObject *parent,
            StatisticKind kind)
      : SimObject(ObjectKind::Statistic, std::move(name), id, parent),
        kind_(kind), valid_(kind != StatisticKind::Histogram) {}

  Statistic(std::string name, ObjectId id, SimObject *parent,
            std::initializer_list<uint64_t> bounds)
      : Statistic(std::move(name), id, parent, std::vector<uint64_t>(bounds)) {}

  Statistic(std::string name, ObjectId id, SimObject *parent,
            std::vector<uint64_t> bounds)
      : SimObject(ObjectKind::Statistic, std::move(name), id, parent),
        kind_(StatisticKind::Histogram), bounds_(std::move(bounds)),
        bucketCounts_(bounds_.size() + 1),
        proposedBucketCounts_(bounds_.size() + 1),
        valid_(std::adjacent_find(bounds_.begin(), bounds_.end(),
                                  std::greater_equal<>()) == bounds_.end()) {}

  StatisticKind statisticKind() const { return kind_; }

  bool proposeAdd(uint64_t amount = 1) {
    if (!valid_ || kind_ != StatisticKind::Counter ||
        amount > maxValue() - proposedValue_ ||
        proposedValue_ + amount > maxValue() - value_)
      return false;
    proposedValue_ += amount;
    pending_ = true;
    return true;
  }

  bool proposeSet(uint64_t value) {
    if (!valid_ || kind_ != StatisticKind::Gauge || pending_)
      return false;
    proposedValue_ = value;
    pending_ = true;
    return true;
  }

  bool proposeObserve(uint64_t value) {
    if (!valid_ || kind_ != StatisticKind::Histogram ||
        proposedCount_ >= maxValue() - count_ ||
        value > maxValue() - proposedSum_ ||
        proposedSum_ + value > maxValue() - sum_)
      return false;
    size_t bucket = static_cast<size_t>(
        std::lower_bound(bounds_.begin(), bounds_.end(), value) -
        bounds_.begin());
    if (proposedBucketCounts_[bucket] >= maxValue() - bucketCounts_[bucket])
      return false;
    ++proposedBucketCounts_[bucket];
    ++proposedCount_;
    proposedSum_ += value;
    if (!proposedMinimum_ || value < *proposedMinimum_)
      proposedMinimum_ = value;
    if (!proposedMaximum_ || value > *proposedMaximum_)
      proposedMaximum_ = value;
    pending_ = true;
    return true;
  }

  void doXfer(Epoch epoch) override {
    if (!pending_)
      return;
    if (kind_ == StatisticKind::Counter)
      value_ += proposedValue_;
    else if (kind_ == StatisticKind::Gauge)
      value_ = proposedValue_;
    else {
      count_ += proposedCount_;
      sum_ += proposedSum_;
      if (proposedMinimum_ && (!minimum_ || *proposedMinimum_ < *minimum_))
        minimum_ = proposedMinimum_;
      if (proposedMaximum_ && (!maximum_ || *proposedMaximum_ > *maximum_))
        maximum_ = proposedMaximum_;
      for (size_t index = 0; index < bucketCounts_.size(); ++index)
        bucketCounts_[index] += proposedBucketCounts_[index];
    }
    lastUpdate_ = epoch;
    clearProposals();
  }

  bool hasPendingCommit() const override { return pending_; }
  bool validate() const { return valid_; }

  StatSnapshot snapshot() const {
    StatSnapshot result{.name = std::string(name()),
                        .objectPath = std::string(path()),
                        .kind = kind_,
                        .value =
                            kind_ == StatisticKind::Histogram ? count_ : value_,
                        .count = count_,
                        .sum = sum_,
                        .minimum = minimum_.value_or(0),
                        .maximum = maximum_.value_or(0),
                        .lastUpdate = lastUpdate_};
    if (kind_ == StatisticKind::Histogram) {
      result.buckets.reserve(bucketCounts_.size());
      for (size_t index = 0; index < bounds_.size(); ++index)
        result.buckets.push_back({bounds_[index], bucketCounts_[index]});
      result.buckets.push_back({maxValue(), bucketCounts_.back()});
    }
    return result;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back(snapshot());
  }

  void reset() override {
    value_ = 0;
    count_ = 0;
    sum_ = 0;
    minimum_.reset();
    maximum_.reset();
    std::fill(bucketCounts_.begin(), bucketCounts_.end(), 0);
    lastUpdate_ = {};
    clearProposals();
  }

private:
  static constexpr uint64_t maxValue() {
    return std::numeric_limits<uint64_t>::max();
  }

  void clearProposals() {
    proposedValue_ = 0;
    proposedCount_ = 0;
    proposedSum_ = 0;
    proposedMinimum_.reset();
    proposedMaximum_.reset();
    std::fill(proposedBucketCounts_.begin(), proposedBucketCounts_.end(), 0);
    pending_ = false;
  }

  StatisticKind kind_;
  uint64_t value_ = 0;
  uint64_t count_ = 0;
  uint64_t sum_ = 0;
  std::optional<uint64_t> minimum_;
  std::optional<uint64_t> maximum_;
  std::vector<uint64_t> bounds_;
  std::vector<uint64_t> bucketCounts_;
  Epoch lastUpdate_;
  bool pending_ = false;
  uint64_t proposedValue_ = 0;
  uint64_t proposedCount_ = 0;
  uint64_t proposedSum_ = 0;
  std::optional<uint64_t> proposedMinimum_;
  std::optional<uint64_t> proposedMaximum_;
  std::vector<uint64_t> proposedBucketCounts_;
  bool valid_ = true;
};

} // namespace gfsim

#endif // GFSIM_STATISTICS_H
