#ifndef GFSIM_OBSERVATIONSLOT_H
#define GFSIM_OBSERVATIONSLOT_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>
#include <vector>

namespace gfsim {

enum class SlotValueKind : std::uint8_t { Bool, Signed, Unsigned };

struct SlotValue {
  SlotValueKind kind = SlotValueKind::Unsigned;
  std::uint64_t bits = 0;

  static constexpr SlotValue Bool(bool value) noexcept {
    return {SlotValueKind::Bool, value ? 1u : 0u};
  }
  static constexpr SlotValue Signed(std::int64_t value) noexcept {
    return {SlotValueKind::Signed, static_cast<std::uint64_t>(value)};
  }
  static constexpr SlotValue Unsigned(std::uint64_t value) noexcept {
    return {SlotValueKind::Unsigned, value};
  }

  bool operator==(const SlotValue &) const = default;
};

enum class ObservationKind : std::uint8_t { Event, Gauge };

struct ObservationDescriptor {
  std::uint32_t stableOrdinal = 0;
  std::uint64_t ownerKey = 0;
  std::uint64_t registrationKey = 0;
  std::uint64_t siteKey = 0;
  ObservationKind kind = ObservationKind::Event;

  bool operator==(const ObservationDescriptor &) const = default;
};

struct ObservationSlot {
  bool pending = false;
  SlotValue value;
  bool valid = false;
  bool path = false;
  std::uint64_t evaluationEpoch = 0;
};

struct CommittedEventSlot {
  std::uint64_t epoch = 0;
  ObservationDescriptor descriptor;
  SlotValue value;
  bool operator==(const CommittedEventSlot &) const = default;
};

struct GaugeSnapshot {
  std::uint32_t stableOrdinal = 0;
  SlotValue value = SlotValue::Unsigned(0);
  std::uint64_t lastUpdate = 0;
  bool operator==(const GaugeSnapshot &) const = default;
};

class ObservationSlots {
public:
  bool Configure(std::span<const ObservationDescriptor> descriptors,
                 std::size_t eventCapacity) {
    if (configured_)
      return false;
    for (std::size_t index = 0; index < descriptors.size(); ++index) {
      const ObservationDescriptor &current = descriptors[index];
      auto currentKey = key(current);
      if (index != 0 && !(key(descriptors[index - 1]) < currentKey))
        return false;
      for (std::size_t previous = 0; previous < index; ++previous)
        if (descriptors[previous].stableOrdinal == current.stableOrdinal)
          return false;
    }

    descriptors_.assign(descriptors.begin(), descriptors.end());
    slots_.resize(descriptors_.size());
    gaugeIndexBySlot_.assign(descriptors_.size(), kNoGauge);
    gauges_.reserve(descriptors_.size());
    for (std::size_t index = 0; index < descriptors_.size(); ++index)
      if (descriptors_[index].kind == ObservationKind::Gauge) {
        gaugeIndexBySlot_[index] = gauges_.size();
        gauges_.push_back(
            {descriptors_[index].stableOrdinal, SlotValue::Unsigned(0), 0});
      }
    eventCapacity_ = eventCapacity;
    events_.reserve(eventCapacity_);
    configured_ = true;
    return true;
  }

  bool Stage(std::size_t slot, SlotValue value, bool valid, bool path,
             std::uint64_t evaluationEpoch) noexcept {
    if (!configured_ || slot >= slots_.size() || slots_[slot].pending)
      return false;
    ObservationSlot &pending = slots_[slot];
    pending.pending = true;
    pending.value = value;
    pending.valid = valid;
    pending.path = path;
    pending.evaluationEpoch = evaluationEpoch;
    return true;
  }

  bool Check() const noexcept {
    if (!configured_)
      return false;
    std::size_t additions = 0;
    for (std::size_t index = 0; index < slots_.size(); ++index) {
      const ObservationSlot &slot = slots_[index];
      if (!slot.pending || !slot.valid || !slot.path)
        continue;
      if (descriptors_[index].kind == ObservationKind::Gauge) {
        if (slot.value.kind != SlotValueKind::Unsigned)
          return false;
      } else {
        ++additions;
      }
    }
    return additions <= eventCapacity_;
  }

  bool Precommit(std::uint64_t evaluationEpoch) const noexcept {
    if (!configured_)
      return false;

    std::size_t additions = 0;
    for (std::size_t index = 0; index < slots_.size(); ++index) {
      const ObservationSlot &slot = slots_[index];
      if (!slot.pending)
        continue;
      if (slot.evaluationEpoch != evaluationEpoch)
        return false;
      if (!slot.valid || !slot.path)
        continue;
      if (descriptors_[index].kind == ObservationKind::Gauge) {
        if (slot.value.kind != SlotValueKind::Unsigned ||
            gaugeIndexBySlot_[index] == kNoGauge)
          return false;
      } else {
        ++additions;
      }
    }
    return additions <= eventCapacity_;
  }

  void DiscardNext() noexcept {
    for (ObservationSlot &slot : slots_)
      slot = {};
  }

  void Xfer(std::uint64_t committedEpoch) noexcept {
    events_.clear();
    for (std::size_t index = 0; index < slots_.size(); ++index) {
      ObservationSlot &slot = slots_[index];
      if (slot.pending && slot.valid && slot.path &&
          slot.evaluationEpoch != std::numeric_limits<std::uint64_t>::max() &&
          slot.evaluationEpoch + 1 == committedEpoch) {
        const ObservationDescriptor &descriptor = descriptors_[index];
        if (descriptor.kind == ObservationKind::Event) {
          if (events_.size() < eventCapacity_)
            events_.push_back({committedEpoch, descriptor, slot.value});
        } else {
          GaugeSnapshot &gauge = gauges_[gaugeIndexBySlot_[index]];
          gauge.value = slot.value;
          gauge.lastUpdate = committedEpoch;
        }
      }
      slot = {};
    }
  }

  void Reset() noexcept {
    DiscardNext();
    events_.clear();
    for (GaugeSnapshot &gauge : gauges_) {
      gauge.value = SlotValue::Unsigned(0);
      gauge.lastUpdate = 0;
    }
  }

  std::span<const CommittedEventSlot> Events() const noexcept {
    return events_;
  }
  std::span<const GaugeSnapshot> Gauges() const noexcept { return gauges_; }
  std::span<const ObservationDescriptor> Descriptors() const noexcept {
    return descriptors_;
  }

private:
  static constexpr std::size_t kNoGauge =
      std::numeric_limits<std::size_t>::max();

  using DescriptorKey = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t,
                                   std::uint32_t, ObservationKind>;

  static constexpr DescriptorKey
  key(const ObservationDescriptor &descriptor) noexcept {
    return std::tuple(descriptor.ownerKey, descriptor.registrationKey,
                      descriptor.siteKey, descriptor.stableOrdinal,
                      descriptor.kind);
  }

  bool configured_ = false;
  std::size_t eventCapacity_ = 0;
  std::vector<ObservationDescriptor> descriptors_;
  std::vector<ObservationSlot> slots_;
  std::vector<std::size_t> gaugeIndexBySlot_;
  std::vector<CommittedEventSlot> events_;
  std::vector<GaugeSnapshot> gauges_;
};

} // namespace gfsim

#endif // GFSIM_OBSERVATIONSLOT_H
