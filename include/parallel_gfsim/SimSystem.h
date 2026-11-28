#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

#include "Logger.h"
#include "Reporter.h"
#include "SimMemory.h"
#include "SimTrace.h"

namespace parallel_gfsim {

class SimSystem;
class SimModule;
class SimExecutor;

// Common interface for every queue and module in a simulation.
class SimObj {
public:
    explicit SimObj(std::string name) : name_(std::move(name))
    {
        if (name_.empty()) {
            throw std::invalid_argument("simulation object name must not be empty");
        }
    }
    virtual ~SimObj() = default;

    const std::string& Name() const { return name_; }
    const std::vector<SimObj*>& Objects() const { return objects_; }
    virtual void Build() = 0;
    virtual void Work() = 0;
    virtual void Xfer() = 0;
    virtual void Reset() = 0;
    virtual void ReportStat() = 0;
    virtual SimSystem* GetSim() { return nullptr; }

protected:
    void SetObjectCapacity(std::size_t capacity)
    {
        if (frozen_ || capacityDeclared_ || !objects_.empty()) {
            throw std::logic_error("object capacity must be set once during Build");
        }
        objects_.reserve(capacity);
        objectCapacity_ = capacity;
        capacityDeclared_ = true;
    }

    void EnsureObjectSlot() const
    {
        if (frozen_ || objects_.size() >= objectCapacity_) {
            throw std::logic_error("SimObj collection capacity exceeded or frozen");
        }
    }

    void AddObject(SimObj* object)
    {
        EnsureObjectSlot();
        if (object == nullptr) {
            throw std::invalid_argument("child simulation object must not be null");
        }
        objects_.push_back(object);
    }

    bool ObjectsFrozen() const { return frozen_; }

private:
    friend class SimSystem;

    void FreezeObjects()
    {
        if (objects_.size() != objectCapacity_) {
            throw std::logic_error("SimObj collection does not match Build capacity");
        }
        frozen_ = true;
        for (SimObj* object : objects_) {
            object->FreezeObjects();
        }
    }

    std::string name_;
    std::size_t objectCapacity_ = 0;
    bool capacityDeclared_ = false;
    bool frozen_ = false;
    std::vector<SimObj*> objects_;
};

class DeadlockError : public std::runtime_error {
public:
    explicit DeadlockError(const std::string& message) : std::runtime_error(message) {}
};

// Run all Work phases before publishing state through Xfer.
class SimSystem {
public:
    SimSystem();
    ~SimSystem();
    SimSystem(const SimSystem&) = delete;
    SimSystem& operator=(const SimSystem&) = delete;
    SimSystem(SimSystem&&) = delete;
    SimSystem& operator=(SimSystem&&) = delete;

    // Configure before Build starts. A split is opt-in even with one worker.
    void SetWorkerCount(std::size_t count);
    void SetParallelChildren(SimModule* parent);
    void AddModule(std::shared_ptr<SimObj> module);
    void Build();
    void Step();
    void Run(std::uint64_t maxSteps);
    void Reset();
    void ReportStat();
    void WriteDot(std::ostream& output) const;
    void SaveDot(const std::string& path) const;
    void EnableTrace(std::ostream& output, std::uint64_t sampleEvery = 1);
    void DisableTrace();

    void RequestTerminate(std::string reason = "requested");
    void MarkProgress();
    void SetDeadlockThreshold(std::uint64_t cycles);
    void CheckDeadlock();

    std::uint64_t Cycles() const { return cycles_; }
    bool IsBuilt() const { return built_; }
    bool NeedTerminate() const { return terminated_; }
    bool IsDeadlocked() const { return deadlocked_; }
    const std::string& TerminationReason() const { return terminationReason_; }
    std::uint64_t DeadlockThreshold() const { return deadlockThreshold_; }
    std::uint64_t StalledCycles() const { return stalledCycles_; }
    Reporter& GetReporter() { return reporter_; }
    Logger& GetLogger() { return logger_; }
    SimMemory& GetMemory() { return memory_; }
    SimTrace* GetTrace() { return trace_.get(); }

private:
    friend class SimModule;
    void RegisterTraceQueue(TraceableQueue& queue, const std::string& path);
    bool ExecuteChildren(SimModule* parent, bool work);
    bool DelegatesQueue(const SimObj* queue) const;
    void PublishTermination();

    std::uint64_t cycles_ = 0;
    std::uint64_t deadlockThreshold_ = 0;
    std::uint64_t stalledCycles_ = 0;
    bool built_ = false;
    bool buildAttempted_ = false;
    bool terminated_ = false;
    bool deadlocked_ = false;
    std::atomic<bool> progressThisStep_{false};
    bool stepping_ = false;
    bool failed_ = false;
    std::size_t workerCount_ = 1;
    SimModule* parallelParent_ = nullptr;
    std::mutex terminationMutex_;
    bool pendingTermination_ = false;
    unsigned pendingPhase_ = 0;
    std::size_t pendingTask_ = 0;
    std::string pendingReason_;
    std::string terminationReason_;
    Reporter reporter_;
    Logger logger_;
    SimMemory memory_;
    struct TraceQueueRegistration {
        TraceableQueue* queue;
        std::string path;
    };
    std::vector<TraceQueueRegistration> traceQueues_;
    std::unique_ptr<SimTrace> trace_;
    std::vector<std::shared_ptr<SimObj>> modules_;
    std::unique_ptr<SimExecutor> executor_;
    std::unordered_set<const SimObj*> delegatedQueues_;
};

// Short name for the simulation system.
using SimSys = SimSystem;

} // namespace parallel_gfsim
