#include "gfsim/showcase.h"

#include "gfsim/components.h"
#include "gfsim/dispatch.h"
#include "gfsim/object.h"
#include "gfsim/process.h"
#include "gfsim/queue.h"

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace gfsim {
namespace {

std::vector<size_t> workOrder(size_t count, ShowcaseWorkOrder order,
                              uint64_t seed, Tick tick) {
  std::vector<size_t> result(count);
  std::iota(result.begin(), result.end(), 0);
  if (order == ShowcaseWorkOrder::Descending)
    std::reverse(result.begin(), result.end());
  else if (order == ShowcaseWorkOrder::Seeded) {
    std::mt19937_64 random(seed ^ (tick * 0x9e3779b97f4a7c15ULL));
    std::shuffle(result.begin(), result.end(), random);
  }
  return result;
}

bool installRuntime(SimSystem &system, std::span<const DispatchRow> rows,
                    std::vector<uint32_t> &offsets,
                    std::vector<ObjectId> &targets) {
  for (const DispatchRow &row : rows) {
    auto *object = static_cast<SimObject *>(row.object);
    object->bindSystem(&system);
    object->setObservationSink(&system);
  }
  offsets.assign(rows.size() + 1, 0);
  targets.clear();
  targets.reserve(rows.size() * rows.size());
  for (size_t source = 0; source < rows.size(); ++source) {
    for (ObjectId target = 0; target < rows.size(); ++target)
      targets.push_back(target);
    offsets[source + 1] = static_cast<uint32_t>(targets.size());
  }
  return system.setDispatchTable(rows) &&
         system.setActivationPlan(offsets, targets);
}

ShowcaseResult finish(SimSystem &system, TerminationResult termination,
                      std::span<SimObject *const> objects,
                      std::map<std::string, uint64_t> values,
                      uint64_t tracePosition) {
  ShowcaseResult result;
  result.termination = std::move(termination);
  result.termination.tracePosition = tracePosition;
  result.architecturalValues = std::move(values);
  result.tracePosition = tracePosition;
  result.statistics = system.statistics();
  auto events = system.observations();
  result.events.assign(events.begin(), events.end());
  std::vector<SimObject *> sorted(objects.begin(), objects.end());
  std::sort(sorted.begin(), sorted.end(),
            [](const SimObject *left, const SimObject *right) {
              return left->id() < right->id();
            });
  for (const SimObject *object : sorted)
    result.hierarchy.push_back(
        {object->id(), std::string(object->path()), object->kind()});
  return result;
}

class QueueScenarioActor final : public SimObject {
public:
  QueueScenarioActor(ObjectId id, std::span<const uint64_t> values,
                     Queue<uint64_t> &queue, Sink<uint64_t> &sink)
      : SimObject(ObjectKind::Process, "producer", id), values_(values),
        queue_(queue), sink_(sink) {}

  void doWork(Epoch) override {
    if (nextValue_ < values_.size() && queue_.proposePush(values_[nextValue_]))
      advance_ = true;
    if (auto value = queue_.proposePop())
      sink_.receive(*value);
  }

  void doXfer(Epoch) override {
    if (advance_)
      ++nextValue_;
    advance_ = false;
  }

  bool hasPendingCommit() const override { return advance_; }
  RuntimeObjectState runtimeState(Epoch) const override {
    const bool done = nextValue_ == values_.size() && queue_.isEmpty();
    return {.quiescent = done && !advance_,
            .runnable = !done,
            .pendingCommit = advance_,
            .reason = done ? "" : "showcase_queue_flow"};
  }

private:
  std::span<const uint64_t> values_;
  Queue<uint64_t> &queue_;
  Sink<uint64_t> &sink_;
  size_t nextValue_ = 0;
  bool advance_ = false;
};

ShowcaseResult
runProducerQueueConsumer(const ProducerQueueConsumerPolicy &policy) {
  SimSystem system("showcase");
  Queue<uint64_t> queue("queue", 1, nullptr, policy.queueCapacity, SIZE_MAX,
                        &system);
  Sink<uint64_t> sink("consumer", 2, nullptr, &system);
  QueueScenarioActor actor(0, policy.values, queue, sink);
  system.root().attachChild(actor);
  system.root().attachChild(queue);
  system.root().attachChild(sink);
  std::array rows = {makeDispatchRow(&actor), makeDispatchRow(&queue),
                     makeDispatchRow(&sink)};
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  TerminationResult termination = system.run();
  uint64_t sum =
      std::accumulate(sink.received().begin(), sink.received().end(), 0ULL);
  std::array<SimObject *, 3> objects = {&actor, &queue, &sink};
  return finish(
      system, std::move(termination), objects,
      {{"consumed_count", sink.totalReceived()}, {"consumed_sum", sum}},
      sink.totalReceived());
}

class BackpressureActor final : public SimObject {
public:
  BackpressureActor(ObjectId id, const BackpressuredPipelinePolicy &policy,
                    ReadyValid<uint64_t> &channel, Sink<uint64_t> &sink)
      : SimObject(ObjectKind::Process, "producer", id), policy_(policy),
        channel_(channel), sink_(sink) {}

  void doWork(Epoch epoch) override {
    if (channel_.transferCount() > observedTransfers_) {
      sink_.receive(channel_.lastTransferred());
      proposedObservedTransfers_ = channel_.transferCount();
    }
    if (nextValue_ < policy_.values.size() && !channel_.hasOffer() &&
        channel_.proposeOffer(policy_.values[nextValue_]))
      advance_ = true;
    if (channel_.transferCount() < policy_.values.size() ||
        nextValue_ < policy_.values.size() || channel_.hasOffer()) {
      const bool ready = std::ranges::find(policy_.readyTicks, epoch.time) !=
                         policy_.readyTicks.end();
      channel_.proposeReady(ready);
    }
  }

  void doXfer(Epoch) override {
    if (advance_)
      ++nextValue_;
    if (proposedObservedTransfers_)
      observedTransfers_ = *proposedObservedTransfers_;
    advance_ = false;
    proposedObservedTransfers_.reset();
  }

  bool hasPendingCommit() const override {
    return advance_ || proposedObservedTransfers_.has_value();
  }
  RuntimeObjectState runtimeState(Epoch) const override {
    const bool done = observedTransfers_ == policy_.values.size() &&
                      nextValue_ == policy_.values.size() &&
                      !channel_.hasOffer();
    return {.quiescent = done && !hasPendingCommit(),
            .runnable = !done,
            .pendingCommit = hasPendingCommit(),
            .reason = done ? "" : "showcase_backpressure"};
  }

private:
  const BackpressuredPipelinePolicy &policy_;
  ReadyValid<uint64_t> &channel_;
  Sink<uint64_t> &sink_;
  size_t nextValue_ = 0;
  uint64_t observedTransfers_ = 0;
  std::optional<uint64_t> proposedObservedTransfers_;
  bool advance_ = false;
};

ShowcaseResult runBackpressured(const BackpressuredPipelinePolicy &policy) {
  SimSystem system("showcase");
  ReadyValid<uint64_t> channel("ready_valid", 1, nullptr, &system);
  Sink<uint64_t> sink("consumer", 2, nullptr, &system);
  BackpressureActor actor(0, policy, channel, sink);
  system.root().attachChild(actor);
  system.root().attachChild(channel);
  system.root().attachChild(sink);
  std::array rows = {makeDispatchRow(&actor), makeDispatchRow(&channel),
                     makeDispatchRow(&sink)};
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  TerminationResult termination = system.run();
  uint64_t sum =
      std::accumulate(sink.received().begin(), sink.received().end(), 0ULL);
  std::array<SimObject *, 3> objects = {&actor, &channel, &sink};
  return finish(system, std::move(termination), objects,
                {{"consumed_count", sink.totalReceived()},
                 {"consumed_sum", sum},
                 {"transfer_count", channel.transferCount()}},
                channel.transferCount());
}

class MemoryScenarioActor final : public SimObject {
public:
  MemoryScenarioActor(ObjectId id, const RequestResponseMemoryPolicy &policy,
                      RequestResponse<MemoryWorkItem, uint64_t> &protocol,
                      Memory<uint64_t> &memory)
      : SimObject(ObjectKind::Process, "requester", id), policy_(policy),
        protocol_(protocol), memory_(memory) {}

  void doWork(Epoch) override {
    if (nextRequest_ == policy_.requests.size())
      return;
    const MemoryWorkItem &request = policy_.requests[nextRequest_];
    switch (phase_) {
    case Phase::Submit:
      if (protocol_.proposeRequest(request, request.correlationId))
        proposedPhase_ = Phase::ConsumeRequest;
      break;
    case Phase::ConsumeRequest: {
      auto envelope = protocol_.proposePopRequest();
      if (!envelope)
        break;
      if (!memory_.proposeWrite(envelope->payload.address,
                                envelope->payload.value)) {
        setRuntimeFailureCode("showcase_memory_address_out_of_range");
        break;
      }
      proposedPhase_ = Phase::Respond;
      break;
    }
    case Phase::Respond:
      if (protocol_.proposeResponse(request.value, request.correlationId))
        proposedPhase_ = Phase::ConsumeResponse;
      break;
    case Phase::ConsumeResponse:
      if (protocol_.proposePopResponse()) {
        proposedPhase_ = Phase::Submit;
        completeRequest_ = true;
      }
      break;
    }
  }

  void doXfer(Epoch) override {
    if (proposedPhase_)
      phase_ = *proposedPhase_;
    if (completeRequest_) {
      ++nextRequest_;
      ++completed_;
    }
    proposedPhase_.reset();
    completeRequest_ = false;
  }

  bool hasPendingCommit() const override {
    return proposedPhase_.has_value() || completeRequest_;
  }
  RuntimeObjectState runtimeState(Epoch) const override {
    const bool done = nextRequest_ == policy_.requests.size();
    return {.quiescent = done && !hasPendingCommit(),
            .runnable = !done,
            .pendingCommit = hasPendingCommit(),
            .reason = done ? "" : "showcase_memory_protocol"};
  }
  uint64_t completed() const { return completed_; }

private:
  enum class Phase { Submit, ConsumeRequest, Respond, ConsumeResponse };
  const RequestResponseMemoryPolicy &policy_;
  RequestResponse<MemoryWorkItem, uint64_t> &protocol_;
  Memory<uint64_t> &memory_;
  size_t nextRequest_ = 0;
  uint64_t completed_ = 0;
  Phase phase_ = Phase::Submit;
  std::optional<Phase> proposedPhase_;
  bool completeRequest_ = false;
};

ShowcaseResult runRequestResponse(const RequestResponseMemoryPolicy &policy) {
  SimSystem system("showcase");
  RequestResponse<MemoryWorkItem, uint64_t> protocol("request_response", 1,
                                                     nullptr, 2, &system);
  Memory<uint64_t> memory("memory", 2, nullptr, policy.memoryCapacity, &system);
  MemoryScenarioActor actor(0, policy, protocol, memory);
  system.root().attachChild(actor);
  system.root().attachChild(protocol);
  system.root().attachChild(memory);
  std::array rows = {makeDispatchRow(&actor), makeDispatchRow(&protocol),
                     makeDispatchRow(&memory)};
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  TerminationResult termination = system.run();
  std::map<std::string, uint64_t> values = {
      {"completed_responses", actor.completed()}};
  for (size_t address = 0; address < memory.capacity(); ++address)
    values.emplace("memory." + std::to_string(address), memory.read(address));
  std::array<SimObject *, 3> objects = {&actor, &protocol, &memory};
  return finish(system, std::move(termination), objects, std::move(values),
                actor.completed());
}

class NestedArrayActor final : public SimObject {
public:
  NestedArrayActor(ObjectId id, const NestedArraysPolicy &policy,
                   std::span<Queue<uint64_t> *const> queues,
                   std::span<Sink<uint64_t> *const> sinks,
                   ShowcaseWorkOrder order, uint64_t seed)
      : SimObject(ObjectKind::Process, "producer", id), policy_(policy),
        queues_(queues), sinks_(sinks), order_(order), seed_(seed),
        nextValues_(queues.size()), advance_(queues.size()) {}

  void doWork(Epoch epoch) override {
    for (size_t lane : workOrder(queues_.size(), order_, seed_, epoch.time)) {
      if (nextValues_[lane] < policy_.laneValues[lane].size() &&
          queues_[lane]->proposePush(
              policy_.laneValues[lane][nextValues_[lane]]))
        advance_[lane] = true;
      if (auto value = queues_[lane]->proposePop())
        sinks_[lane]->receive(*value);
    }
  }

  void doXfer(Epoch) override {
    for (size_t lane = 0; lane < advance_.size(); ++lane) {
      if (advance_[lane])
        ++nextValues_[lane];
      advance_[lane] = false;
    }
  }

  bool hasPendingCommit() const override {
    return std::ranges::find(advance_, true) != advance_.end();
  }
  RuntimeObjectState runtimeState(Epoch) const override {
    bool done = true;
    for (size_t lane = 0; lane < queues_.size(); ++lane)
      done = done && nextValues_[lane] == policy_.laneValues[lane].size() &&
             queues_[lane]->isEmpty();
    return {.quiescent = done && !hasPendingCommit(),
            .runnable = !done,
            .pendingCommit = hasPendingCommit(),
            .reason = done ? "" : "showcase_nested_arrays"};
  }

private:
  const NestedArraysPolicy &policy_;
  std::span<Queue<uint64_t> *const> queues_;
  std::span<Sink<uint64_t> *const> sinks_;
  ShowcaseWorkOrder order_;
  uint64_t seed_;
  std::vector<size_t> nextValues_;
  std::vector<bool> advance_;
};

ShowcaseResult runNestedArrays(const NestedArraysPolicy &policy,
                               ShowcaseWorkOrder order, uint64_t seed) {
  SimSystem system("showcase");
  Module lanes("lanes", kInvalidObjectId);
  system.root().attachChild(lanes);
  std::vector<std::unique_ptr<Module>> laneModules;
  std::vector<std::unique_ptr<Queue<uint64_t>>> queueStorage;
  std::vector<std::unique_ptr<Sink<uint64_t>>> sinkStorage;
  std::vector<Queue<uint64_t> *> queues;
  std::vector<Sink<uint64_t> *> sinks;
  std::vector<SimObject *> objects;
  std::vector<DispatchRow> rows(policy.laneValues.size() * 2 + 1);
  laneModules.reserve(policy.laneValues.size());
  for (size_t lane = 0; lane < policy.laneValues.size(); ++lane) {
    auto module =
        std::make_unique<Module>(std::to_string(lane), kInvalidObjectId);
    lanes.attachChild(*module);
    ObjectId queueId = static_cast<ObjectId>(1 + lane * 2);
    ObjectId sinkId = queueId + 1;
    auto queue = std::make_unique<Queue<uint64_t>>(
        "queue", queueId, nullptr, policy.queueCapacity, SIZE_MAX, &system);
    auto sink =
        std::make_unique<Sink<uint64_t>>("sink", sinkId, nullptr, &system);
    module->attachChild(*queue);
    module->attachChild(*sink);
    queues.push_back(queue.get());
    sinks.push_back(sink.get());
    rows[queueId] = makeDispatchRow(queue.get());
    rows[sinkId] = makeDispatchRow(sink.get());
    queueStorage.push_back(std::move(queue));
    sinkStorage.push_back(std::move(sink));
    laneModules.push_back(std::move(module));
  }
  NestedArrayActor actor(0, policy, queues, sinks, order, seed);
  system.root().attachChild(actor);
  rows[0] = makeDispatchRow(&actor);
  objects.push_back(&actor);
  for (size_t lane = 0; lane < queues.size(); ++lane) {
    objects.push_back(queues[lane]);
    objects.push_back(sinks[lane]);
  }
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  TerminationResult termination = system.run();
  std::map<std::string, uint64_t> values;
  uint64_t tracePosition = 0;
  for (size_t lane = 0; lane < sinks.size(); ++lane) {
    uint64_t sum = std::accumulate(sinks[lane]->received().begin(),
                                   sinks[lane]->received().end(), 0ULL);
    values.emplace("lane." + std::to_string(lane) + ".sum", sum);
    tracePosition += sinks[lane]->totalReceived();
  }
  return finish(system, std::move(termination), objects, std::move(values),
                tracePosition);
}

class TimeDomainActor final : public SimObject {
public:
  TimeDomainActor(ObjectId id, const MultiTimeDomainBridgePolicy &policy,
                  ReadyValid<uint64_t> &bridge, Sink<uint64_t> &sink)
      : SimObject(ObjectKind::Process, "source", id), policy_(policy),
        bridge_(bridge), sink_(sink) {}

  void doWork(Epoch epoch) override {
    if (bridge_.transferCount() > observedTransfers_) {
      sink_.receive(bridge_.lastTransferred());
      proposedObservedTransfers_ = bridge_.transferCount();
      proposedLastTransferTick_ = epoch.time == 0 ? 0 : epoch.time - 1;
    }
    if (nextValue_ < policy_.values.size() && !bridge_.hasOffer() &&
        epoch.time % policy_.sourcePeriod == 0 &&
        bridge_.proposeOffer(policy_.values[nextValue_]))
      advance_ = true;
    if (bridge_.transferCount() < policy_.values.size() ||
        nextValue_ < policy_.values.size() || bridge_.hasOffer())
      bridge_.proposeReady(epoch.time % policy_.targetPeriod == 0);
  }

  void doXfer(Epoch) override {
    if (advance_)
      ++nextValue_;
    if (proposedObservedTransfers_) {
      observedTransfers_ = *proposedObservedTransfers_;
      lastTransferTick_ = *proposedLastTransferTick_;
    }
    advance_ = false;
    proposedObservedTransfers_.reset();
    proposedLastTransferTick_.reset();
  }
  bool hasPendingCommit() const override {
    return advance_ || proposedObservedTransfers_.has_value();
  }
  RuntimeObjectState runtimeState(Epoch) const override {
    const bool done = observedTransfers_ == policy_.values.size() &&
                      nextValue_ == policy_.values.size() &&
                      !bridge_.hasOffer();
    return {.quiescent = done && !hasPendingCommit(),
            .runnable = !done,
            .pendingCommit = hasPendingCommit(),
            .reason = done ? "" : "showcase_time_domain_bridge"};
  }
  Tick lastTransferTick() const { return lastTransferTick_; }

private:
  const MultiTimeDomainBridgePolicy &policy_;
  ReadyValid<uint64_t> &bridge_;
  Sink<uint64_t> &sink_;
  size_t nextValue_ = 0;
  uint64_t observedTransfers_ = 0;
  Tick lastTransferTick_ = 0;
  bool advance_ = false;
  std::optional<uint64_t> proposedObservedTransfers_;
  std::optional<Tick> proposedLastTransferTick_;
};

ShowcaseResult runTimeDomains(const MultiTimeDomainBridgePolicy &policy) {
  SimSystem system("showcase");
  ReadyValid<uint64_t> bridge("bridge", 1, nullptr, &system);
  Sink<uint64_t> sink("consumer", 2, nullptr, &system);
  TimeDomainActor actor(0, policy, bridge, sink);
  system.root().attachChild(actor);
  system.root().attachChild(bridge);
  system.root().attachChild(sink);
  std::array rows = {makeDispatchRow(&actor), makeDispatchRow(&bridge),
                     makeDispatchRow(&sink)};
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  const std::array domains = {
      TimeDomainRuntime{"source", policy.sourcePeriod, 0, 1},
      TimeDomainRuntime{"target", policy.targetPeriod, 0, 1}};
  system.setTimeDomains(domains);
  TerminationResult termination = system.run();
  uint64_t sum =
      std::accumulate(sink.received().begin(), sink.received().end(), 0ULL);
  std::array<SimObject *, 3> objects = {&actor, &bridge, &sink};
  return finish(system, std::move(termination), objects,
                {{"bridged_count", sink.totalReceived()},
                 {"bridged_sum", sum},
                 {"last_transfer_tick", actor.lastTransferTick()}},
                sink.totalReceived());
}

class ShowcaseProcess final : public ProcessRuntime<ShowcaseProcess> {
public:
  explicit ShowcaseProcess(const SuspendedProcessPolicy &policy)
      : ProcessRuntime("process", 0, nullptr, 0, 2), policy_(policy) {}

  ProcessStep executeProcessStep(uint32_t pc, Epoch) {
    if (pc == 0) {
      proposedValue_ = policy_.initialValue;
      return ProcessStep::suspendAt(
          1, {.kind = ProcessWakeKind::EventQueue, .id = 7}, 42);
    }
    proposedValue_ = liveValue_ + policy_.incrementAfterWake;
    proposedResume_ = true;
    return ProcessStep::terminate();
  }

  void doWork(Epoch epoch) override {
    const uint32_t activePc = pc();
    ProcessRuntime::doWork(epoch);
    if (!hasPendingCommit())
      return;
    if (activePc == 0)
      emitObservation({.category = "process",
                       .name = "suspended",
                       .phase = TraceEventPhase::Instant});
    else
      emitObservation({.category = "process",
                       .name = "resumed",
                       .phase = TraceEventPhase::Instant});
  }

  void doXfer(Epoch epoch) override {
    const bool commitValue = hasPendingCommit();
    ProcessRuntime::doXfer(epoch);
    if (commitValue) {
      liveValue_ = proposedValue_;
      if (proposedResume_)
        ++resumeCount_;
    }
    proposedResume_ = false;
  }
  uint64_t result() const { return liveValue_; }
  uint64_t resumeCount() const { return resumeCount_; }

private:
  const SuspendedProcessPolicy &policy_;
  uint64_t liveValue_ = 0;
  uint64_t proposedValue_ = 0;
  uint64_t resumeCount_ = 0;
  bool proposedResume_ = false;
};

class WakeDriver final : public SimObject {
public:
  WakeDriver(SimSystem &system, ShowcaseProcess &process, Tick wakeTick)
      : SimObject(ObjectKind::Process, "wake", 1), system_(system),
        process_(process), wakeTick_(wakeTick) {}

  void doWork(Epoch epoch) override {
    if (!scheduled_) {
      system_.scheduleEvent({{wakeTick_, 0}, id(), 7, 42});
      pendingSchedule_ = true;
      return;
    }
    if (epoch.time == wakeTick_ &&
        process_.wake({ProcessWakeKind::EventQueue, 7}, 42)) {
      system_.scheduleWork(process_.id(), epoch);
      emitObservation({.category = "process",
                       .name = "wake",
                       .phase = TraceEventPhase::Instant});
      pendingWake_ = true;
    }
  }
  void doXfer(Epoch) override {
    if (pendingSchedule_)
      scheduled_ = true;
    pendingSchedule_ = false;
    pendingWake_ = false;
  }
  bool hasPendingCommit() const override {
    return pendingSchedule_ || pendingWake_;
  }

private:
  SimSystem &system_;
  ShowcaseProcess &process_;
  Tick wakeTick_;
  bool scheduled_ = false;
  bool pendingSchedule_ = false;
  bool pendingWake_ = false;
};

ShowcaseResult runSuspended(const SuspendedProcessPolicy &policy) {
  SimSystem system("showcase");
  ShowcaseProcess process(policy);
  WakeDriver wake(system, process, policy.wakeTick);
  system.root().attachChild(process);
  system.root().attachChild(wake);
  std::array rows = {makeDispatchRow(&process), makeDispatchRow(&wake)};
  std::vector<uint32_t> offsets;
  std::vector<ObjectId> targets;
  installRuntime(system, rows, offsets, targets);
  TerminationResult termination = system.run();
  std::array<SimObject *, 2> objects = {&process, &wake};
  return finish(system, std::move(termination), objects,
                {{"process_result", process.result()},
                 {"resume_count", process.resumeCount()},
                 {"wake_tick", policy.wakeTick}},
                0);
}

template <typename T>
void appendOptional(std::ostringstream &output, const std::optional<T> &value) {
  if (value)
    output << *value;
  else
    output << '-';
}

void appendObservationValue(std::ostringstream &output,
                            const ObservationValue &value) {
  std::visit([&](const auto &item) { output << item; }, value);
}

} // namespace

ShowcaseTraceSource::ShowcaseTraceSource(std::string name, ObjectId id,
                                         SimObject *parent, uint64_t scenario)
    : SimObject(ObjectKind::TraceSource, std::move(name), id, parent),
      scenario_(scenario) {}

bool ShowcaseTraceSource::loadDocument(PtoTraceDocument document) {
  if (loaded_ || pending_ || committed_)
    return false;
  document_ = std::move(document);
  loaded_ = true;
  return true;
}

void ShowcaseTraceSource::doWork(Epoch) {
  if (pending_ || committed_)
    return;
  if (!loaded_) {
    setRuntimeFailureCode("showcase_trace_not_loaded");
    return;
  }
  ShowcasePolicy policy;
  switch (scenario_) {
  case 0:
    policy = ProducerQueueConsumerPolicy{};
    break;
  case 1:
    policy = BackpressuredPipelinePolicy{};
    break;
  case 2:
    policy = RequestResponseMemoryPolicy{};
    break;
  case 3:
    policy = NestedArraysPolicy{};
    break;
  case 4:
    policy = MultiTimeDomainBridgePolicy{};
    break;
  case 5:
    policy = SuspendedProcessPolicy{};
    break;
  default:
    setRuntimeFailureCode("invalid_showcase_scenario");
    return;
  }
  result_ = runShowcase(policy, ShowcaseWorkOrder::Ascending);
  if (result_.termination.classification != TerminationClass::Completed) {
    setRuntimeFailureCode(result_.termination.diagnosticCode.empty()
                              ? "showcase_execution_failed"
                              : result_.termination.diagnosticCode);
    return;
  }
  for (const CommittedEvent &event : result_.events) {
    std::vector<ObservationArgument> arguments = event.arguments;
    arguments.push_back({.name = "showcase_epoch_delta",
                         .value = static_cast<uint64_t>(event.epoch.delta)});
    arguments.push_back(
        {.name = "showcase_epoch_time", .value = event.epoch.time});
    if (!emitObservation({.category = event.category,
                          .name = event.name,
                          .phase = event.phase,
                          .rootSequenceId = event.rootSequenceId,
                          .duration = event.duration,
                          .flowId = event.flowId,
                          .arguments = std::move(arguments)}))
      return;
  }
  pending_ = true;
}

void ShowcaseTraceSource::doXfer(Epoch epoch) {
  if (!pending_)
    return;
  pending_ = false;
  committed_ = true;
  lastUpdate_ = epoch;
}

bool ShowcaseTraceSource::hasPendingCommit() const { return pending_; }

RuntimeObjectState ShowcaseTraceSource::runtimeState(Epoch) const {
  return {.quiescent = committed_ && !pending_,
          .runnable = loaded_ && !committed_ && !pending_,
          .pendingCommit = pending_,
          .reason = committed_ ? "" : "showcase_trace_pending",
          .traceOwner = true,
          .tracePosition = committed_ ? document_.records.size() : 0,
          .traceLastCommittedSequenceId =
              committed_ && !document_.records.empty()
                  ? std::optional<uint64_t>(document_.records.back().sequenceId)
                  : std::nullopt,
          .traceEof = committed_};
}

void ShowcaseTraceSource::collectStatistics(
    std::vector<StatSnapshot> &out) const {
  if (!committed_)
    return;
  auto append = [&](std::string name, uint64_t value) {
    std::replace(name.begin(), name.end(), '.', '_');
    out.push_back({.name = std::move(name),
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = value,
                   .lastUpdate = lastUpdate_});
  };
  append("trace_records", document_.records.size());
  append("showcase_events", result_.events.size());
  for (const auto &[name, value] : result_.architecturalValues)
    append("architectural_" + name, value);
}

void ShowcaseTraceSource::reset() {
  document_ = {};
  result_ = {};
  loaded_ = false;
  pending_ = false;
  committed_ = false;
  lastUpdate_ = {};
  clearRuntimeFailureCode();
}

bool ShowcaseTraceSource::validate() const { return scenario_ < 6; }

ShowcaseResult runShowcase(const ShowcasePolicy &policy,
                           ShowcaseWorkOrder order, uint64_t permutationSeed) {
  return std::visit(
      [&](const auto &typedPolicy) -> ShowcaseResult {
        using Policy = std::decay_t<decltype(typedPolicy)>;
        if constexpr (std::is_same_v<Policy, ProducerQueueConsumerPolicy>)
          return runProducerQueueConsumer(typedPolicy);
        else if constexpr (std::is_same_v<Policy, BackpressuredPipelinePolicy>)
          return runBackpressured(typedPolicy);
        else if constexpr (std::is_same_v<Policy, RequestResponseMemoryPolicy>)
          return runRequestResponse(typedPolicy);
        else if constexpr (std::is_same_v<Policy, NestedArraysPolicy>)
          return runNestedArrays(typedPolicy, order, permutationSeed);
        else if constexpr (std::is_same_v<Policy, MultiTimeDomainBridgePolicy>)
          return runTimeDomains(typedPolicy);
        else
          return runSuspended(typedPolicy);
      },
      policy);
}

std::string canonicalShowcaseResult(const ShowcaseResult &result) {
  std::ostringstream output;
  const auto &termination = result.termination;
  output << "termination|" << static_cast<unsigned>(termination.classification)
         << '|' << termination.finalEpoch.time << '|'
         << termination.finalEpoch.delta << '|'
         << termination.committedEventCount << '|' << termination.tracePosition
         << '|';
  appendOptional(output, termination.traceLastCommittedSequenceId);
  output << '|';
  appendOptional(output, termination.terminationCap);
  output << '|' << termination.diagnosticCode << '|';
  appendOptional(output, termination.message);
  output << '\n';
  for (const auto &[name, cycles] : termination.domainCycles)
    output << "domain|" << name << '|' << cycles << '\n';
  for (const auto &[name, value] : result.architecturalValues)
    output << "value|" << name << '|' << value << '\n';
  output << "trace|" << result.tracePosition << '\n';
  for (const auto &entry : result.hierarchy)
    output << "hierarchy|" << entry.id << '|' << entry.path << '|'
           << static_cast<unsigned>(entry.kind) << '\n';
  for (const auto &stat : result.statistics) {
    output << "stat|" << stat.objectPath << '|' << stat.name << '|'
           << static_cast<unsigned>(stat.kind) << '|' << stat.value << '|'
           << stat.count << '|' << stat.sum << '|' << stat.minimum << '|'
           << stat.maximum << '|' << stat.lastUpdate.time << '|'
           << stat.lastUpdate.delta;
    for (const auto &bucket : stat.buckets)
      output << '|' << bucket.upperBound << ':' << bucket.count;
    output << '\n';
  }
  for (const auto &event : result.events) {
    output << "event|" << event.epoch.time << '|' << event.epoch.delta << '|'
           << event.ownerId << '|' << event.localCommittedIndex << '|'
           << event.category << '|' << event.name << '|'
           << static_cast<unsigned>(event.phase) << '|';
    appendOptional(output, event.rootSequenceId);
    output << '|';
    appendOptional(output, event.duration);
    output << '|';
    appendOptional(output, event.flowId);
    for (const auto &argument : event.arguments) {
      output << '|' << argument.name << '=';
      appendObservationValue(output, argument.value);
    }
    output << '\n';
  }
  return output.str();
}

} // namespace gfsim
