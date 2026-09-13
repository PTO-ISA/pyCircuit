#ifndef ACIR_ANALYSIS_VARIABLEANALYSISTESTHOOKS_H
#define ACIR_ANALYSIS_VARIABLEANALYSISTESTHOOKS_H

#include <cstdint>

namespace acir::detail {

/// Test-only accounting for distinct snapshot traversal contexts. Production
/// execution leaves the thread-local pointer null and pays one guarded check
/// for each context that is actually explored.
struct SnapshotTraversalWork {
  uint64_t contexts = 0;
};

inline thread_local SnapshotTraversalWork *activeSnapshotTraversalWork =
    nullptr;

class ScopedSnapshotTraversalWorkRecorder {
public:
  explicit ScopedSnapshotTraversalWorkRecorder(SnapshotTraversalWork &work)
      : previous(activeSnapshotTraversalWork) {
    work = {};
    activeSnapshotTraversalWork = &work;
  }

  ~ScopedSnapshotTraversalWorkRecorder() {
    activeSnapshotTraversalWork = previous;
  }

  ScopedSnapshotTraversalWorkRecorder(
      const ScopedSnapshotTraversalWorkRecorder &) = delete;
  ScopedSnapshotTraversalWorkRecorder &
  operator=(const ScopedSnapshotTraversalWorkRecorder &) = delete;

private:
  SnapshotTraversalWork *previous;
};

inline void accountSnapshotTraversalContext() {
  if (activeSnapshotTraversalWork)
    ++activeSnapshotTraversalWork->contexts;
}

} // namespace acir::detail

#endif // ACIR_ANALYSIS_VARIABLEANALYSISTESTHOOKS_H
