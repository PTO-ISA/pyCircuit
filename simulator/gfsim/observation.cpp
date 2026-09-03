#include "gfsim/observation.h"

#include <algorithm>
#include <limits>
#include <tuple>
#include <utility>

namespace gfsim {
namespace {

bool validName(std::string_view value) {
  auto alpha = [](char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
  };
  if (value.empty() || !(alpha(value.front()) || value.front() == '_'))
    return false;
  return std::all_of(value.begin() + 1, value.end(), [&](char character) {
    return alpha(character) || (character >= '0' && character <= '9') ||
           character == '_' || character == '.' || character == '-';
  });
}

bool lessEvent(const CommittedEvent &left, const CommittedEvent &right) {
  return std::tie(left.epoch, left.ownerId, left.localCommittedIndex) <
         std::tie(right.epoch, right.ownerId, right.localCommittedIndex);
}

} // namespace

bool ObservationRecorder::reject(std::string message) {
  lastError_ = std::move(message);
  return false;
}

bool ObservationRecorder::propose(EventProposal proposal) {
  lastError_.clear();
  if (proposal.ownerId == kInvalidObjectId || !validName(proposal.category) ||
      !validName(proposal.name))
    return reject("observation owner, category, or name is invalid");
  std::string previous;
  for (const ObservationArgument &argument : proposal.arguments) {
    if (!validName(argument.name) ||
        std::string_view(argument.name).starts_with("gfsim_") ||
        (!previous.empty() && previous >= argument.name))
      return reject("observation arguments must be sorted and unique");
    previous = argument.name;
  }
  const bool flow = proposal.phase == TraceEventPhase::FlowStart ||
                    proposal.phase == TraceEventPhase::FlowEnd;
  if (flow != proposal.flowId.has_value())
    return reject("flow observations require exactly one flow identity");
  if (proposal.phase == TraceEventPhase::Complete) {
    if (!proposal.duration)
      return reject("complete observations require a duration");
  } else if (proposal.duration) {
    return reject("only complete observations may carry a duration");
  }
  pending_[proposal.ownerId].push_back(std::move(proposal));
  return true;
}

bool ObservationRecorder::commitOwner(ObjectId owner, Epoch epoch) {
  lastError_.clear();
  if (owner == kInvalidObjectId)
    return reject("observation commit owner is invalid");
  if (lastCommitEpoch_ && epoch < *lastCommitEpoch_)
    return reject("observation commit time regressed");
  auto pending = pending_.find(owner);
  if (pending == pending_.end())
    return true;

  uint64_t next = nextIndices_[owner];
  if (pending->second.size() > std::numeric_limits<uint64_t>::max() - next)
    return reject("observation owner-local index overflowed");
  std::map<uint64_t, bool> flows = activeFlows_;
  for (const EventProposal &proposal : pending->second) {
    if (proposal.phase == TraceEventPhase::FlowStart) {
      if (flows[*proposal.flowId])
        return reject("observation flow started more than once");
      flows[*proposal.flowId] = true;
    } else if (proposal.phase == TraceEventPhase::FlowEnd) {
      auto flow = flows.find(*proposal.flowId);
      if (flow == flows.end() || !flow->second)
        return reject("observation flow ended before it started");
      flow->second = false;
    }
  }

  std::vector<CommittedEvent> additions;
  additions.reserve(pending->second.size());
  for (EventProposal &proposal : pending->second) {
    additions.push_back({.epoch = epoch,
                         .ownerId = owner,
                         .localCommittedIndex = next++,
                         .category = std::move(proposal.category),
                         .name = std::move(proposal.name),
                         .phase = proposal.phase,
                         .rootSequenceId = proposal.rootSequenceId,
                         .duration = proposal.duration,
                         .flowId = proposal.flowId,
                         .arguments = std::move(proposal.arguments)});
  }
  pending_.erase(pending);
  nextIndices_[owner] = next;
  activeFlows_ = std::move(flows);
  for (CommittedEvent &event : additions) {
    auto position = std::lower_bound(committed_.begin(), committed_.end(),
                                     event, lessEvent);
    committed_.insert(position, std::move(event));
  }
  lastCommitEpoch_ = epoch;
  return true;
}

void ObservationRecorder::rejectOwner(ObjectId owner) { pending_.erase(owner); }

size_t ObservationRecorder::pendingCount(ObjectId owner) const {
  auto pending = pending_.find(owner);
  return pending == pending_.end() ? 0 : pending->second.size();
}

void ObservationRecorder::reset() {
  pending_.clear();
  nextIndices_.clear();
  activeFlows_.clear();
  committed_.clear();
  lastCommitEpoch_.reset();
  lastError_.clear();
}

} // namespace gfsim
