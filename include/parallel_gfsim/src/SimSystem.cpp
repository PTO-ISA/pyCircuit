#include "parallel_gfsim/SimSystem.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>

#include "parallel_gfsim/SimModule.h"
#include "SimExecutor.h"

namespace parallel_gfsim {

SimSystem::SimSystem() = default;
SimSystem::~SimSystem()
{
    // Join before destroying any module, queue, logger, or trace.
    executor_.reset();
}

void SimSystem::SetWorkerCount(std::size_t count)
{
    if (buildAttempted_) throw std::logic_error("worker configuration is frozen after Build starts");
    if (count == 0) throw std::invalid_argument("worker count must be positive");
    workerCount_ = count;
}

void SimSystem::SetParallelChildren(SimModule* parent)
{
    if (buildAttempted_) throw std::logic_error("parallel split is frozen after Build starts");
    if (!parent) throw std::invalid_argument("parallel parent must not be null");
    parallelParent_ = parent;
}

bool SimSystem::ExecuteChildren(SimModule* parent, bool work)
{
    if (parent != parallelParent_ || !executor_) return false;
    executor_->Execute(work ? SimPhase::Work : SimPhase::Xfer);
    return true;
}

bool SimSystem::DelegatesQueue(const SimObj* queue) const
{
    return executor_ && delegatedQueues_.count(queue) != 0;
}

void SimSystem::AddModule(std::shared_ptr<SimObj> module)
{
    if (buildAttempted_) {
        throw std::logic_error("cannot add a root object after Build");
    }
    if (!module) {
        throw std::invalid_argument("root object must not be null");
    }
    if (std::find(modules_.begin(), modules_.end(), module) != modules_.end()) {
        throw std::invalid_argument("root object is already registered");
    }
    if (dynamic_cast<TraceableQueue*>(module.get()) != nullptr) {
        throw std::invalid_argument("queues must be owned by a parent module");
    }
    modules_.push_back(module);
}

void SimSystem::RegisterTraceQueue(TraceableQueue& queue, const std::string& path)
{
    if (path.empty()) {
        throw std::invalid_argument("queue trace path must not be empty");
    }
    for (const auto& entry : traceQueues_) {
        if (entry.queue == &queue) {
            return;
        }
    }
    std::string unique = path;
    for (std::size_t suffix = 2;; ++suffix) {
        const auto found = std::find_if(traceQueues_.begin(), traceQueues_.end(),
                                        [&unique](const TraceQueueRegistration& entry) {
                                            return entry.path == unique;
                                        });
        if (found == traceQueues_.end()) {
            break;
        }
        unique = path + "#" + std::to_string(suffix);
    }
    if (trace_) {
        trace_->RegisterQueue(queue, unique);
    }
    traceQueues_.push_back(TraceQueueRegistration{&queue, unique});
}

void SimSystem::EnableTrace(std::ostream& output, std::uint64_t sampleEvery)
{
    if (!built_) {
        throw std::logic_error("call Build before EnableTrace");
    }
    if (trace_) {
        throw std::logic_error("trace is already enabled");
    }
    std::unique_ptr<SimTrace> trace(new SimTrace(output, sampleEvery));
    for (const auto& entry : traceQueues_) {
        trace->RegisterQueue(*entry.queue, entry.path);
    }
    trace->Capture(cycles_);
    trace_ = std::move(trace);
}

void SimSystem::DisableTrace()
{
    if (trace_) {
        trace_->Close();
        trace_.reset();
    }
}

void SimSystem::Build()
{
    if (buildAttempted_) {
        throw std::logic_error("SimSystem build has already been attempted");
    }
    buildAttempted_ = true;
    if (modules_.empty()) {
        throw std::logic_error("SimSystem has no root objects to build");
    }
    if (workerCount_ > 1 && !parallelParent_) {
        throw std::logic_error("multiple workers require a parallel parent");
    }
    for (const auto& module : modules_) {
        module->Build();
    }
    for (const auto& module : modules_) {
        module->FreezeObjects();
    }

    struct QueueUse {
        std::size_t owners = 0;
        std::size_t producers = 0;
        std::size_t consumers = 0;
        const SimModule* producer = nullptr;
        const SimModule* consumer = nullptr;
    };
    std::unordered_map<const SimObj*, QueueUse> queues;
    std::vector<SimObj*> queueOrder;
    std::vector<const SimModule*> moduleOrder;
    std::unordered_set<const SimModule*> moduleSet;
    bool foundParallelParent = false;
    std::function<void(const SimModule*)> collect;
    collect = [&](const SimModule* parent) {
        moduleOrder.push_back(parent);
        moduleSet.insert(parent);
        if (parent == parallelParent_) foundParallelParent = true;
        for (const auto& queue : parent->innerQueues_) {
            ++queues[queue.get()].owners;
            queueOrder.push_back(queue.get());
        }
        for (const SimObj* queue : parent->outputs_) {
            ++queues[queue].producers;
            queues[queue].producer = parent;
        }
        for (const SimObj* queue : parent->inputs_) {
            ++queues[queue].consumers;
            queues[queue].consumer = parent;
        }
        for (const auto& child : parent->submodules_) {
            collect(child.get());
        }
    };
    for (const auto& root : modules_) {
        if (const auto* parent = dynamic_cast<const SimModule*>(root.get())) {
            collect(parent);
        }
    }
    // Check references only after every root and descendant has been collected.
    // Following references here would recurse through legal self/cyclic bindings.
    for (const SimModule* caller : moduleOrder) {
        for (const SimModule* target : caller->moduleTargets_) {
            if (moduleSet.count(target) == 0) {
                throw std::logic_error("module binding from " + caller->Path() +
                                       " targets a module outside this system");
            }
        }
    }
    std::function<void(const SimObj*)> collectPhysicalQueues;
    collectPhysicalQueues = [&](const SimObj* object) {
        if (dynamic_cast<const TraceableQueue*>(object) != nullptr) {
            queues[object];
        }
        for (const SimObj* child : object->Objects()) {
            collectPhysicalQueues(child);
        }
    };
    for (const auto& root : modules_) {
        collectPhysicalQueues(root.get());
    }
    for (const auto& entry : queues) {
        const QueueUse& use = entry.second;
        if (use.owners != 1 || use.producers != 1 || use.consumers != 1) {
            std::ostringstream message;
            message << "queue " << entry.first->Name()
                    << " requires one parent owner, one output, one input; got "
                    << use.owners << "/" << use.producers << "/"
                    << use.consumers;
            throw std::logic_error(message.str());
        }
    }
    if (parallelParent_ && (!foundParallelParent || parallelParent_->GetSim() != this)) {
        throw std::logic_error("parallel parent must belong to this system and expose its context");
    }
    built_ = true;
    try {
        Reset();
        if (parallelParent_) {
            SimExecutionPlan plan(parallelParent_, workerCount_);
            for (SimObj* queue : queueOrder) {
                const auto& use = queues.at(queue);
                if (plan.AssignQueue(queue, use.producer, use.consumer)) {
                    delegatedQueues_.insert(queue);
                }
            }
            executor_.reset(new SimExecutor(this, std::move(plan)));
        }
    } catch (...) {
        delegatedQueues_.clear();
        built_ = false;
        throw;
    }
}

void SimSystem::Step()
{
    if (!built_) {
        throw std::logic_error("call Build before Step");
    }
    if (failed_) throw std::logic_error("parallel step failed; call Reset before stepping");
    if (terminated_) {
        throw std::logic_error("simulation has terminated");
    }
    if (deadlocked_) {
        throw DeadlockError("simulation is deadlocked; call Reset before stepping");
    }
    ++cycles_;
    logger_.SetCycle(cycles_);
    progressThisStep_.store(false, std::memory_order_relaxed);
    stepping_ = true;
    try {
        {
            SimTaskScope scope(this, SimPhase::Work, 0);
            for (const auto& module : modules_) module->Work();
        }
        {
            SimTaskScope scope(this, SimPhase::Xfer, 0);
            for (const auto& module : modules_) module->Xfer();
        }
        stepping_ = false;
        if (trace_) trace_->Capture(cycles_);
        PublishTermination();
        stalledCycles_ = progressThisStep_.load(std::memory_order_relaxed) ? 0 : stalledCycles_ + 1;
        CheckDeadlock();
    } catch (...) {
        stepping_ = false;
        if (parallelParent_) {
            failed_ = true;
            pendingTermination_ = false;
            pendingReason_.clear();
        }
        throw;
    }
}

void SimSystem::Run(std::uint64_t maxSteps)
{
    if (maxSteps == 0) {
        throw std::invalid_argument("maxSteps must be positive");
    }
    for (std::uint64_t step = 0; step < maxSteps && !NeedTerminate(); ++step) {
        Step();
    }
    if (!NeedTerminate()) {
        throw std::runtime_error("simulation reached the step limit before termination");
    }
}

void SimSystem::Reset()
{
    if (!built_) {
        throw std::logic_error("call Build before Reset");
    }
    if (trace_) {
        throw std::logic_error("disable trace before Reset");
    }
    if (parallelParent_) failed_ = true;
    cycles_ = 0;
    stalledCycles_ = 0;
    progressThisStep_.store(false, std::memory_order_relaxed);
    stepping_ = false;
    terminated_ = false;
    deadlocked_ = false;
    terminationReason_.clear();
    pendingTermination_ = false;
    pendingReason_.clear();
    logger_.SetCycle(0);
    for (const auto& module : modules_) {
        module->Reset();
    }
    failed_ = false;
}

void SimSystem::ReportStat()
{
    if (!built_) {
        throw std::logic_error("call Build before ReportStat");
    }
    for (const auto& module : modules_) {
        module->ReportStat();
    }
}

void SimSystem::RequestTerminate(std::string reason)
{
    if (!built_) {
        throw std::logic_error("call Build before RequestTerminate");
    }
    if (parallelParent_ && stepping_) {
        const auto& context = CurrentSimTask();
        if (context.system != this) {
            throw std::logic_error("termination during Step requires a model task context");
        }
        const unsigned phase = context.phase == SimPhase::Work ? 0 : 1;
        std::lock_guard<std::mutex> lock(terminationMutex_);
        if (!pendingTermination_ || phase < pendingPhase_ ||
            (phase == pendingPhase_ && context.task < pendingTask_)) {
            pendingReason_ = std::move(reason);
            pendingPhase_ = phase;
            pendingTask_ = context.task;
            pendingTermination_ = true;
        }
        return;
    }
    terminated_ = true;
    terminationReason_ = std::move(reason);
    logger_.Info("system", "terminate") << terminationReason_;
}

void SimSystem::PublishTermination()
{
    // Workers have completed; no model code can be issuing requests here.
    if (pendingTermination_) {
        RequestTerminate(std::move(pendingReason_));
        pendingTermination_ = false;
        pendingReason_.clear();
    }
}

void SimSystem::MarkProgress()
{
    if (!stepping_) {
        throw std::logic_error("MarkProgress is valid only during Step");
    }
    progressThisStep_.store(true, std::memory_order_relaxed);
}

void SimSystem::SetDeadlockThreshold(std::uint64_t cycles)
{
    deadlockThreshold_ = cycles;
}

void SimSystem::CheckDeadlock()
{
    if (deadlocked_) {
        throw DeadlockError("simulation is deadlocked; call Reset before stepping");
    }
    if (terminated_ || deadlockThreshold_ == 0 || stalledCycles_ < deadlockThreshold_) {
        return;
    }
    std::ostringstream message;
    message << "deadlock at cycle " << cycles_ << ": no progress for "
            << stalledCycles_ << " cycles";
    deadlocked_ = true;
    logger_.Error("system", "deadlock") << message.str();
    throw DeadlockError(message.str());
}

} // namespace parallel_gfsim
