#ifndef GFSIM_DISPATCH_H
#define GFSIM_DISPATCH_H

#include "gfsim/core.h"

#include <concepts>
#include <cstddef>
#include <limits>
#include <span>

namespace gfsim {

class SimObject;

/// The two invocations of the generated Xfer thunk preserve the global
/// arbitration/Xfer barrier without adding a second dispatch entry point.
enum class XferPhase : uint8_t { Arbitrate, Probe, Commit };

using WorkThunk = void (*)(void *, Epoch);
using XferThunk = bool (*)(void *, Epoch, XferPhase);
using ResetThunk = void (*)(void *);
using ValidateThunk = bool (*)(const void *, ObjectId, ObjectKind);

/// One generated row per runtime object. Rows are indexed by their dense,
/// stable ObjectId; the object pointer is recovered only by typed thunks.
struct DispatchRow {
  ObjectId id = kInvalidObjectId;
  ObjectKind kind = ObjectKind::Module;
  void *object = nullptr;
  WorkThunk work = nullptr;
  XferThunk xfer = nullptr;
  ResetThunk reset = nullptr;
  ValidateThunk validate = nullptr;
};

template <typename T>
concept DispatchObject =
    std::derived_from<T, SimObject> &&
    requires(T &object, const T &constObject, Epoch epoch) {
      { constObject.id() } -> std::convertible_to<ObjectId>;
      { constObject.kind() } -> std::same_as<ObjectKind>;
      { constObject.hasPendingCommit() } -> std::same_as<bool>;
      object.doWork(epoch);
      object.doArbitrate(epoch);
      object.doXfer(epoch);
      object.reset();
    };

template <DispatchObject T> DispatchRow makeDispatchRow(T *object) {
  return DispatchRow{
      .id = object ? object->id() : kInvalidObjectId,
      .kind = object ? object->kind() : ObjectKind::Module,
      .object = static_cast<SimObject *>(object),
      .work =
          [](void *storage, Epoch epoch) {
            static_cast<T *>(static_cast<SimObject *>(storage))
                ->T::doWork(epoch);
          },
      .xfer =
          [](void *storage, Epoch epoch, XferPhase phase) {
            T *typed = static_cast<T *>(static_cast<SimObject *>(storage));
            if (phase == XferPhase::Arbitrate) {
              typed->T::doArbitrate(epoch);
              return false;
            }
            bool committed = typed->T::hasPendingCommit();
            if (phase == XferPhase::Probe)
              return committed;
            typed->T::doXfer(epoch);
            return committed;
          },
      .reset =
          [](void *storage) {
            static_cast<T *>(static_cast<SimObject *>(storage))->T::reset();
          },
      .validate =
          [](const void *storage, ObjectId id, ObjectKind kind) {
            const T *typed =
                static_cast<const T *>(static_cast<const SimObject *>(storage));
            if (typed->id() != id || typed->kind() != kind)
              return false;
            if constexpr (requires(const T &typed) {
                            { typed.validate() } -> std::convertible_to<bool>;
                          })
              return typed->T::validate();
            return true;
          },
  };
}

/// Non-owning view of the generated static table. Generated storage has static
/// lifetime; tests may provide an array whose lifetime encloses the system.
class DispatchTable {
public:
  DispatchTable() = default;
  explicit DispatchTable(std::span<const DispatchRow> rows) : rows_(rows) {}

  size_t size() const { return rows_.size(); }
  bool empty() const { return rows_.empty(); }

  const DispatchRow *lookup(ObjectId id) const {
    if (id >= rows_.size())
      return nullptr;
    return &rows_[id];
  }

  bool validate() const {
    for (size_t index = 0; index < rows_.size(); ++index) {
      const DispatchRow &row = rows_[index];
      if (row.id != index || !row.object || !row.work || !row.xfer ||
          !row.reset || !row.validate ||
          !row.validate(row.object, row.id, row.kind))
        return false;
    }
    return true;
  }

private:
  std::span<const DispatchRow> rows_;
};

/// Canonical compressed adjacency indexed by activation-source/object ID.
class ActivationPlan {
public:
  ActivationPlan() = default;
  ActivationPlan(std::span<const uint32_t> offsets,
                 std::span<const ObjectId> targets)
      : offsets_(offsets), targets_(targets) {}

  bool empty() const { return offsets_.empty(); }

  bool validate(size_t objectCount) const {
    if (offsets_.size() != objectCount + 1 || offsets_.empty() ||
        targets_.size() > std::numeric_limits<uint32_t>::max() ||
        offsets_.front() != 0 || offsets_.back() != targets_.size())
      return false;
    for (size_t source = 0; source < objectCount; ++source) {
      uint32_t begin = offsets_[source];
      uint32_t end = offsets_[source + 1];
      if (begin > end || end > targets_.size())
        return false;
      for (uint32_t index = begin; index < end; ++index) {
        if (targets_[index] >= objectCount ||
            (index > begin && targets_[index - 1] >= targets_[index]))
          return false;
      }
    }
    return true;
  }

  std::span<const ObjectId> targetsFor(ObjectId source) const {
    if (offsets_.empty() || source >= offsets_.size() - 1)
      return {};
    return targets_.subspan(offsets_[source],
                            offsets_[source + 1] - offsets_[source]);
  }

private:
  std::span<const uint32_t> offsets_;
  std::span<const ObjectId> targets_;
};

/// Compatibility dispatch ABI for the ACIR-to-C++ emitter. Its generated
/// process storage is intentionally opaque and does not derive from SimObject.
struct LegacyDispatchThunk {
  void *object = nullptr;
  void (*work)(void *object, Epoch epoch) = nullptr;
  void (*xfer)(void *object, Epoch epoch) = nullptr;
  void (*reset)(void *object) = nullptr;
  bool (*validate)(void *object) = nullptr;
};

struct LegacyActivationGraph {
  const uint32_t *offsets = nullptr;
  const uint32_t *targets = nullptr;
  uint32_t sourceCount = 0;
};

struct LegacyDispatchTable {
  const LegacyDispatchThunk *rows = nullptr;
  uint32_t objectCount = 0;
};

} // namespace gfsim

#endif // GFSIM_DISPATCH_H
