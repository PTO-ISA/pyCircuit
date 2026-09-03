#ifndef ACIR_ANALYSIS_MODELANALYSISTESTHOOKS_H
#define ACIR_ANALYSIS_MODELANALYSISTESTHOOKS_H

#include "acir/Analysis/ModelAnalysis.h"

#include <cstdint>

namespace acir::detail {

/// Test-only accounting for logical indexed work performed by the complete
/// topology-freeze path. Production execution leaves the thread-local pointer
/// null and pays only the guarded null checks at accounting sites.
struct FreezeWork {
  uint64_t stateIndexInsertions = 0;
  uint64_t topologyIndexLookups = 0;
  uint64_t manifestIndexInsertions = 0;
  uint64_t manifestOwnerLookups = 0;
  uint64_t declarationIndexInsertions = 0;
  uint64_t declarationLookups = 0;

  uint64_t total() const {
    return stateIndexInsertions + topologyIndexLookups +
           manifestIndexInsertions + manifestOwnerLookups +
           declarationIndexInsertions + declarationLookups;
  }
};

inline thread_local FreezeWork *activeFreezeWork = nullptr;

class ScopedFreezeWorkRecorder {
public:
  explicit ScopedFreezeWorkRecorder(FreezeWork &work)
      : previous(activeFreezeWork) {
    work = {};
    activeFreezeWork = &work;
  }

  ~ScopedFreezeWorkRecorder() { activeFreezeWork = previous; }

  ScopedFreezeWorkRecorder(const ScopedFreezeWorkRecorder &) = delete;
  ScopedFreezeWorkRecorder &
  operator=(const ScopedFreezeWorkRecorder &) = delete;

private:
  FreezeWork *previous;
};

struct ProcessSkeletonLimits {
  uint64_t nodes;
  uint64_t edges;
};

inline thread_local const ProcessSkeletonLimits *activeProcessSkeletonLimits =
    nullptr;

class ScopedProcessSkeletonLimits {
public:
  ScopedProcessSkeletonLimits(uint64_t nodes, uint64_t edges)
      : limits{nodes, edges}, previous(activeProcessSkeletonLimits) {
    activeProcessSkeletonLimits = &limits;
  }

  ~ScopedProcessSkeletonLimits() { activeProcessSkeletonLimits = previous; }

  ScopedProcessSkeletonLimits(const ScopedProcessSkeletonLimits &) = delete;
  ScopedProcessSkeletonLimits &
  operator=(const ScopedProcessSkeletonLimits &) = delete;

private:
  ProcessSkeletonLimits limits;
  const ProcessSkeletonLimits *previous;
};

inline uint64_t processSkeletonNodeLimit() {
  return activeProcessSkeletonLimits ? activeProcessSkeletonLimits->nodes
                                     : kMaxModelAnalysisNodes;
}

inline uint64_t processSkeletonEdgeLimit() {
  return activeProcessSkeletonLimits ? activeProcessSkeletonLimits->edges
                                     : kMaxModelAnalysisEdges;
}

} // namespace acir::detail

#endif // ACIR_ANALYSIS_MODELANALYSISTESTHOOKS_H
