#ifndef GFSIM_OBSERVATION_H
#define GFSIM_OBSERVATION_H

#include "gfsim/core.h"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace gfsim {

enum class TraceEventPhase : uint8_t {
  Instant,
  Complete,
  Counter,
  FlowStart,
  FlowEnd,
};

using ObservationValue = std::variant<bool, int64_t, uint64_t, std::string>;

struct ObservationArgument {
  std::string name;
  ObservationValue value;
  bool operator==(const ObservationArgument &) const = default;
};

struct EventProposal {
  ObjectId ownerId = kInvalidObjectId;
  std::string category;
  std::string name;
  TraceEventPhase phase = TraceEventPhase::Instant;
  std::optional<uint64_t> rootSequenceId;
  std::optional<uint64_t> duration;
  std::optional<uint64_t> flowId;
  std::vector<ObservationArgument> arguments;
  bool operator==(const EventProposal &) const = default;
};

/// Optional non-owning destination for cold-path observation proposals.
class ObservationSink {
public:
  virtual ~ObservationSink() = default;
  virtual bool proposeObservation(EventProposal proposal) = 0;
};

struct CommittedEvent {
  Epoch epoch;
  ObjectId ownerId = kInvalidObjectId;
  uint64_t localCommittedIndex = 0;
  std::string category;
  std::string name;
  TraceEventPhase phase = TraceEventPhase::Instant;
  std::optional<uint64_t> rootSequenceId;
  std::optional<uint64_t> duration;
  std::optional<uint64_t> flowId;
  std::vector<ObservationArgument> arguments;
  bool operator==(const CommittedEvent &) const = default;
};

/// Buffers private pre-commit proposals and publishes them only for an owner
/// whose functional state commits at the Xfer barrier.
class ObservationRecorder {
public:
  bool propose(EventProposal proposal);
  bool commitOwner(ObjectId owner, Epoch epoch);
  void rejectOwner(ObjectId owner);

  std::span<const CommittedEvent> events() const { return committed_; }
  size_t pendingCount(ObjectId owner) const;
  std::string_view lastError() const { return lastError_; }
  void reset();

private:
  std::map<ObjectId, std::vector<EventProposal>> pending_;
  std::map<ObjectId, uint64_t> nextIndices_;
  std::map<uint64_t, bool> activeFlows_;
  std::vector<CommittedEvent> committed_;
  std::optional<Epoch> lastCommitEpoch_;
  std::string lastError_;

  bool reject(std::string message);
};

} // namespace gfsim

#endif // GFSIM_OBSERVATION_H
