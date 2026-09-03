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
  uint64_t tracePosition = 0;
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

/// Private provider component used by checked-in example workspaces. It is
/// intentionally absent from the public standard-library catalog.
class ShowcaseTraceSource final : public SimObject {
public:
  static constexpr std::string_view contractName = "workspace.Showcase";
  static constexpr ObjectKind componentKind = ObjectKind::TraceSource;

  ShowcaseTraceSource(std::string name, ObjectId id, SimObject *parent,
                      uint64_t scenario);

  bool loadDocument(PtoTraceDocument document);
  void doWork(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void reset() override;
  bool validate() const;

private:
  uint64_t scenario_ = 0;
  PtoTraceDocument document_;
  ShowcaseResult result_;
  bool loaded_ = false;
  bool pending_ = false;
  bool committed_ = false;
  Epoch lastUpdate_;
};

} // namespace gfsim

#endif // GFSIM_SHOWCASE_H
