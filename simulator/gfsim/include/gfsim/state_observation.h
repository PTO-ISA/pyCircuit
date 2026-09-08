#ifndef GFSIM_STATE_OBSERVATION_H
#define GFSIM_STATE_OBSERVATION_H
#include "gfsim/core.h"
#include <cstddef>
#include <span>

namespace gfsim {
// Synchronous, non-owning notifications. Values/fields live only for the call.
// Observers must not mutate functional state or participate in arbitration.
enum class StateAction {
  OfferPush,
  OfferPop,
  Dequeue,
  Ready,
  Enqueue,
  QueueCommitEnd,
  TableRead,
  BeforeWrite,
  AfterWrite
};
enum class ExecutionPhase { External, Work, Arbitrate, Probe, Commit };
struct StateEvent {
  StateAction action;
  ObjectId object;
  size_t index = 0;
  const void *value = nullptr;
  Tick readyTime = 0;
  ObjectId writer = kInvalidObjectId;
  std::span<const size_t> fields{};
  uint8_t mode = 0;
};
class StateObserver {
public:
  virtual ~StateObserver() = default;
  virtual void notify(const StateEvent &) = 0;
  virtual void context(ObjectId, ExecutionPhase) = 0;
  virtual void clearContext() = 0;
  virtual void begin(Epoch) = 0;
  virtual void end() = 0;
  virtual void beforeReset() = 0;
};
class StateObservationScope {
public:
  StateObservationScope(StateObserver *observer, ObjectId owner,
                        ExecutionPhase phase)
      : observer_(observer) {
    if (observer_)
      observer_->context(owner, phase);
  }
  ~StateObservationScope() {
    if (observer_)
      observer_->clearContext();
  }
  StateObservationScope(const StateObservationScope &) = delete;
  StateObservationScope &operator=(const StateObservationScope &) = delete;

private:
  StateObserver *observer_;
};
} // namespace gfsim
#endif
