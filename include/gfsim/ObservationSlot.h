#ifndef GFSIM_OBSERVATIONSLOT_H
#define GFSIM_OBSERVATIONSLOT_H

#include "gfsim/four_state.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>
#include <vector>

namespace gfsim {

enum class SlotValueKind : std::uint8_t { Bool, Signed, Unsigned, Bits4 };

struct SlotValue {
  SlotValueKind kind = SlotValueKind::Unsigned;
  std::uint64_t bits = 0;
  std::uint32_t width = 0;
  std::span<const std::uint64_t> valueWords{}, knownWords{}, zWords{};

  static constexpr SlotValue Bool(bool value) noexcept {
    return {SlotValueKind::Bool, value ? 1u : 0u};
  }
  static constexpr SlotValue Signed(std::int64_t value) noexcept {
    return {SlotValueKind::Signed, static_cast<std::uint64_t>(value)};
  }
  static constexpr SlotValue Unsigned(std::uint64_t value) noexcept {
    return {SlotValueKind::Unsigned, value};
  }

  static SlotValue Bits4(std::uint32_t width,
                         std::span<const std::uint64_t> value,
                         std::span<const std::uint64_t> known,
                         std::span<const std::uint64_t> z) noexcept {
    return {SlotValueKind::Bits4, 0, width, value, known, z};
  }

  bool isCanonicalBits4() const noexcept {
    const std::size_t words = width / 64 + (width % 64 != 0);
    if (kind != SlotValueKind::Bits4 || !width || valueWords.size() != words ||
        knownWords.size() != words || zWords.size() != words)
      return false;
    for (std::size_t index = 0; index < words; ++index) {
      const auto mask = index + 1 == words && width % 64 != 0
                            ? (std::uint64_t{1} << (width % 64)) - 1
                            : ~std::uint64_t{0};
      if ((knownWords[index] & zWords[index]) != 0 ||
          (valueWords[index] & ~knownWords[index]) != 0 ||
          ((valueWords[index] | knownWords[index] | zWords[index]) & ~mask) !=
              0)
        return false;
    }
    return true;
  }

  bool operator==(const SlotValue &other) const noexcept {
    if (kind != other.kind || bits != other.bits || width != other.width)
      return false;
    auto equal = [](auto left, auto right) {
      return left.size() == right.size() &&
             std::equal(left.begin(), left.end(), right.begin());
    };
    return equal(valueWords, other.valueWords) &&
           equal(knownWords, other.knownWords) && equal(zWords, other.zWords);
  }
};

enum class ObservationKind : std::uint8_t { Event, Gauge };

struct ObservationDescriptor {
  std::uint32_t stableOrdinal = 0;
  std::uint64_t ownerKey = 0;
  std::uint64_t registrationKey = 0;
  std::uint64_t siteKey = 0;
  ObservationKind kind = ObservationKind::Event;
  std::uint32_t groupIndex = 0;
  std::uint32_t groupSize = 0;
  // Zero preserves legacy scalar callers. Generated descriptors resolve the
  // exact width before Configure reserves pending and committed plane storage.
  std::uint32_t payloadWidth = 0;

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
  ObservationSlots() = default;
  ObservationSlots(const ObservationSlots &other) { *this = other; }
  ObservationSlots &operator=(const ObservationSlots &other) {
    if (this == &other)
      return *this;
    configured_ = other.configured_;
    eventCapacity_ = other.eventCapacity_;
    payloadOffsets_ = other.payloadOffsets_;
    pendingPlanes_ = other.pendingPlanes_;
    committedPlanes_ = other.committedPlanes_;
    descriptors_ = other.descriptors_;
    slots_ = other.slots_;
    gaugeIndexBySlot_ = other.gaugeIndexBySlot_;
    events_ = other.events_;
    events_.reserve(eventCapacity_);
    gauges_ = other.gauges_;
    // SlotValue borrows its owner's reserved snapshots. Rebind copied views
    // so copying a configured owner never retains another owner's buffers.
    for (std::size_t index = 0; index < slots_.size(); ++index)
      if (slots_[index].value.kind == SlotValueKind::Bits4)
        slots_[index].value =
            planeView(pendingPlanes_, index, slots_[index].value.width);
    for (auto &event : events_) {
      if (event.value.kind != SlotValueKind::Bits4)
        continue;
      auto descriptor = std::find_if(
          descriptors_.begin(), descriptors_.end(), [&](const auto &item) {
            return item.stableOrdinal == event.descriptor.stableOrdinal;
          });
      event.value =
          planeView(committedPlanes_,
                    static_cast<std::size_t>(descriptor - descriptors_.begin()),
                    event.value.width);
    }
    return *this;
  }
  ObservationSlots(ObservationSlots &&) = default;
  ObservationSlots &operator=(ObservationSlots &&) = default;

  bool Configure(std::span<const ObservationDescriptor> descriptors,
                 std::size_t eventCapacity) {
    if (configured_)
      return false;
    for (std::size_t index = 0; index < descriptors.size(); ++index) {
      const ObservationDescriptor &current = descriptors[index];
      if (current.kind != ObservationKind::Event &&
          current.kind != ObservationKind::Gauge)
        return false;
      auto currentKey = key(current);
      if (index != 0 && !(key(descriptors[index - 1]) < currentKey))
        return false;
      for (std::size_t previous = 0; previous < index; ++previous)
        if (descriptors[previous].stableOrdinal == current.stableOrdinal)
          return false;
    }
    for (std::size_t begin = 0; begin < descriptors.size();) {
      const auto &first = descriptors[begin];
      const std::size_t size = first.groupSize;
      if (first.groupIndex != 0 || size == 0 ||
          size > descriptors.size() - begin ||
          (first.kind == ObservationKind::Gauge && size != 1))
        return false;
      for (std::size_t member = 0; member < size; ++member) {
        const auto &descriptor = descriptors[begin + member];
        if (descriptor.groupIndex != member || descriptor.groupSize != size ||
            descriptor.ownerKey != first.ownerKey ||
            descriptor.registrationKey != first.registrationKey ||
            descriptor.siteKey != first.siteKey ||
            descriptor.kind != first.kind)
          return false;
      }
      begin += size;
    }

    std::size_t payloadWords = 0;
    const std::size_t capacity = std::min(
        std::vector<std::uint64_t>().max_size(),
        static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) /
            sizeof(std::uint64_t));
    for (const auto &descriptor : descriptors) {
      const std::size_t words =
          descriptor.payloadWidth / 64 + (descriptor.payloadWidth % 64 != 0);
      // Three planes in each of two reusable snapshots. Reject overflow
      // before any allocation or assignment changes the configured inventory.
      if (words > capacity / 6 - payloadWords)
        return false;
      payloadWords += words;
    }
    payloadOffsets_.reserve(descriptors.size() + 1);
    payloadOffsets_.push_back(0);
    for (const auto &descriptor : descriptors)
      payloadOffsets_.push_back(payloadOffsets_.back() +
                                descriptor.payloadWidth / 64 +
                                (descriptor.payloadWidth % 64 != 0));
    pendingPlanes_.resize(payloadWords * 3);
    committedPlanes_.resize(payloadWords * 3);
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
    if (value.kind == SlotValueKind::Bits4) {
      if (!value.isCanonicalBits4() ||
          descriptors_[slot].payloadWidth != value.width)
        return false;
      value = copyBits4(value, pendingPlanes_, slot);
    }
    ObservationSlot &pending = slots_[slot];
    pending.pending = true;
    pending.value = value;
    pending.valid = valid;
    pending.path = path;
    pending.evaluationEpoch = evaluationEpoch;
    return true;
  }

  template <unsigned Width>
  bool StagePacked(std::size_t slot, const FourState<Width> &value,
                   bool eventBool, bool valid, bool path,
                   std::uint64_t evaluationEpoch) noexcept {
    if (!configured_ || slot >= slots_.size() || slots_[slot].pending ||
        descriptors_[slot].payloadWidth != Width)
      return false;
    if constexpr (Width <= 64) {
      if (value.isFullyKnown())
        return Stage(slot,
                     eventBool ? SlotValue::Bool(value.value().toBool())
                               : SlotValue::Unsigned(value.value().value()),
                     valid, path, evaluationEpoch);
    }
    auto packed = planeView(pendingPlanes_, slot, Width);
    const std::size_t words = packed.valueWords.size();
    const auto offset = 3 * payloadOffsets_[slot];
    for (std::size_t index = 0; index < words; ++index) {
      const auto known = value.knownMask().word(index);
      pendingPlanes_[offset + index] = value.value().word(index) & known;
      pendingPlanes_[offset + words + index] = known;
      pendingPlanes_[offset + 2 * words + index] = value.zMask().word(index);
    }
    return Stage(slot, packed, valid, path, evaluationEpoch);
  }

  bool Check() const noexcept { return checkGroups(nullptr); }

  bool Precommit(std::uint64_t evaluationEpoch) const noexcept {
    return checkGroups(&evaluationEpoch);
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
          if (events_.size() < eventCapacity_) {
            auto value = slot.value.kind == SlotValueKind::Bits4
                             ? copyBits4(slot.value, committedPlanes_, index)
                             : slot.value;
            events_.push_back({committedEpoch, descriptor, value});
          }
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
  SlotValue planeView(std::vector<std::uint64_t> &storage, std::size_t slot,
                      std::uint32_t width) noexcept {
    const auto words = payloadOffsets_[slot + 1] - payloadOffsets_[slot];
    auto *start = storage.data() + 3 * payloadOffsets_[slot];
    return SlotValue::Bits4(width, {start, words}, {start + words, words},
                            {start + 2 * words, words});
  }

  SlotValue copyBits4(const SlotValue &value,
                      std::vector<std::uint64_t> &storage,
                      std::size_t slot) noexcept {
    const auto words = value.valueWords.size();
    const auto offset = 3 * payloadOffsets_[slot];
    for (std::size_t index = 0; index < words; ++index) {
      storage[offset + index] = value.valueWords[index];
      storage[offset + words + index] = value.knownWords[index];
      storage[offset + 2 * words + index] = value.zWords[index];
    }
    return planeView(storage, slot, value.width);
  }

  static constexpr std::size_t kNoGauge =
      std::numeric_limits<std::size_t>::max();

  bool checkGroups(const std::uint64_t *evaluationEpoch) const noexcept {
    if (!configured_)
      return false;
    std::size_t additions = 0;
    // Configure established complete, nonempty groups and immutable boundaries.
    for (std::size_t begin = 0; begin < slots_.size();
         begin += descriptors_[begin].groupSize) {
      const auto &descriptor = descriptors_[begin];
      const std::size_t end = begin + descriptor.groupSize;
      bool anyPending = false;
      bool allPending = true;
      for (std::size_t index = begin; index < end; ++index) {
        anyPending |= slots_[index].pending;
        allPending &= slots_[index].pending;
      }
      if (!anyPending)
        continue;
      if (!allPending)
        return false;
      const auto &first = slots_[begin];
      for (std::size_t index = begin; index < end; ++index) {
        const auto &slot = slots_[index];
        if ((evaluationEpoch && slot.evaluationEpoch != *evaluationEpoch) ||
            slot.path != first.path)
          return false;
      }
      // An inactive path suppresses payload knownness differences, but cannot
      // conceal missing snapshots or an incorrect evaluation epoch.
      if (!first.path)
        continue;
      for (std::size_t index = begin; index < end; ++index)
        if (slots_[index].valid != first.valid)
          return false;
      if (!first.valid)
        continue;
      for (std::size_t index = begin; index < end; ++index) {
        const auto kind = slots_[index].value.kind;
        if (descriptor.kind == ObservationKind::Gauge) {
          if (kind != SlotValueKind::Unsigned ||
              gaugeIndexBySlot_[index] == kNoGauge)
            return false;
        } else {
          if (kind != SlotValueKind::Bool && kind != SlotValueKind::Signed &&
              kind != SlotValueKind::Unsigned &&
              (kind != SlotValueKind::Bits4 ||
               !slots_[index].value.isCanonicalBits4()))
            return false;
          if (additions == eventCapacity_)
            return false;
          ++additions;
        }
      }
    }
    return true;
  }

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
  std::vector<std::size_t> payloadOffsets_;
  std::vector<std::uint64_t> pendingPlanes_, committedPlanes_;
  std::vector<ObservationDescriptor> descriptors_;
  std::vector<ObservationSlot> slots_;
  std::vector<std::size_t> gaugeIndexBySlot_;
  std::vector<CommittedEventSlot> events_;
  std::vector<GaugeSnapshot> gauges_;
};

} // namespace gfsim

#endif // GFSIM_OBSERVATIONSLOT_H
