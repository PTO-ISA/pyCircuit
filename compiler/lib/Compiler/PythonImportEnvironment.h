#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTENVIRONMENT_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTENVIRONMENT_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

#include <memory>
#include <utility>

namespace acir::compiler::detail {

// Immutable shared history plus a small local overlay. Forking an environment
// commits the overlay into a frame and copies only O(1) state, so branch arms
// share the entries they never touch instead of re-copying them; joins then
// merge only the per-arm deltas. Budget charging follows the same shape:
// slots are reserved for entries that are actually committed or merged.
//
// Work attribution (each real operation is billed exactly once):
//   - moving overlay entries into a frame at commit is pre-paid by the fork
//     site's branch-phase reservation, which charges the pending entries;
//   - flattening the frame chain is recorded in `unbilledWork` and billed by
//     the next branch/join copy reservation that drains it. Copies made by
//     fork() start with a clean counter so sibling arms never re-bill the
//     same pending work; moves transfer the counter unchanged.
template <typename T> class EnvironmentMap {
public:
  EnvironmentMap() = default;

  // Fork copies share the committed frames and clone the overlay, but they do
  // not inherit pending unbilled work: the fork point settles that itself.
  EnvironmentMap(const EnvironmentMap &other)
      : frame(other.frame), overlay(other.overlay), hidden(other.hidden) {}
  EnvironmentMap &operator=(const EnvironmentMap &other) {
    frame = other.frame;
    overlay = other.overlay;
    hidden = other.hidden;
    return *this;
  }
  // Move operations are load-bearing: declaring the copy forms suppresses the
  // implicit moves, which would silently deep-copy overlays at every
  // std::move of a BranchEnvironment. Moves keep ownership of the pending
  // billing so it is still settled exactly once.
  EnvironmentMap(EnvironmentMap &&) = default;
  EnvironmentMap &operator=(EnvironmentMap &&) = default;

  struct Frame {
    std::shared_ptr<const Frame> parent;
    llvm::StringMap<T> entries;
    llvm::StringSet<> hidden; // names erased relative to the parent frame
  };

  // Freezes the local overlay into an immutable shared frame. Later forks
  // alias the frame instead of copying its entries.
  void commit() {
    if (overlay.empty() && hidden.empty())
      return;
    constexpr unsigned flattenDepth = 64;
    if (frameDepth() >= flattenDepth)
      flattenFrames();
    auto next = std::make_shared<Frame>();
    next->parent = frame;
    next->entries = std::move(overlay);
    next->hidden = std::move(hidden);
    frame = std::move(next);
  }

  // A forkable copy of this map: current deltas published, arms stay O(1).
  EnvironmentMap fork() {
    commit();
    return *this;
  }

  // Real container work (frame flattening) performed since the last charge,
  // so budgets can bill work the reservation paths did not see. Draining is
  // const: charging sites only observe const environments.
  uint64_t takeUnbilledWork() const {
    uint64_t work = unbilledWork;
    unbilledWork = 0;
    return work;
  }

  // lookupPtr replaces StringMap::find/end pairs: a null result is a miss.
  const T *lookupPtr(llvm::StringRef name) const {
    auto local = overlay.find(name);
    if (local != overlay.end())
      return &local->second;
    if (hidden.contains(name))
      return nullptr;
    for (const Frame *f = frame.get(); f; f = f->parent.get()) {
      auto hit = f->entries.find(name);
      if (hit != f->entries.end())
        return &hit->second;
      if (f->hidden.contains(name))
        return nullptr;
    }
    return nullptr;
  }
  // StringMap-compatible by-value lookup (default-constructed on a miss).
  T lookup(llvm::StringRef name) const {
    const T *hit = lookupPtr(name);
    return hit ? *hit : T();
  }
  bool contains(llvm::StringRef name) const { return lookupPtr(name) != nullptr; }

  // Writes always land in the local overlay, shadowing the frames, and cancel
  // any local erase so lookupPtr, contains, size and materialize agree.
  // Reading through operator[] first imports the shared entry, matching
  // StringMap's "return the existing value" behavior -- except after a local
  // erase: undoing a delete starts from the default value, it does not
  // resurrect the older frame entry the erase had hidden.
  T &operator[](llvm::StringRef name) {
    bool wasHidden = hidden.contains(name);
    hidden.erase(name);
    auto existing = overlay.find(name);
    if (existing != overlay.end())
      return existing->second;
    if (!wasHidden) {
      for (const Frame *f = frame.get(); f; f = f->parent.get()) {
        auto hit = f->entries.find(name);
        if (hit != f->entries.end()) {
          overlay.insert({name, hit->second});
          return overlay.find(name)->second;
        }
        if (f->hidden.contains(name))
          break;
      }
    }
    return overlay[name];
  }
  // StringMap-compatible conditional insert. A locally erased name counts as
  // absent, so the caller's value is inserted rather than the older frame
  // entry re-imported.
  template <typename... Args>
  T &try_emplace(llvm::StringRef name, Args &&...args) {
    bool wasHidden = hidden.contains(name);
    hidden.erase(name);
    auto existing = overlay.find(name);
    if (existing != overlay.end())
      return existing->second;
    if (!wasHidden && lookupPtr(name))
      return operator[](name);
    auto inserted = overlay.try_emplace(name, std::forward<Args>(args)...);
    return inserted.first->second;
  }
  // Erasing must keep the name invisible even when a parent frame still
  // carries an older entry, so removal records a tombstone whenever the name
  // is reachable through the frames, not only when it lived in the overlay.
  bool erase(llvm::StringRef name) {
    bool throughFrames = visibleThroughFrames(name);
    bool removed = overlay.erase(name);
    if (throughFrames)
      hidden.insert(name);
    else
      hidden.erase(name);
    return removed || throughFrames;
  }

  // Names added, changed or erased locally since the last commit. This is the
  // delta a fork publishes and a join has to merge. Overlay names that are
  // also hidden cannot occur (writes cancel erases) but are skipped anyway so
  // the delta never lists a name twice.
  llvm::SmallVector<llvm::StringRef> pendingNames() const {
    llvm::SmallVector<llvm::StringRef> names;
    for (const auto &entry : overlay)
      if (!hidden.contains(entry.first()))
        names.push_back(entry.first());
    for (const auto &entry : hidden)
      names.push_back(entry.first());
    return names;
  }

  // Entries added or changed locally (erased names are reported by
  // pendingNames instead because they carry no payload to charge).
  llvm::SmallVector<llvm::StringRef> pendingEntryNames() const {
    llvm::SmallVector<llvm::StringRef> names;
    for (const auto &entry : overlay)
      names.push_back(entry.first());
    return names;
  }

  // Appends the names added, changed or erased since the state shared with
  // `reference`: every frame this map holds above the deepest frame common
  // with `reference`, plus the local overlay and tombstones. Nested forks
  // inside a branch arm commit arm-local writes into frames the sibling never
  // sees, so a join delta must be measured from the shared fork point, not
  // from the last local commit; frame identity (not content) keeps this O(1)
  // per unchanged level.
  void appendDeltaSince(const EnvironmentMap<T> &reference,
                        llvm::SmallVectorImpl<llvm::StringRef> &names) const {
    llvm::SmallPtrSet<const void *, 8> shared;
    for (const Frame *f = reference.frame.get(); f; f = f->parent.get())
      shared.insert(f);
    for (const Frame *f = frame.get(); f && !shared.contains(f);
         f = f->parent.get()) {
      for (const auto &entry : f->entries)
        if (!llvm::is_contained(names, entry.first()))
          names.push_back(entry.first());
      for (const auto &erased : f->hidden)
        if (!llvm::is_contained(names, erased.first()))
          names.push_back(erased.first());
    }
    for (const auto &entry : overlay)
      if (!llvm::is_contained(names, entry.first()))
        names.push_back(entry.first());
    for (const auto &erased : hidden)
      if (!llvm::is_contained(names, erased.first()))
        names.push_back(erased.first());
  }

  // Visible snapshot as a plain map, oldest state first so newer entries win.
  // Assignment (not insert) is load-bearing: StringMap::insert keeps the
  // existing entry, which would freeze stale frame values over newer ones.
  llvm::StringMap<T> materialize() const {
    llvm::SmallVector<const Frame *> chain;
    for (const Frame *f = frame.get(); f; f = f->parent.get())
      chain.push_back(f);
    llvm::StringMap<T> flat;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      for (const auto &entry : (*it)->entries)
        flat[entry.first()] = entry.second;
      for (const auto &erased : (*it)->hidden)
        flat.erase(erased.first());
    }
    for (const auto &entry : overlay)
      flat[entry.first()] = entry.second;
    for (const auto &erased : hidden)
      flat.erase(erased.first());
    return flat;
  }

  size_t size() const { return materialize().size(); }
  bool empty() const { return size() == 0; }

  // Exchanges the whole visible state with a plain map. The importer only
  // swaps maps into and out of freshly constructed environments.
  void swap(llvm::StringMap<T> &other) {
    auto flat = materialize();
    overlay = std::move(flat);
    frame.reset();
    hidden.clear();
    overlay.swap(other);
  }
  // Exchanges the whole state with another overlay map (frames and the
  // pending billing counter included).
  void swap(EnvironmentMap<T> &other) {
    std::swap(frame, other.frame);
    std::swap(overlay, other.overlay);
    std::swap(hidden, other.hidden);
    std::swap(unbilledWork, other.unbilledWork);
  }

private:
  unsigned frameDepth() const {
    unsigned depth = 0;
    for (const Frame *f = frame.get(); f; f = f->parent.get())
      ++depth;
    return depth;
  }
  // Whether the name resolves through the frame chain alone (ignoring the
  // local overlay and tombstone). Each frame's erases shadow its ancestors.
  bool visibleThroughFrames(llvm::StringRef name) const {
    for (const Frame *f = frame.get(); f; f = f->parent.get()) {
      if (f->entries.contains(name))
        return true;
      if (f->hidden.contains(name))
        return false;
    }
    return false;
  }
  // Collapses the frame chain into one frame, oldest state first. Assignment
  // (not insert) is load-bearing for the same reason as in materialize():
  // StringMap::insert keeps the oldest entry and would resurrect stale values
  // once the chain is flattened. The walk is real work and is billed.
  void flattenFrames() {
    if (!frame || !frame->parent)
      return;
    llvm::SmallVector<const Frame *> chain;
    for (const Frame *f = frame.get(); f; f = f->parent.get())
      chain.push_back(f);
    auto merged = std::make_shared<Frame>();
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      for (const auto &entry : (*it)->entries)
        merged->entries[entry.first()] = entry.second;
      for (const auto &erased : (*it)->hidden)
        merged->entries.erase(erased.first());
    }
    unbilledWork += merged->entries.size() + 1;
    frame = std::move(merged);
  }

  std::shared_ptr<const Frame> frame;
  llvm::StringMap<T> overlay;
  llvm::StringSet<> hidden;
  // Container work not yet seen by any budget reservation. Mutable: draining
  // happens from charging sites that observe const environments.
  mutable uint64_t unbilledWork = 0;
};

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTENVIRONMENT_H
