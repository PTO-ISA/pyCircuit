#pragma once

// Internal execution machinery; not part of the public model API.
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace parallel_gfsim {
class SimModule;
class SimObj;
class SimSystem;

enum class SimPhase { Work, Xfer };

struct SimTaskContext {
    SimSystem* system;
    SimPhase phase;
    std::size_t task;
};

const SimTaskContext& CurrentSimTask();

class SimTaskScope {
public:
    SimTaskScope(SimSystem* system, SimPhase phase, std::size_t task);
    ~SimTaskScope();
private:
    SimTaskContext previous_;
};

struct SimExecutionPlan {
    struct Task {
        SimModule* root;
        std::size_t id;
        std::vector<SimObj*> queues;
    };
    SimExecutionPlan(SimModule* parent, std::size_t workers);
    bool AssignQueue(SimObj* queue, const SimModule* producer, const SimModule* consumer);
    SimModule* parent;
    std::vector<std::vector<Task>> workers;
private:
    void IndexSubtree(const SimModule* module, std::size_t task);
    std::unordered_map<const SimModule*, std::size_t> taskIds_;
};

class SimExecutor {
public:
    // Injectable launcher is internal and permits deterministic failure testing.
    using ThreadFactory = std::function<std::thread(std::function<void()>)>;
    SimExecutor(SimSystem* system, SimExecutionPlan plan,
                ThreadFactory launch = ThreadFactory());
    ~SimExecutor();
    void Execute(SimPhase phase);
private:
    void Worker(std::size_t index);
    void Stop();

    SimSystem* system_;
    const SimExecutionPlan plan_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::condition_variable done_;
    std::vector<std::thread> threads_;
    std::size_t batch_ = 0;
    std::size_t remaining_ = 0;
    SimPhase phase_ = SimPhase::Work;
    bool stopping_ = false;
    std::exception_ptr exception_;
};
} // namespace parallel_gfsim
