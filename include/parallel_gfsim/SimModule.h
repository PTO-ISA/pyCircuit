#pragma once

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "SimQueue.h"

// Port role markers. BindInput/BindOutput/AddInnerQueue check the roles.
#ifndef PARALLEL_GFSIM_INPUT
#define PARALLEL_GFSIM_INPUT
#endif
#ifndef PARALLEL_GFSIM_OUTPUT
#define PARALLEL_GFSIM_OUTPUT
#endif
#ifndef PARALLEL_GFSIM_INNER
#define PARALLEL_GFSIM_INNER
#endif
#ifndef PARALLEL_GFSIM_PURE
#define PARALLEL_GFSIM_PURE
#endif

namespace parallel_gfsim {

class SimModule : public SimObj {
public:
    explicit SimModule(std::string name, SimSystem* system = nullptr)
        : SimObj(std::move(name)), system_(system) {}
    ~SimModule() override = default;
    SimModule(const SimModule&) = delete;
    SimModule& operator=(const SimModule&) = delete;
    SimModule(SimModule&&) = delete;
    SimModule& operator=(SimModule&&) = delete;

    SimModule* Parent() const { return parent_; }
    std::string Path() const
    {
        return parent_ == nullptr ? Name() : parent_->Path() + "/" + Name();
    }
    SimSystem* GetSim() override
    {
        return system_ != nullptr ? system_ : (parent_ != nullptr ? parent_->GetSim() : nullptr);
    }

    void AddSubmodule(std::shared_ptr<SimModule> child)
    {
        EnsureObjectSlot();
        if (!child || child->parent_ != nullptr) {
            throw std::invalid_argument("submodule must be non-null and have no parent");
        }
        if (child->system_ != nullptr && child->system_ != GetSim()) {
            throw std::invalid_argument("submodule belongs to another system");
        }
        for (const SimModule* ancestor = this; ancestor != nullptr; ancestor = ancestor->parent_) {
            if (ancestor == child.get()) {
                throw std::invalid_argument("submodule would create a cycle");
            }
        }
        submodules_.push_back(std::move(child));
        submodules_.back()->parent_ = this;
        AddObject(submodules_.back().get());
    }

    const std::vector<std::shared_ptr<SimModule>>& Submodules() const { return submodules_; }

    void Build() override = 0;
    void Work() override = 0;
    void Xfer() override = 0;
    void Reset() override = 0;
    void ReportStat() override = 0;

protected:
    // Records topology only. Ordinary methods still execute on the caller thread.
    template <typename T>
    T* BindModule(T* target)
    {
        static_assert(std::is_base_of<SimModule, T>::value,
                      "module targets must derive from SimModule");
        if (ObjectsFrozen()) {
            throw std::logic_error("module bindings are frozen after Build");
        }
        const SimModule* module = target;
        if (module == nullptr ||
            std::find(moduleTargets_.begin(), moduleTargets_.end(), module) != moduleTargets_.end()) {
            throw std::invalid_argument("module target must be unique and non-null");
        }
        moduleTargets_.push_back(module);
        return target;
    }

    void BuildChildren()
    {
        for (const auto& child : submodules_) {
            child->Build();
        }
    }

    void BuildInnerQueues()
    {
        for (const auto& queue : innerQueues_) {
            queue->Build();
            auto* traceable = dynamic_cast<TraceableQueue*>(queue.get());
            if (traceable != nullptr && GetSim() != nullptr) {
                GetSim()->RegisterTraceQueue(*traceable, Path() + "/" + queue->Name());
            }
        }
    }

    void WorkChildren();
    void XferChildren();

    void ResetChildren()
    {
        for (const auto& child : submodules_) {
            child->Reset();
        }
    }

    void ReportChildren()
    {
        for (const auto& child : submodules_) {
            child->ReportStat();
        }
    }

    void AdvanceInnerQueues();

    void ResetInnerQueues()
    {
        for (const auto& queue : innerQueues_) {
            queue->Reset();
        }
    }

    Logger& Log()
    {
        SimSystem* const system = GetSim();
        if (system == nullptr) {
            throw std::logic_error("module is not attached to a simulation system");
        }
        return system->GetLogger();
    }

    Reporter& Report()
    {
        SimSystem* const system = GetSim();
        if (system == nullptr) {
            throw std::logic_error("module is not attached to a simulation system");
        }
        return system->GetReporter();
    }

    SimTrace* Trace()
    {
        SimSystem* const system = GetSim();
        return system == nullptr ? nullptr : system->GetTrace();
    }

    template <typename T>
    SimQueue<T>* BindInput(SimQueue<T>* queue)
    {
        BindPort(queue, inputs_);
        return queue;
    }

    template <typename T>
    SimQueue<T>* BindOutput(SimQueue<T>* queue)
    {
        BindPort(queue, outputs_);
        return queue;
    }

    template <typename T>
    SimQueue<T>* AddInnerQueue(std::shared_ptr<SimQueue<T>> queue)
    {
        EnsureObjectSlot();
        SimQueue<T>* const port = queue.get();
        if (port == nullptr || Contains(innerQueuePorts_, port)) {
            throw std::invalid_argument("inner queue must be unique and non-null");
        }
        innerQueues_.reserve(innerQueues_.size() + 1);
        innerQueuePorts_.reserve(innerQueuePorts_.size() + 1);
        innerQueues_.push_back(std::move(queue));
        innerQueuePorts_.push_back(port);
        AddObject(port);
        return port;
    }

private:
    friend class SimSystem;

    static bool Contains(const std::vector<const SimObj*>& ports, const SimObj* port)
    {
        return std::find(ports.begin(), ports.end(), port) != ports.end();
    }

    void BindPort(const SimObj* port, std::vector<const SimObj*>& role)
    {
        if (ObjectsFrozen()) {
            throw std::logic_error("queue bindings are frozen after Build");
        }
        if (port == nullptr || Contains(role, port)) {
            throw std::invalid_argument("queue port must be unique and non-null");
        }
        role.push_back(port);
    }

    SimSystem* system_ = nullptr; // non-owning
    SimModule* parent_ = nullptr; // non-owning; parent owns this module
    std::vector<std::shared_ptr<SimModule>> submodules_;
    std::vector<std::shared_ptr<SimObj>> innerQueues_;
    std::vector<const SimObj*> inputs_;
    std::vector<const SimObj*> outputs_;
    std::vector<const SimObj*> innerQueuePorts_;
    std::vector<const SimModule*> moduleTargets_; // non-owning, in binding order
};

} // namespace parallel_gfsim
