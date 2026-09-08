#ifndef GFSIM_NPU_H
#define GFSIM_NPU_H

#include "gfsim/components.h"
#include "gfsim/observation.h"
#include "gfsim/queue.h"
#include "gfsim/trace.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gfsim {

enum class NpuEngineClass : uint8_t {
  Scalar,
  Vector,
  Cube,
  Tma,
};

struct NpuTimestamps {
  std::optional<uint64_t> decoded;
  std::optional<uint64_t> dispatched;
  std::optional<uint64_t> issued;
  std::optional<uint64_t> completed;
  std::optional<uint64_t> retired;

  bool operator==(const NpuTimestamps &) const = default;
};

struct NpuScalarImmediate {
  std::string type;
  PtoValue value;

  bool operator==(const NpuScalarImmediate &) const = default;
};

struct NpuTileDescriptor {
  std::string identity;
  std::string address;
  std::string type;
  std::string layout;
  PtoValue::Array shape;

  bool operator==(const NpuTileDescriptor &) const = default;
};

struct NpuInstruction {
  uint64_t sequenceId = 0;
  uint64_t blockId = 0;
  std::string opcode;
  std::vector<uint64_t> dependencies;
  std::vector<PtoTraceOperand> operands;
  std::vector<std::string> inputTiles;
  std::vector<std::string> outputTiles;
  std::vector<NpuTileDescriptor> inputTileDescriptors;
  std::vector<NpuTileDescriptor> outputTileDescriptors;
  std::vector<NpuScalarImmediate> scalarInputs;
  NpuEngineClass engine = NpuEngineClass::Scalar;
  NpuTimestamps timestamps;

  bool operator==(const NpuInstruction &) const = default;
};

struct NpuDecodeDiagnostic {
  std::string code;
  std::string message;

  bool operator==(const NpuDecodeDiagnostic &) const = default;
};

struct NpuDecodeResult {
  std::optional<NpuInstruction> instruction;
  std::vector<NpuDecodeDiagnostic> diagnostics;
  std::vector<EventProposal> observations;

  bool succeeded() const {
    return instruction.has_value() && diagnostics.empty();
  }
  std::string_view primaryDiagnostic() const {
    return diagnostics.empty() ? std::string_view{} : diagnostics.front().code;
  }
};

class NpuDecoder {
public:
  NpuDecodeResult decode(const PtoTraceRecord &record) const;
  std::optional<NpuInstruction> operator()(const PtoTraceRecord &record) const {
    return decode(record).instruction;
  }
};

std::string_view toString(NpuEngineClass engine);

struct NpuIssueQueueCapacities {
  size_t scalar = 0;
  size_t vector = 0;
  size_t cube = 0;
  size_t tma = 0;
};

struct NpuPhysicalTag {
  uint32_t tag = 0;
  uint64_t generation = 0;

  auto operator<=>(const NpuPhysicalTag &) const = default;
};

struct NpuDependency {
  uint64_t producerSequenceId = 0;
  std::string tileIdentity;
  uint64_t flowId = 0;
  uint32_t tag = 0;
  uint64_t generation = 0;

  bool operator==(const NpuDependency &) const = default;
};

struct NpuOutputRename {
  std::string tileIdentity;
  NpuPhysicalTag output;
  std::optional<NpuPhysicalTag> replaced;

  bool operator==(const NpuOutputRename &) const = default;
};

struct NpuIssueEntry {
  NpuInstruction instruction;
  ObjectId stableObjectId = kInvalidObjectId;
  std::vector<NpuDependency> derivedDependencies;
  std::vector<NpuOutputRename> outputRenames;

  bool operator==(const NpuIssueEntry &) const = default;
};

struct NpuCompletion {
  uint64_t sequenceId = 0;
  std::vector<NpuPhysicalTag> outputTags;

  bool operator==(const NpuCompletion &) const = default;
};

struct NpuDispatch {
  NpuInstruction instruction;
  ObjectId stableObjectId = kInvalidObjectId;

  bool operator==(const NpuDispatch &) const = default;
};

enum class NpuDispatchValidation : uint8_t {
  Acceptable,
  InvalidEngineClass,
  InvalidStableObjectId,
  DuplicateSequence,
  StaleSequence,
};

/// Block-local tile rename state and four finite deterministic issue queues.
class NpuDependencyTracker final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.npu.DependencyTracker";
  static constexpr ObjectKind componentKind = ObjectKind::Scheduler;

  NpuDependencyTracker(std::string name, ObjectId id, SimObject *parent,
                       NpuIssueQueueCapacities capacities,
                       ObservationSink *observations = nullptr);
  NpuDependencyTracker(std::string name, ObjectId id, SimObject *parent,
                       NpuIssueQueueCapacities capacities,
                       size_t physicalTagCapacity,
                       ObservationSink *observations = nullptr);

  bool proposeDispatch(const NpuInstruction &instruction,
                       ObjectId stableObjectId);
  NpuDispatchValidation validateDispatch(const NpuInstruction &instruction,
                                         ObjectId stableObjectId) const;
  bool proposeIssue(NpuEngineClass engine);
  bool proposeComplete(uint64_t sequenceId);
  bool proposeComplete(NpuCompletion completion);
  bool proposeRecycle(NpuPhysicalTag tag);

  void doArbitrate(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  bool isRunnable(Epoch epoch) const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void reset() override;

  const NpuIssueEntry *proposedIssue(NpuEngineClass engine) const;
  const NpuIssueEntry *oldestReadyIssue(NpuEngineClass engine) const;
  const std::vector<NpuIssueEntry> &issued() const { return issued_; }
  std::vector<NpuIssueEntry> queued(NpuEngineClass engine) const;
  std::vector<NpuDependency> dependencies(uint64_t sequenceId) const;
  bool isReady(uint64_t sequenceId) const;
  bool dispatchAccepted(uint64_t sequenceId) const;
  size_t queueSize(NpuEngineClass engine) const;
  size_t issueWindowOccupancy(NpuEngineClass engine) const;
  bool hasReadyIssue(NpuEngineClass engine) const;
  size_t physicalTagCapacity() const { return tags_.size(); }
  size_t freeTagCount() const;
  size_t liveFlowCount() const { return usedFlowIds_.size(); }
  size_t outstandingProducerCount() const { return outstanding_.size(); }
  bool tagReady(NpuPhysicalTag tag) const;
  std::optional<NpuPhysicalTag> producerTag(uint64_t blockId,
                                            std::string_view tile) const;
  bool dispatchWillCommit(uint64_t sequenceId) const;
  const std::vector<uint64_t> &rejectedDispatches() const {
    return rejectedDispatches_;
  }

private:
  friend class NpuScheduleV2;

  using TileKey = std::pair<uint64_t, std::string>;

  struct DispatchProposal {
    NpuInstruction instruction;
    ObjectId stableObjectId = kInvalidObjectId;
  };

  struct TagState {
    uint64_t generation = 0;
    uint64_t producerSequenceId = 0;
    bool allocated = false;
    bool ready = false;
  };

  static size_t engineIndex(NpuEngineClass engine);
  size_t capacity(NpuEngineClass engine) const;
  bool knownSequence(uint64_t sequenceId) const;
  bool ready(const NpuIssueEntry &entry) const;
  void cancelIssueProposals();
  void cancelAllProposals();

  struct OutstandingProducer {
    NpuEngineClass engine = NpuEngineClass::Scalar;
    std::vector<NpuPhysicalTag> outputTags;
  };

  NpuIssueQueueCapacities capacities_;
  std::array<std::vector<NpuIssueEntry>, 4> queues_;
  std::vector<DispatchProposal> dispatchProposals_;
  std::vector<NpuIssueEntry> acceptedDispatches_;
  std::vector<uint64_t> proposedRejectedDispatches_;
  std::set<uint64_t> proposedTagStalls_;
  std::map<uint64_t, NpuEngineClass> proposedWindowStalls_;
  std::vector<uint64_t> rejectedDispatches_;
  std::set<uint64_t> acceptedDispatchSequences_;
  std::array<std::optional<NpuIssueEntry>, 4> issueProposals_;
  std::vector<NpuIssueEntry> issued_;
  std::vector<NpuCompletion> completionProposals_;
  std::vector<NpuPhysicalTag> recycleProposals_;
  std::vector<NpuPhysicalTag> acceptedRecycles_;
  std::map<uint64_t, OutstandingProducer> outstanding_;
  std::map<TileKey, NpuPhysicalTag> producers_;
  std::vector<TagState> tags_;
  std::set<uint64_t> usedFlowIds_;
  std::optional<uint64_t> lastDispatchedSequence_;
  std::array<size_t, 4> highWatermarks_{};
  std::array<size_t, 4> windowHighWatermarks_{};
  std::array<uint64_t, 4> totalWindowStalls_{};
  uint64_t totalDispatches_ = 0;
  uint64_t totalDispatchStalls_ = 0;
  uint64_t totalIssues_ = 0;
  uint64_t totalDependencyWakeups_ = 0;
  uint64_t totalTagPoolStalls_ = 0;
  uint64_t totalTagRecycles_ = 0;
  Epoch lastUpdate_;
};

/// Queue-backed schedule-v2 provider. All Queue storage remains owned by the
/// parent module; this object borrows endpoints and atomically publishes one
/// dispatch, one completion/recycle, and at most one issue per engine/epoch.
/// Stale completion/recycle tokens are consumed once as observable rejects so
/// an invalid update cannot permanently block a later valid Queue token.
class NpuScheduleV2 final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.schedule.v2";
  static constexpr ObjectKind componentKind = ObjectKind::Scheduler;

  NpuScheduleV2(std::string name, ObjectId id, SimObject *parent,
                NpuIssueQueueCapacities capacities, size_t physicalTagCapacity,
                SimQueue<NpuDispatch> &dispatch,
                SimQueue<NpuCompletion> &completion,
                std::array<SimQueue<NpuIssueEntry> *, 4> issued,
                SimQueue<NpuPhysicalTag> *recycle = nullptr,
                ObservationSink *observations = nullptr);

  void doWork(Epoch epoch) override;
  void doArbitrate(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  bool isRunnable(Epoch epoch) const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void reset() override;
  bool validate() const;

  const NpuDependencyTracker &tracker() const { return tracker_; }

private:
  bool endpointsOwnedByParent() const;
  void cancelPrepared();

  NpuDependencyTracker tracker_;
  SimQueue<NpuDispatch> &dispatch_;
  SimQueue<NpuCompletion> &completion_;
  std::array<SimQueue<NpuIssueEntry> *, 4> issued_;
  SimQueue<NpuPhysicalTag> *recycle_ = nullptr;
  bool candidate_ = false;
  bool fired_ = false;
  bool completionRejected_ = false;
  bool recycleRejected_ = false;
  bool dispatchRejected_ = false;
  uint64_t totalCompletionRejects_ = 0;
  uint64_t totalRecycleRejects_ = 0;
  uint64_t totalDispatchRejects_ = 0;
  Epoch lastRejectUpdate_;
};

struct NpuExecutionConfig {
  size_t scalarUnits = 0;
  size_t vectorUnits = 0;
  size_t cubeUnits = 0;
  size_t tmaUnits = 0;
  size_t memoryRequests = 0;
  size_t scratchpadTiles = 0;
};

struct NpuMemoryRequest {
  uint64_t sequenceId = 0;
  uint64_t correlationId = 0;
  uint64_t address = 0;
  bool write = false;
  std::string tileIdentity;

  bool operator==(const NpuMemoryRequest &) const = default;
};

struct NpuMemoryResponse {
  uint64_t correlationId = 0;
  uint64_t value = 0;

  bool operator==(const NpuMemoryResponse &) const = default;
};

struct NpuArchitecturalResult {
  uint64_t retiredInstructions = 0;
  uint64_t digest = 14695981039346656037ULL;
  std::vector<uint64_t> retiredSequenceIds;

  bool operator==(const NpuArchitecturalResult &) const = default;
};

/// Finite event-driven execution, memory, completion, and retirement state.
class NpuExecutionPipeline final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.npu.ExecutionPipeline";
  static constexpr ObjectKind componentKind = ObjectKind::Compute;

  NpuExecutionPipeline(std::string name, ObjectId id, SimObject *parent,
                       NpuExecutionConfig config, SimSystem *system = nullptr,
                       ObservationSink *observations = nullptr);
  ~NpuExecutionPipeline() override;

  bool proposeAdmit(const NpuInstruction &instruction);
  bool proposeExecute(const NpuIssueEntry &entry, Epoch issueEpoch);
  bool proposeTraceExhausted();

  void doWork(Epoch epoch) override;
  void doArbitrate(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  bool isRunnable(Epoch epoch) const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void bindSystem(SimSystem *system) override;
  void reset() override;

  static uint64_t executionLatency(const NpuInstruction &instruction);
  Epoch completionEpoch(uint64_t sequenceId) const;
  bool canAccept(NpuEngineClass engine) const;
  size_t activeExecutions(NpuEngineClass engine) const;
  const std::vector<NpuIssueEntry> &completed() const;
  const std::vector<NpuInstruction> &retired() const;
  const std::vector<NpuMemoryRequest> &memoryRequests() const;
  const std::vector<NpuMemoryResponse> &memoryResponses() const;
  bool scratchpadContains(std::string_view tileIdentity) const;
  std::optional<uint64_t> globalMemoryValue(uint64_t address) const;
  const NpuArchitecturalResult &architecturalResult() const;
  bool complete() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/// Trace-owning provider component for the checked-in hierarchical NPU model.
class NpuTraceSource final : public SimObject {
public:
  static constexpr std::string_view contractName = "workspace.Npu";
  static constexpr ObjectKind componentKind = ObjectKind::TraceSource;

  NpuTraceSource(std::string name, ObjectId id, SimObject *parent,
                 ObservationSink *observations = nullptr);

  bool loadDocument(PtoTraceDocument document);
  void doWork(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void reset() override;
  bool validate() const;

private:
  PtoTraceDocument document_;
  NpuArchitecturalResult result_;
  uint64_t eventCount_ = 0;
  bool loaded_ = false;
  bool pending_ = false;
  bool committed_ = false;
  Epoch lastUpdate_;
};

/// Quiescent structural marker used to preserve the generated NPU hierarchy.
class NpuNode final : public SimObject {
public:
  static constexpr std::string_view contractName = "workspace.NpuNode";
  static constexpr ObjectKind componentKind = ObjectKind::Compute;

  NpuNode(std::string name, ObjectId id, SimObject *parent)
      : SimObject(ObjectKind::Compute, std::move(name), id, parent) {}

  RuntimeObjectState runtimeState(Epoch) const override {
    return {.quiescent = true};
  }
  bool validate() const { return true; }
};

} // namespace gfsim

#endif // GFSIM_NPU_H
