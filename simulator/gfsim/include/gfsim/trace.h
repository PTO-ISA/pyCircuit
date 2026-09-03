#ifndef GFSIM_TRACE_H
#define GFSIM_TRACE_H

#include "gfsim/object.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace gfsim {

struct PtoValue {
  using Array = std::vector<PtoValue>;
  using Object = std::map<std::string, PtoValue>;
  using Storage = std::variant<std::monostate, bool, int64_t, uint64_t, double,
                               std::string, Array, Object>;
  Storage value;

  bool operator==(const PtoValue &) const = default;
};

enum class PtoOperandKind : uint8_t {
  Immediate,
  Buffer,
  Tile,
  Address,
  Symbol,
  RecordResult,
};

struct PtoTraceOperand {
  PtoOperandKind kind = PtoOperandKind::Immediate;
  std::string type;
  std::optional<PtoValue> immediate;
  std::string id;
  std::string addressSpace;
  uint64_t address = 0;
  uint64_t sequenceId = 0;
  uint64_t resultIndex = 0;

  bool operator==(const PtoTraceOperand &) const = default;
};

struct PtoSourceLocation {
  std::string file;
  uint64_t line = 0;
  uint64_t column = 0;

  bool operator==(const PtoSourceLocation &) const = default;
};

struct PtoTraceRecord {
  uint64_t sequenceId = 0;
  std::string opcode;
  std::vector<PtoTraceOperand> operands;
  std::vector<uint64_t> dependencies;
  PtoValue::Object attributes;
  std::optional<uint64_t> issueTime;
  std::optional<PtoSourceLocation> source;

  bool operator==(const PtoTraceRecord &) const = default;
};

struct PtoTraceMetadata {
  std::string producer;
  std::string ptoIdentity;
  std::string sourceProgram;
  std::vector<std::string> addressSpaces;
  std::string dataLayout;
  std::optional<uint64_t> recordCount;
  std::string contentHash;

  bool operator==(const PtoTraceMetadata &) const = default;
};

struct PtoTraceDocument {
  PtoTraceMetadata metadata;
  std::vector<PtoTraceRecord> records;

  PtoTraceDocument() = default;
  PtoTraceDocument(const PtoTraceDocument &) = delete;
  PtoTraceDocument &operator=(const PtoTraceDocument &) = delete;
  PtoTraceDocument(PtoTraceDocument &&) = default;
  PtoTraceDocument &operator=(PtoTraceDocument &&) = default;
  bool operator==(const PtoTraceDocument &) const = default;
};

struct TraceValidationLimits {
  size_t maxDocumentBytes = 1U << 20;
  size_t maxNestingDepth = 64;
  size_t maxStringBytes = 1U << 18;
  size_t maxRecordCount = 65536;
  size_t maxOperandsPerRecord = 1024;
  size_t maxDependenciesPerRecord = 1024;
  size_t maxAttributeMembers = 4096;
  size_t maxAggregateDecodedBytes = 1U << 24;
  size_t maxDiagnostics = 16;
};

struct TraceDiagnostic {
  std::string code;
  std::string jsonPointer;
  std::optional<uint64_t> sequenceId;
  std::string message;

  bool operator==(const TraceDiagnostic &) const = default;
};

struct TraceLoadResult {
  std::optional<PtoTraceDocument> document;
  std::vector<TraceDiagnostic> diagnostics;

  bool succeeded() const { return document.has_value() && diagnostics.empty(); }
  std::string primaryDiagnostic() const;
};

TraceLoadResult
parsePtoTrace(std::string_view input,
              const TraceValidationLimits &limits = TraceValidationLimits());

class PtoTraceStream {
public:
  explicit PtoTraceStream(
      TraceValidationLimits limits = TraceValidationLimits())
      : limits_(limits) {}

  bool append(std::string_view bytes);
  TraceLoadResult finish();

private:
  TraceValidationLimits limits_;
  std::string buffer_;
  bool finished_ = false;
  bool exceededByteLimit_ = false;
};

struct TracePosition {
  size_t nextRecordIndex = 0;
  std::optional<uint64_t> lastCommittedSequenceId;
  bool endOfTrace = false;

  bool operator==(const TracePosition &) const = default;
};

template <typename Transaction> struct IdentityTraceDecoder {
  std::optional<Transaction> operator()(const PtoTraceRecord &record) const
    requires std::constructible_from<Transaction, PtoTraceRecord>
  {
    return Transaction(record);
  }
};

template <typename Decoder, typename Transaction>
concept TraceDecoder =
    requires(const Decoder &decoder, const PtoTraceRecord &record) {
      {
        std::invoke(decoder, record)
      } -> std::same_as<std::optional<Transaction>>;
    };

/// The unique cursor owner for one validated PTO trace document.
template <typename Transaction = PtoTraceRecord,
          typename Decoder = IdentityTraceDecoder<Transaction>>
  requires TraceDecoder<Decoder, Transaction>
class TraceSource final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.TraceSource";
  static constexpr ObjectKind componentKind = ObjectKind::TraceSource;

  TraceSource(std::string name, ObjectId id, SimObject *parent,
              Decoder decoder = {}, SimSystem *system = nullptr,
              ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::TraceSource, std::move(name), id, parent,
                  observations),
        decoder_(std::move(decoder)), system_(system) {
    position_.endOfTrace = true;
  }

  TraceSource(std::string name, ObjectId id, SimObject *parent,
              PtoTraceDocument document, Decoder decoder = {},
              SimSystem *system = nullptr,
              ObservationSink *observations = nullptr)
      : SimObject(ObjectKind::TraceSource, std::move(name), id, parent,
                  observations),
        document_(std::move(document)), decoder_(std::move(decoder)),
        documentLoaded_(true), system_(system) {
    position_.endOfTrace = document_.records.empty();
  }

  bool loadDocument(PtoTraceDocument document) {
    if (documentLoaded_ || committedOffer_ || offerProposal_ ||
        acceptProposal_ || position_.nextRecordIndex != 0 ||
        !issuedSequences_.empty() || !completedSequences_.empty())
      return false;
    document_ = std::move(document);
    documentLoaded_ = true;
    position_.endOfTrace = document_.records.empty();
    return true;
  }

  const Transaction *peekOffer() const {
    return committedOffer_ ? &*committedOffer_ : nullptr;
  }
  bool hasOffer() const { return committedOffer_.has_value(); }
  bool eof() const { return position_.endOfTrace; }
  TracePosition position() const { return position_; }

  bool proposeAccept() {
    if (!committedOffer_ || acceptProposal_)
      return false;
    acceptProposal_ = true;
    return true;
  }

  bool markDependencyComplete(uint64_t sequenceId) {
    if (!issuedSequences_.contains(sequenceId))
      return false;
    completedSequences_.insert(sequenceId);
    return true;
  }

  void doWork(Epoch epoch) override {
    if (position_.endOfTrace || committedOffer_ || offerProposal_ ||
        acceptProposal_ ||
        position_.nextRecordIndex >= document_.records.size())
      return;
    const PtoTraceRecord &record = document_.records[position_.nextRecordIndex];
    if (record.issueTime && epoch.time < *record.issueTime) {
      if (system_ && !issueWakeScheduled_) {
        if (!system_->scheduleEvent({{*record.issueTime, 0},
                                     id(),
                                     kIssueEventKind,
                                     record.sequenceId})) {
          setRuntimeFailureCode("trace_issue_wake_failed");
          return;
        }
        issueWakeScheduled_ = true;
      }
      return;
    }
    issueWakeScheduled_ = false;
    for (uint64_t dependency : record.dependencies)
      if (!completedSequences_.contains(dependency))
        return;

    std::optional<Transaction> decoded = std::invoke(decoder_, record);
    if (!decoded) {
      setRuntimeFailureCode("trace_decode_failed");
      return;
    }
    offerProposal_ = std::move(decoded);
    proposedSequenceId_ = record.sequenceId;
  }

  void bindSystem(SimSystem *system) override { system_ = system; }

  void doArbitrate(Epoch) override {
    if (acceptProposal_ && committedSequenceId_)
      emitObservation({.category = "transaction",
                       .name = "accepted",
                       .phase = TraceEventPhase::Instant,
                       .rootSequenceId = committedSequenceId_,
                       .arguments = {{"trace_position",
                                      static_cast<uint64_t>(
                                          position_.nextRecordIndex + 1)}}});
    if (offerProposal_ && proposedSequenceId_)
      emitObservation(
          {.category = "transaction",
           .name = "offered",
           .phase = TraceEventPhase::Instant,
           .rootSequenceId = proposedSequenceId_,
           .arguments = {{"trace_position",
                          static_cast<uint64_t>(position_.nextRecordIndex)}}});
  }

  void doXfer(Epoch epoch) override {
    if (acceptProposal_) {
      issuedSequences_.insert(*committedSequenceId_);
      position_.lastCommittedSequenceId = committedSequenceId_;
      ++position_.nextRecordIndex;
      position_.endOfTrace =
          position_.nextRecordIndex == document_.records.size();
      committedOffer_.reset();
      committedSequenceId_.reset();
      acceptProposal_ = false;
      lastUpdate_ = epoch;
    }
    if (offerProposal_) {
      committedOffer_ = std::move(offerProposal_);
      committedSequenceId_ = proposedSequenceId_;
      offerProposal_.reset();
      proposedSequenceId_.reset();
    }
  }

  bool hasPendingCommit() const override {
    return offerProposal_.has_value() || acceptProposal_;
  }

  bool isRunnable(Epoch epoch) const override {
    if (position_.endOfTrace || committedOffer_ || offerProposal_ ||
        acceptProposal_ ||
        position_.nextRecordIndex >= document_.records.size())
      return false;
    const PtoTraceRecord &record = document_.records[position_.nextRecordIndex];
    if (record.issueTime && epoch.time < *record.issueTime)
      return false;
    for (uint64_t dependency : record.dependencies)
      if (!completedSequences_.contains(dependency))
        return false;
    return true;
  }

  RuntimeObjectState runtimeState(Epoch epoch) const override {
    RuntimeObjectState state = SimObject::runtimeState(epoch);
    state.traceOwner = true;
    state.tracePosition = position_.nextRecordIndex;
    state.traceLastCommittedSequenceId = position_.lastCommittedSequenceId;
    state.traceEof = position_.endOfTrace;
    state.pendingOffers =
        committedOffer_.has_value() || offerProposal_.has_value();
    state.quiescent = position_.endOfTrace && !hasPendingCommit() &&
                      !committedOffer_.has_value();
    if (state.quiescent)
      state.reason.clear();
    else if (committedOffer_)
      state.reason = "trace_offer_blocked";
    else if (hasPendingCommit())
      state.reason = "pending_commit";
    else if (position_.nextRecordIndex < document_.records.size()) {
      const PtoTraceRecord &record =
          document_.records[position_.nextRecordIndex];
      for (uint64_t dependency : record.dependencies)
        if (!completedSequences_.contains(dependency))
          state.dependencyChain.push_back(dependency);
      if (!state.dependencyChain.empty())
        state.reason = "trace_dependency_blocked";
      else if (record.issueTime && epoch.time < *record.issueTime)
        state.reason = "trace_issue_time_pending";
      else
        state.reason = "trace_runnable_unscheduled";
    }
    return state;
  }

  void collectStatistics(std::vector<StatSnapshot> &out) const override {
    out.push_back({.name = "accepted_transactions",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = position_.nextRecordIndex,
                   .lastUpdate = lastUpdate_});
    out.push_back({.name = "trace_position",
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Gauge,
                   .value = position_.nextRecordIndex,
                   .lastUpdate = lastUpdate_});
  }

  bool validate() const {
    return !(committedOffer_ && offerProposal_) &&
           (committedOffer_.has_value() == committedSequenceId_.has_value()) &&
           (offerProposal_.has_value() == proposedSequenceId_.has_value()) &&
           (!acceptProposal_ || committedOffer_.has_value()) &&
           position_.nextRecordIndex <= document_.records.size() &&
           position_.endOfTrace ==
               (position_.nextRecordIndex == document_.records.size()) &&
           issuedSequences_.size() == position_.nextRecordIndex &&
           position_.lastCommittedSequenceId.has_value() ==
               (position_.nextRecordIndex != 0);
  }

  void reset() override {
    position_ = TracePosition{.endOfTrace = document_.records.empty()};
    committedOffer_.reset();
    offerProposal_.reset();
    committedSequenceId_.reset();
    proposedSequenceId_.reset();
    acceptProposal_ = false;
    issueWakeScheduled_ = false;
    issuedSequences_.clear();
    completedSequences_.clear();
    clearRuntimeFailureCode();
    lastUpdate_ = {};
  }

private:
  PtoTraceDocument document_;
  [[no_unique_address]] Decoder decoder_;
  TracePosition position_;
  std::optional<Transaction> committedOffer_;
  std::optional<Transaction> offerProposal_;
  std::optional<uint64_t> committedSequenceId_;
  std::optional<uint64_t> proposedSequenceId_;
  bool acceptProposal_ = false;
  bool issueWakeScheduled_ = false;
  bool documentLoaded_ = false;
  std::set<uint64_t> issuedSequences_;
  std::set<uint64_t> completedSequences_;
  SimSystem *system_ = nullptr;
  Epoch lastUpdate_;
  static constexpr uint32_t kIssueEventKind = 0x54524345;
};

} // namespace gfsim

#endif // GFSIM_TRACE_H
