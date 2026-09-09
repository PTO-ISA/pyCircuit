#ifndef GFSIM_SHOWCASE_H
#define GFSIM_SHOWCASE_H

#include "gfsim/components.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace gfsim {

enum class ShowcaseWorkOrder : uint8_t { Ascending, Descending, Seeded };

struct ProducerQueueConsumerPolicy {
  std::vector<uint64_t> values = {3, 5, 8};
  size_t queueCapacity = 2;
};

struct BackpressuredPipelinePolicy {
  std::vector<uint64_t> values = {21, 34};
  std::vector<Tick> readyTicks = {2, 4};
};

struct MemoryWorkItem {
  uint64_t correlationId = 0;
  size_t address = 0;
  uint64_t value = 0;
};

struct RequestResponseMemoryPolicy {
  std::vector<MemoryWorkItem> requests = {
      {.correlationId = 10, .address = 1, .value = 17},
      {.correlationId = 11, .address = 3, .value = 29}};
  size_t memoryCapacity = 4;
};

struct NestedArraysPolicy {
  std::vector<std::vector<uint64_t>> laneValues = {
      {1, 2}, {10, 20}, {100, 200}};
  size_t queueCapacity = 2;
};

struct MultiTimeDomainBridgePolicy {
  std::vector<uint64_t> values = {4, 6, 9};
  uint64_t sourcePeriod = 2;
  uint64_t targetPeriod = 3;
};

struct SuspendedProcessPolicy {
  uint64_t initialValue = 40;
  uint64_t incrementAfterWake = 2;
  Tick wakeTick = 3;
};

using ShowcasePolicy =
    std::variant<ProducerQueueConsumerPolicy, BackpressuredPipelinePolicy,
                 RequestResponseMemoryPolicy, NestedArraysPolicy,
                 MultiTimeDomainBridgePolicy, SuspendedProcessPolicy>;

struct ShowcaseHierarchyEntry {
  ObjectId id = kInvalidObjectId;
  std::string path;
  ObjectKind kind = ObjectKind::Module;
  bool operator==(const ShowcaseHierarchyEntry &) const = default;
};

struct ShowcaseResult {
  TerminationResult termination;
  std::map<std::string, uint64_t> architecturalValues;
  std::vector<ShowcaseHierarchyEntry> hierarchy;
  uint64_t completedTransactions = 0;
  std::vector<StatSnapshot> statistics;
  std::vector<CommittedEvent> events;
};

ShowcaseResult runShowcase(const ShowcasePolicy &policy,
                           ShowcaseWorkOrder order,
                           uint64_t permutationSeed = 0);

template <typename Policy>
ShowcaseResult runShowcase(const Policy &policy, ShowcaseWorkOrder order,
                           uint64_t permutationSeed = 0) {
  return runShowcase(ShowcasePolicy{policy}, order, permutationSeed);
}

/// Stable byte representation used by conformance tests and golden fixtures.
std::string canonicalShowcaseResult(const ShowcaseResult &result);

} // namespace gfsim

#endif // GFSIM_SHOWCASE_H
