#include "gfsim/object.h"
#include "gfsim/pto_trace.h"
#include "gfsim/queue.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace gfsim {

struct SimSystem::Impl {
  std::map<ObjectId, SimObject *> objects;
  std::map<Epoch, std::set<ObjectId>> scheduledWork;
  EventQueue eventQueue{"events", kInvalidObjectId, nullptr};
  DispatchTable dispatch;
  ActivationPlan activation;
  LegacyDispatchTable legacyDispatch;
  LegacyActivationGraph legacyActivation;
  uint64_t committedEventCount = 0;
  bool executingEpoch = false;
  std::optional<ObjectId> activeProposalOwner;
  NoProgressReport noProgress;
  size_t traceOwnerCount = 0;
  bool traceEof = true;
  bool preflightValidated = false;
  std::map<ObjectId, Tick> lastCommitTick;
  std::optional<Tick> eventQueueLastCommitTick;
  std::map<std::string, TimeDomainRuntime> timeDomains;
  std::map<std::string, uint64_t> maxDomainCycles;
  std::map<std::string, uint64_t> domainCycles;
  std::map<std::string, StatSnapshot, std::less<>> generatedStats;
  ObservationRecorder observations;
  PtoTraceProvider ptoTrace;
  std::optional<uint64_t> deadlockWindow;
  Tick lastProgressTick = 0;
};

SimSystem::~SimSystem() = default;

SimSystem::SimSystem(std::string name)
    : SimObject(ObjectKind::System, std::move(name), kSystemObjectId),
      root_(std::make_unique<Module>("root", kRootObjectId, this)),
      impl_(std::make_unique<Impl>()) {
  setPath("/" + std::string(this->name()));
  root_->setPath(std::string(path()));
  impl_->objects[kSystemObjectId] = this;
}

bool SimSystem::fail(std::string code, std::string message) {
  terminated_ = true;
  impl_->executingEpoch = false;
  result_.classification = TerminationClass::Failed;
  result_.finalEpoch = epoch_;
  result_.committedEventCount = impl_->committedEventCount;
  result_.domainCycles = impl_->domainCycles;
  result_.diagnosticCode = std::move(code);
  result_.message = std::move(message);
  return false;
}

std::vector<SimObject *> SimSystem::runtimeObjects() const {
  std::map<ObjectId, SimObject *> objects;
  for (const auto &[id, object] : impl_->objects)
    if (id != kSystemObjectId && object)
      objects[id] = object;
  root_->walk([&](const SimObject &object) {
    if (object.kind() != ObjectKind::Module)
      objects[object.id()] = const_cast<SimObject *>(&object);
  });
  for (ObjectId id = 0; id < impl_->dispatch.size(); ++id)
    if (const DispatchRow *row = impl_->dispatch.lookup(id))
      objects[id] = static_cast<SimObject *>(row->object);

  std::vector<SimObject *> result;
  result.reserve(objects.size());
  for (const auto &[id, object] : objects)
    result.push_back(object);
  return result;
}

bool SimSystem::validateRuntimeIdentities() {
  std::map<ObjectId, const SimObject *> ids;
  std::map<std::string, const SimObject *> paths;
  std::string conflict;
  auto record = [&](const SimObject *object) {
    if (!object || !conflict.empty() || object == this)
      return;
    if (object->id() == kInvalidObjectId && object->asModule() == nullptr) {
      conflict = "runtime object has the invalid object ID";
      return;
    }
    if (object->id() != kInvalidObjectId) {
      if (auto [position, inserted] = ids.emplace(object->id(), object);
          !inserted && position->second != object) {
        conflict = "stable object ID " + std::to_string(object->id()) +
                   " names more than one runtime object";
        return;
      }
    }
    if (object->path().empty())
      return;
    if (auto [position, inserted] =
            paths.emplace(std::string(object->path()), object);
        !inserted && position->second != object)
      conflict = "canonical object path " + std::string(object->path()) +
                 " names more than one runtime object";
  };

  for (const auto &[id, object] : impl_->objects)
    record(object);
  root_->walk([&](const SimObject &object) { record(&object); });
  for (ObjectId id = 0; id < impl_->dispatch.size(); ++id)
    if (const DispatchRow *row = impl_->dispatch.lookup(id))
      record(static_cast<const SimObject *>(row->object));
  if (!conflict.empty())
    return fail("duplicate_object_identity", std::move(conflict));
  impl_->preflightValidated = true;
  return true;
}

void SimSystem::refreshRuntimeSummary() {
  impl_->noProgress = {};
  impl_->noProgress.nextEvent = impl_->eventQueue.nextEvent();
  impl_->traceOwnerCount = 0;
  impl_->traceEof = true;

  for (SimObject *object : runtimeObjects()) {
    RuntimeObjectState state = object->runtimeState(epoch_);
    if (state.traceOwner) {
      ++impl_->traceOwnerCount;
      impl_->traceEof = impl_->traceEof && state.traceEof;
      impl_->noProgress.tracePosition = state.tracePosition;
      impl_->noProgress.lastCommittedSequenceId =
          state.traceLastCommittedSequenceId;
    }
    impl_->noProgress.queueOccupancy += state.queueOccupancy;
    impl_->noProgress.pendingOffers += state.pendingOffers;
    impl_->noProgress.activeReservations += state.activeReservations;
    if (state.quiescent)
      continue;
    impl_->noProgress.blockedObjects.push_back(
        {.id = object->id(),
         .path = std::string(object->path()),
         .reason = std::move(state.reason),
         .subscriptions = std::move(state.subscriptions),
         .dependencyChain = std::move(state.dependencyChain),
         .correlationChain = std::move(state.correlationChain),
         .queueOccupancy = state.queueOccupancy,
         .pendingOffers = state.pendingOffers,
         .activeReservations = state.activeReservations,
         .protocolState = std::move(state.protocolState)});
  }
  result_.tracePosition = impl_->noProgress.tracePosition;
  result_.traceLastCommittedSequenceId =
      impl_->noProgress.lastCommittedSequenceId;
  if (!impl_->noProgress.blockedObjects.empty())
    impl_->noProgress.summary =
        "unfinished runtime state has no scheduled wake or future event";
}

bool SimSystem::stopAtTraceCap() {
  refreshRuntimeSummary();
  if (impl_->traceOwnerCount > 1) {
    fail("multiple_trace_owners",
         "the runtime must have exactly one committed trace cursor owner");
    return true;
  }
  if (impl_->traceOwnerCount == 0 || impl_->traceEof ||
      result_.tracePosition < maxTraceRecords_)
    return false;
  terminated_ = true;
  impl_->executingEpoch = false;
  result_.classification = TerminationClass::Incomplete;
  result_.finalEpoch = epoch_;
  result_.committedEventCount = impl_->committedEventCount;
  result_.domainCycles = impl_->domainCycles;
  result_.terminationCap = maxTraceRecords_;
  result_.diagnosticCode = "max_trace_records_reached";
  return true;
}

NoProgressReport SimSystem::noProgressReport() const {
  return impl_->noProgress;
}

std::vector<StatSnapshot> SimSystem::statistics() const {
  std::vector<StatSnapshot> snapshots;
  for (const SimObject *object : runtimeObjects())
    object->collectStatistics(snapshots);
  for (const auto &[name, snapshot] : impl_->generatedStats)
    snapshots.push_back(snapshot);
  std::stable_sort(snapshots.begin(), snapshots.end(),
                   [](const StatSnapshot &left, const StatSnapshot &right) {
                     return std::tie(left.objectPath, left.name) <
                            std::tie(right.objectPath, right.name);
                   });
  return snapshots;
}

std::span<const CommittedEvent> SimSystem::observations() const {
  return impl_->observations.events();
}

bool SimSystem::proposeObservation(EventProposal proposal) {
  if (terminated_ || !impl_->executingEpoch || !impl_->activeProposalOwner)
    return false;
  if (proposal.ownerId != *impl_->activeProposalOwner ||
      !lookup(proposal.ownerId))
    return fail("invalid_observation_owner",
                "observation owner must be the active runtime object");
  if (!impl_->observations.propose(std::move(proposal)))
    return fail("invalid_observation_proposal",
                std::string(impl_->observations.lastError()));
  return true;
}

void SimSystem::registerObject(SimObject *obj) {
  if (!obj || obj->id() == kInvalidObjectId ||
      impl_->objects.contains(obj->id())) {
    fail("invalid_object_registration",
         "runtime object registration is null, invalid, or duplicate");
    return;
  }
  impl_->objects[obj->id()] = obj;
  impl_->preflightValidated = false;
}

bool SimSystem::setDispatchTable(std::span<const DispatchRow> rows) {
  DispatchTable candidate(rows);
  if (!candidate.validate())
    return fail("invalid_dispatch_table",
                "dispatch rows must be complete and densely indexed");
  impl_->dispatch = candidate;
  impl_->activation = ActivationPlan{};
  impl_->preflightValidated = false;
  return true;
}

bool SimSystem::setActivationPlan(std::span<const uint32_t> offsets,
                                  std::span<const ObjectId> targets) {
  ActivationPlan candidate(offsets, targets);
  if (!candidate.validate(impl_->dispatch.size()))
    return fail("invalid_activation_plan",
                "activation offsets and targets must be canonical and dense");
  impl_->activation = candidate;
  return true;
}

bool SimSystem::setLegacyDispatchTable(LegacyDispatchTable table) {
  if ((!table.rows && table.objectCount != 0) ||
      (table.rows && table.objectCount == 0))
    return fail("invalid_dispatch_table",
                "legacy dispatch storage and object count disagree");
  for (uint32_t id = 0; id < table.objectCount; ++id) {
    const LegacyDispatchThunk &row = table.rows[id];
    if (!row.object || !row.work || !row.xfer || !row.reset ||
        !row.validate || !row.validate(row.object))
      return fail("invalid_dispatch_table",
                  "legacy dispatch rows must be complete and valid");
  }
  impl_->legacyDispatch = table;
  impl_->legacyActivation = {};
  impl_->preflightValidated = false;
  return true;
}

bool SimSystem::setLegacyActivationGraph(LegacyActivationGraph graph) {
  if (!graph.offsets || graph.sourceCount != impl_->legacyDispatch.objectCount)
    return fail("invalid_activation_plan",
                "legacy activation graph must cover every dispatch row");
  const uint32_t targetCount = graph.offsets[graph.sourceCount];
  if (targetCount != 0 && !graph.targets)
    return fail("invalid_activation_plan",
                "legacy activation targets are missing");
  for (uint32_t source = 0; source < graph.sourceCount; ++source) {
    if (graph.offsets[source] > graph.offsets[source + 1])
      return fail("invalid_activation_plan",
                  "legacy activation offsets must be monotonic");
    for (uint32_t index = graph.offsets[source];
         index < graph.offsets[source + 1]; ++index)
      if (graph.targets[index] >= graph.sourceCount)
        return fail("invalid_activation_plan",
                    "legacy activation target is out of range");
  }
  impl_->legacyActivation = graph;
  return true;
}

bool SimSystem::setTimeDomains(std::span<const TimeDomainRuntime> domains) {
  if (terminated_)
    return false;
  std::map<std::string, TimeDomainRuntime> candidate;
  std::string previous;
  for (const TimeDomainRuntime &domain : domains) {
    if (domain.name.empty() || domain.period == 0 || domain.tickScale == 0 ||
        (!previous.empty() && previous >= domain.name) ||
        !candidate.emplace(domain.name, domain).second)
      return fail("invalid_time_domains",
                  "time domains must be sorted, unique, and positive");
    previous = domain.name;
  }
  impl_->timeDomains = std::move(candidate);
  impl_->domainCycles.clear();
  for (const auto &[name, domain] : impl_->timeDomains)
    impl_->domainCycles.emplace(name, 0);
  return true;
}

bool SimSystem::setDeadlockWindow(std::optional<uint64_t> window) {
  if (terminated_)
    return false;
  if (window && *window == 0)
    return fail("invalid_runtime_limits",
                "the deadlock window must be positive");
  impl_->deadlockWindow = window;
  impl_->lastProgressTick = epoch_.time;
  return true;
}

bool SimSystem::setMaxDomainCycles(
    const std::map<std::string, uint64_t> &limits) {
  if (terminated_)
    return false;
  for (const auto &[name, maximum] : limits)
    if (maximum == 0 || !impl_->timeDomains.contains(name))
      return fail("invalid_runtime_limits",
                  "domain limits must name configured time domains");
  impl_->maxDomainCycles = limits;
  return true;
}

bool SimSystem::setRuntimeLimits(const RuntimeLimits &limits) {
  if (terminated_)
    return false;
  if (limits.maxTicks && *limits.maxTicks == 0)
    return fail("invalid_runtime_limits", "runtime limits must be positive");
  if (!setDeadlockWindow(limits.deadlockWindow) ||
      !setMaxDomainCycles(limits.maxDomainCycles))
    return false;
  maxTicks_ = limits.maxTicks.value_or(UINT64_MAX);
  return true;
}

SimObject *SimSystem::lookup(ObjectId id) const {
  if (const DispatchRow *row = impl_->dispatch.lookup(id))
    return static_cast<SimObject *>(row->object);
  auto it = impl_->objects.find(id);
  return it != impl_->objects.end() ? it->second : nullptr;
}

void SimSystem::requestTerminate(TerminationClass classification,
                                 std::string diagnosticCode) {
  terminated_ = true;
  impl_->executingEpoch = false;
  result_.classification = classification;
  result_.finalEpoch = epoch_;
  result_.committedEventCount = impl_->committedEventCount;
  result_.domainCycles = impl_->domainCycles;
  result_.diagnosticCode = std::move(diagnosticCode);
}

void SimSystem::recordStat(std::string name, uint64_t value) {
  StatSnapshot snapshot;
  snapshot.name = name;
  snapshot.objectPath = std::string(path());
  snapshot.kind = StatisticKind::Counter;
  snapshot.value = value;
  snapshot.lastUpdate = epoch_;
  impl_->generatedStats.insert_or_assign(std::move(name), std::move(snapshot));
}

void SimSystem::loadPtoTrace(std::string source, const std::string &path) {
  impl_->ptoTrace.load(std::move(source), path);
}

uint64_t SimSystem::traceOpen(std::string_view source) const {
  return impl_->ptoTrace.open(source);
}

TraceNextResult SimSystem::traceNext(std::string_view source,
                                     uint64_t cursor) const {
  return impl_->ptoTrace.next(source, static_cast<size_t>(cursor));
}

uint64_t SimSystem::traceDecode(uint64_t handle) const {
  return impl_->ptoTrace.decode(handle);
}

bool SimSystem::traceEof(std::string_view source, uint64_t cursor) const {
  return impl_->ptoTrace.eof(source, static_cast<size_t>(cursor));
}

uint64_t SimSystem::tracePosition(std::string_view source,
                                  uint64_t cursor) const {
  return impl_->ptoTrace.position(source, static_cast<size_t>(cursor));
}

uint64_t SimSystem::traceRecordCount(std::string_view source) const {
  return impl_->ptoTrace.recordCount(source);
}

bool SimSystem::scheduleWork(ObjectId id, Epoch epoch) {
  if (terminated_)
    return false;
  if (epoch.delta >= kMaxDeltasPerTick)
    return fail("max_deltas_exceeded",
                "scheduled work exceeds the causal delta limit");
  if (epoch < epoch_)
    return fail("work_before_current_epoch",
                "work cannot be scheduled before the committed epoch");
  const bool hasLegacy =
      impl_->legacyDispatch.rows && id < impl_->legacyDispatch.objectCount &&
      impl_->legacyDispatch.rows[id].object;
  if (!lookup(id) && !hasLegacy)
    return fail("unknown_work_target",
                "work target is absent from the static dispatch table");
  if (impl_->executingEpoch && epoch == epoch_) {
    if (epoch_.delta + 1 >= kMaxDeltasPerTick)
      return fail("max_deltas_exceeded",
                  "causal continuation exceeds the delta limit");
    epoch = epoch_.nextDelta();
  }
  impl_->scheduledWork[epoch].insert(id);
  return true;
}

bool SimSystem::scheduleEvent(Event event) {
  if (terminated_)
    return false;
  if (event.readyTime.delta >= kMaxDeltasPerTick)
    return fail("max_deltas_exceeded",
                "scheduled event exceeds the causal delta limit");
  if (event.readyTime < epoch_)
    return fail("event_before_current_epoch",
                "events cannot be scheduled before the committed epoch");
  if (!lookup(event.targetId))
    return fail("unknown_event_target",
                "event target is absent from the static dispatch table");
  if (!impl_->eventQueue.proposeSchedule(event))
    return fail("event_queue_capacity_exceeded",
                "the global event queue capacity was exceeded");
  return true;
}

std::optional<Event> SimSystem::nextEvent() const {
  return impl_->eventQueue.nextEvent();
}

bool SimSystem::step() {
  if (terminated_)
    return false;
  if (!impl_->preflightValidated && !validateRuntimeIdentities())
    return false;
  if (stopAtTraceCap())
    return false;

  if (epoch_.time >= maxTicks_) {
    terminated_ = true;
    result_.classification = TerminationClass::Incomplete;
    result_.finalEpoch = epoch_;
    result_.committedEventCount = impl_->committedEventCount;
    result_.domainCycles = impl_->domainCycles;
    result_.terminationCap = maxTicks_;
    result_.diagnosticCode = "max_ticks_reached";
    return false;
  }

  if (epoch_.delta == 0) {
    for (const auto &[name, domain] : impl_->timeDomains) {
      if (epoch_.time < domain.phase ||
          (epoch_.time - domain.phase) % domain.period != 0)
        continue;
      uint64_t &cycles = impl_->domainCycles[name];
      if (auto maximum = impl_->maxDomainCycles.find(name);
          maximum != impl_->maxDomainCycles.end() &&
          cycles >= maximum->second) {
        terminated_ = true;
        impl_->executingEpoch = false;
        result_.classification = TerminationClass::Incomplete;
        result_.finalEpoch = epoch_;
        result_.committedEventCount = impl_->committedEventCount;
        result_.terminationCap = maximum->second;
        result_.domainCycles = impl_->domainCycles;
        result_.diagnosticCode = "max_domain_cycles_reached";
        return false;
      }
      ++cycles;
    }
  }

  auto stopAtEventCap = [this] {
    if (impl_->committedEventCount < maxEvents_)
      return false;
    terminated_ = true;
    impl_->executingEpoch = false;
    result_.classification = TerminationClass::Incomplete;
    result_.finalEpoch = epoch_;
    result_.committedEventCount = impl_->committedEventCount;
    result_.domainCycles = impl_->domainCycles;
    result_.terminationCap = maxEvents_;
    result_.diagnosticCode = "max_events_reached";
    return true;
  };

  if (stopAtEventCap())
    return false;

  // Events committed by a previous epoch activate their target at their exact
  // ready epoch before the immutable Work snapshot is observed.
  while (auto event = impl_->eventQueue.nextEvent()) {
    if (event->readyTime < epoch_)
      return fail("event_before_current_epoch",
                  "the event queue contains a stale event");
    if (event->readyTime != epoch_)
      break;
    if (stopAtEventCap())
      return false;
    impl_->eventQueue.popNext();
    if (!scheduleWork(event->targetId, epoch_))
      return false;
    ++impl_->committedEventCount;
    impl_->lastProgressTick = epoch_.time;
  }

  std::set<ObjectId> currentWork;
  if (auto current = impl_->scheduledWork.find(epoch_);
      current != impl_->scheduledWork.end()) {
    currentWork = std::move(current->second);
    impl_->scheduledWork.erase(current);
  }

  impl_->executingEpoch = true;
  for (ObjectId id : currentWork) {
    impl_->activeProposalOwner = id;
    const LegacyDispatchThunk *legacy =
        impl_->legacyDispatch.rows && id < impl_->legacyDispatch.objectCount
            ? &impl_->legacyDispatch.rows[id]
            : nullptr;
    if (legacy)
      legacy->work(legacy->object, epoch_);
    else if (const DispatchRow *row = impl_->dispatch.lookup(id))
      row->work(row->object, epoch_);
    else if (SimObject *object = lookup(id))
      object->doWork(epoch_);
    impl_->activeProposalOwner.reset();
    if (terminated_)
      return false;
  }

  for (ObjectId id : currentWork) {
    impl_->activeProposalOwner = id;
    const bool hasLegacy =
        impl_->legacyDispatch.rows && id < impl_->legacyDispatch.objectCount;
    if (!hasLegacy) {
      if (const DispatchRow *row = impl_->dispatch.lookup(id))
        row->xfer(row->object, epoch_, XferPhase::Arbitrate);
      else if (SimObject *object = lookup(id))
        object->doArbitrate(epoch_);
    }
    impl_->activeProposalOwner.reset();
    if (terminated_)
      return false;
  }

  std::vector<ObjectId> committedSources;
  for (ObjectId id : currentWork) {
    SimObject *object = lookup(id);
    const DispatchRow *row = impl_->dispatch.lookup(id);
    const LegacyDispatchThunk *legacy =
        impl_->legacyDispatch.rows && id < impl_->legacyDispatch.objectCount
            ? &impl_->legacyDispatch.rows[id]
            : nullptr;
    bool willCommit =
        legacy ? true
               : (row ? row->xfer(row->object, epoch_, XferPhase::Probe)
                      : object && object->hasPendingCommit());
    if (willCommit) {
      auto previousCommit = impl_->lastCommitTick.find(id);
      if (!legacy && previousCommit != impl_->lastCommitTick.end() &&
          previousCommit->second == epoch_.time)
        return fail("multiple_stateful_commits",
                    "a stateful object cannot commit twice in one tick");
    }

    bool committed = false;
    if (legacy) {
      legacy->xfer(legacy->object, epoch_);
      committed = true;
    } else if (row)
      committed = row->xfer(row->object, epoch_, XferPhase::Commit);
    else if (object) {
      object->doXfer(epoch_);
      committed = willCommit;
    }
    if (committed != willCommit)
      return fail("xfer_probe_mismatch",
                  "Xfer pending state changed between probe and commit");
    if (committed) {
      committedSources.push_back(id);
      impl_->lastCommitTick[id] = epoch_.time;
      impl_->lastProgressTick = epoch_.time;
    }
    if (terminated_)
      return false;
    if (object && !object->runtimeFailureCode().empty())
      return fail(std::string(object->runtimeFailureCode()),
                  "runtime object reported a committed failure");
    if (committed) {
      if (!impl_->observations.commitOwner(id, epoch_))
        return fail("invalid_observation_commit",
                    std::string(impl_->observations.lastError()));
    } else {
      impl_->observations.rejectOwner(id);
    }
  }
  if (impl_->eventQueue.hasPendingCommit()) {
    if (impl_->eventQueueLastCommitTick == epoch_.time)
      return fail("multiple_stateful_commits",
                  "the event queue cannot commit twice in one tick");
    impl_->eventQueueLastCommitTick = epoch_.time;
    impl_->lastProgressTick = epoch_.time;
  }
  impl_->eventQueue.doXfer(epoch_);

  if (!committedSources.empty() && !impl_->activation.empty()) {
    if (epoch_.time == std::numeric_limits<Tick>::max())
      return fail("tick_overflow", "activation would overflow simulation time");
    Epoch activationEpoch{epoch_.time + 1, 0};
    for (ObjectId source : committedSources)
      for (ObjectId target : impl_->activation.targetsFor(source))
        if (!scheduleWork(target, activationEpoch))
          return false;
  }
  if (!committedSources.empty() && impl_->legacyActivation.offsets) {
    if (epoch_.time == std::numeric_limits<Tick>::max())
      return fail("tick_overflow", "activation would overflow simulation time");
    Epoch activationEpoch{epoch_.time + 1, 0};
    for (ObjectId source : committedSources) {
      if (source >= impl_->legacyActivation.sourceCount)
        continue;
      for (uint32_t index = impl_->legacyActivation.offsets[source];
           index < impl_->legacyActivation.offsets[source + 1]; ++index)
        if (!scheduleWork(impl_->legacyActivation.targets[index],
                          activationEpoch))
          return false;
    }
  }

  // An event committed for the active epoch is a causal continuation. Its
  // target runs at the next delta, never inside the closed Work snapshot.
  while (auto event = impl_->eventQueue.nextEvent()) {
    if (event->readyTime < epoch_)
      return fail("event_before_current_epoch",
                  "the event queue contains a stale event");
    if (event->readyTime != epoch_)
      break;
    if (stopAtEventCap())
      return false;
    impl_->eventQueue.popNext();
    if (!scheduleWork(event->targetId, epoch_))
      return false;
    ++impl_->committedEventCount;
    impl_->lastProgressTick = epoch_.time;
  }
  impl_->executingEpoch = false;
  impl_->activeProposalOwner.reset();

  std::optional<Epoch> nextEpoch;
  bool nextEpochIsEvent = false;
  if (!impl_->scheduledWork.empty())
    nextEpoch = impl_->scheduledWork.begin()->first;
  if (auto event = impl_->eventQueue.nextEvent();
      event && (!nextEpoch || event->readyTime <= *nextEpoch)) {
    nextEpoch = event->readyTime;
    nextEpochIsEvent = true;
  }

  if (!nextEpoch) {
    if (stopAtTraceCap())
      return false;
    refreshRuntimeSummary();
    if (impl_->traceOwnerCount == 1 && impl_->traceEof) {
      std::vector<ObjectId> traceEndProcesses;
      for (SimObject *object : runtimeObjects())
        if (object->requestTraceEnd())
          traceEndProcesses.push_back(object->id());
      if (!traceEndProcesses.empty()) {
        if (epoch_.time == std::numeric_limits<Tick>::max())
          return fail("tick_overflow",
                      "trace-end process termination exceeds tick range");
        Epoch shutdownEpoch{epoch_.time + 1, 0};
        if (shutdownEpoch.time >= maxTicks_) {
          epoch_ = {maxTicks_, 0};
          terminated_ = true;
          result_.classification = TerminationClass::Incomplete;
          result_.finalEpoch = epoch_;
          result_.committedEventCount = impl_->committedEventCount;
          result_.terminationCap = maxTicks_;
          result_.domainCycles = impl_->domainCycles;
          result_.diagnosticCode = "max_ticks_reached";
          return false;
        }
        for (ObjectId id : traceEndProcesses)
          if (!scheduleWork(id, shutdownEpoch))
            return false;
        epoch_ = shutdownEpoch;
        return true;
      }
    }
    if (!impl_->noProgress.blockedObjects.empty() && impl_->deadlockWindow) {
      const Tick window = *impl_->deadlockWindow;
      if (impl_->lastProgressTick > std::numeric_limits<Tick>::max() - window)
        return fail("tick_overflow", "deadlock window exceeds tick range");
      Tick deadline = impl_->lastProgressTick + window;
      if (deadline >= maxTicks_) {
        epoch_ = {maxTicks_, 0};
        terminated_ = true;
        result_.classification = TerminationClass::Incomplete;
        result_.finalEpoch = epoch_;
        result_.committedEventCount = impl_->committedEventCount;
        result_.terminationCap = maxTicks_;
        result_.domainCycles = impl_->domainCycles;
        result_.diagnosticCode = "max_ticks_reached";
        return false;
      }
      epoch_ = {deadline, 0};
      return fail("deadlock_window_reached", impl_->noProgress.summary);
    }
    if (!impl_->noProgress.blockedObjects.empty())
      return fail("no_progress", impl_->noProgress.summary);
    terminated_ = true;
    result_.classification = TerminationClass::Completed;
    result_.finalEpoch = epoch_;
    result_.committedEventCount = impl_->committedEventCount;
    result_.domainCycles = impl_->domainCycles;
    return false;
  }
  if (*nextEpoch <= epoch_)
    return fail("non_monotonic_epoch",
                "scheduler failed to advance beyond the committed epoch");
  if (!nextEpochIsEvent && impl_->deadlockWindow) {
    const Tick window = *impl_->deadlockWindow;
    if (impl_->lastProgressTick > std::numeric_limits<Tick>::max() - window)
      return fail("tick_overflow", "deadlock window exceeds tick range");
    const Tick deadline = impl_->lastProgressTick + window;
    if (nextEpoch->time >= deadline) {
      epoch_ = {deadline, 0};
      return fail("deadlock_window_reached",
                  "scheduled work made no declared progress within the "
                  "deadlock window");
    }
  }
  if (nextEpoch->time >= maxTicks_) {
    epoch_ = {maxTicks_, 0};
    terminated_ = true;
    result_.classification = TerminationClass::Incomplete;
    result_.finalEpoch = epoch_;
    result_.committedEventCount = impl_->committedEventCount;
    result_.domainCycles = impl_->domainCycles;
    result_.terminationCap = maxTicks_;
    result_.diagnosticCode = "max_ticks_reached";
    return false;
  }
  epoch_ = *nextEpoch;
  return true;
}

TerminationResult SimSystem::run() {
  epoch_ = {0, 0};

  for (SimObject *object : runtimeObjects())
    if (object->kind() == ObjectKind::Process ||
        object->kind() == ObjectKind::TraceSource)
      scheduleWork(object->id(), epoch_);
  if (impl_->legacyDispatch.rows)
    for (ObjectId id = 0; id < impl_->legacyDispatch.objectCount; ++id)
      scheduleWork(id, epoch_);

  while (!terminated_)
    if (!step())
      break;

  result_.finalEpoch = epoch_;
  result_.committedEventCount = impl_->committedEventCount;
  result_.domainCycles = impl_->domainCycles;
  refreshRuntimeSummary();
  result_.stats = statistics();
  return result_;
}

void SimSystem::reset() {
  epoch_ = {0, 0};
  terminated_ = false;
  result_ = TerminationResult{};
  impl_->scheduledWork.clear();
  impl_->eventQueue.reset();
  impl_->committedEventCount = 0;
  impl_->executingEpoch = false;
  impl_->activeProposalOwner.reset();
  impl_->noProgress = {};
  impl_->generatedStats.clear();
  impl_->observations.reset();
  impl_->traceOwnerCount = 0;
  impl_->traceEof = true;
  impl_->preflightValidated = false;
  impl_->lastCommitTick.clear();
  impl_->eventQueueLastCommitTick.reset();
  impl_->lastProgressTick = 0;
  impl_->domainCycles.clear();
  for (const auto &[name, domain] : impl_->timeDomains)
    impl_->domainCycles.emplace(name, 0);
  if (impl_->legacyDispatch.rows) {
    for (ObjectId id = 0; id < impl_->legacyDispatch.objectCount; ++id) {
      const LegacyDispatchThunk &row = impl_->legacyDispatch.rows[id];
      row.reset(row.object);
    }
  } else if (!impl_->dispatch.empty()) {
    for (ObjectId id = 0; id < impl_->dispatch.size(); ++id) {
      const DispatchRow *row = impl_->dispatch.lookup(id);
      row->reset(row->object);
    }
  } else {
    for (SimObject *object : runtimeObjects())
      if (object->kind() != ObjectKind::Module)
        object->reset();
  }
}

} // namespace gfsim
