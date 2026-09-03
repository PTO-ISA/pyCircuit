#ifndef GFSIM_NPU_H
#define GFSIM_NPU_H

#include "gfsim/components.h"
#include "gfsim/observation.h"
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

struct NpuDependency {
  uint64_t producerSequenceId = 0;
  std::string tileIdentity;
  uint64_t flowId = 0;

  bool operator==(const NpuDependency &) const = default;
};

struct NpuIssueEntry {
  NpuInstruction instruction;
  ObjectId stableObjectId = kInvalidObjectId;
  std::vector<NpuDependency> derivedDependencies;

  bool operator==(const NpuIssueEntry &) const = default;
};

/// Block-local tile rename state and four finite deterministic issue queues.
class NpuDependencyTracker final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.npu.DependencyTracker";
  static constexpr ObjectKind componentKind = ObjectKind::Scheduler;

  NpuDependencyTracker(std::string name, ObjectId id, SimObject *parent,
                       NpuIssueQueueCapacities capacities,
                       ObservationSink *observations = nullptr);

  bool proposeDispatch(const NpuInstruction &instruction,
                       ObjectId stableObjectId);
  bool proposeIssue(NpuEngineClass engine);
  bool proposeComplete(uint64_t sequenceId);

  void doArbitrate(Epoch epoch) override;
  void doXfer(Epoch epoch) override;
  bool hasPendingCommit() const override;
  bool isRunnable(Epoch epoch) const override;
  RuntimeObjectState runtimeState(Epoch epoch) const override;
  void collectStatistics(std::vector<StatSnapshot> &out) const override;
  void reset() override;

  const NpuIssueEntry *proposedIssue(NpuEngineClass engine) const;
  const std::vector<NpuIssueEntry> &issued() const { return issued_; }
  std::vector<NpuIssueEntry> queued(NpuEngineClass engine) const;
  std::vector<NpuDependency> dependencies(uint64_t sequenceId) const;
  bool isReady(uint64_t sequenceId) const;
  bool dispatchAccepted(uint64_t sequenceId) const;
  size_t queueSize(NpuEngineClass engine) const;
  const std::vector<uint64_t> &rejectedDispatches() const {
    return rejectedDispatches_;
  }

private:
  using TileKey = std::pair<uint64_t, std::string>;

  struct DispatchProposal {
    NpuInstruction instruction;
    ObjectId stableObjectId = kInvalidObjectId;
  };

  static size_t engineIndex(NpuEngineClass engine);
  size_t capacity(NpuEngineClass engine) const;
  bool knownSequence(uint64_t sequenceId) const;
  bool ready(const NpuIssueEntry &entry) const;

  NpuIssueQueueCapacities capacities_;
  std::array<std::vector<NpuIssueEntry>, 4> queues_;
  std::vector<DispatchProposal> dispatchProposals_;
  std::vector<NpuIssueEntry> acceptedDispatches_;
  std::vector<uint64_t> proposedRejectedDispatches_;
  std::vector<uint64_t> rejectedDispatches_;
  std::set<uint64_t> acceptedDispatchSequences_;
  std::array<std::optional<NpuIssueEntry>, 4> issueProposals_;
  std::vector<NpuIssueEntry> issued_;
  std::vector<uint64_t> completionProposals_;
  std::set<uint64_t> outstandingSequences_;
  std::set<uint64_t> completedSequences_;
  std::map<TileKey, uint64_t> producers_;
  std::set<uint64_t> usedFlowIds_;
  std::optional<uint64_t> lastDispatchedSequence_;
  std::array<size_t, 4> highWatermarks_{};
  uint64_t totalDispatches_ = 0;
  uint64_t totalDispatchStalls_ = 0;
  uint64_t totalIssues_ = 0;
  uint64_t totalDependencyWakeups_ = 0;
  Epoch lastUpdate_;
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
