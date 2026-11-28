#include "SimExecutor.h"

#include <algorithm>
#include <utility>
#include "parallel_gfsim/SimModule.h"

namespace parallel_gfsim {
namespace {
thread_local SimTaskContext context = {nullptr, SimPhase::Work, 0};
}

const SimTaskContext& CurrentSimTask() { return context; }

SimTaskScope::SimTaskScope(SimSystem* system, SimPhase phase, std::size_t task)
    : previous_(context)
{
    context = SimTaskContext{system, phase, task};
}
SimTaskScope::~SimTaskScope() { context = previous_; }

SimExecutionPlan::SimExecutionPlan(SimModule* split, std::size_t count)
    : parent(split), workers(std::min(count, split->Submodules().size()))
{
    for (std::size_t i = 0; i < split->Submodules().size(); ++i) {
        SimModule* root = split->Submodules()[i].get();
        workers[i % workers.size()].push_back(Task{root, i + 1, {}});
        IndexSubtree(root, i + 1);
    }
}

void SimExecutionPlan::IndexSubtree(const SimModule* module, std::size_t task)
{
    taskIds_.emplace(module, task);
    for (const auto& child : module->Submodules()) IndexSubtree(child.get(), task);
}

bool SimExecutionPlan::AssignQueue(SimObj* queue, const SimModule* producer,
                                 const SimModule* consumer)
{
    auto endpoint = taskIds_.find(producer);
    if (endpoint == taskIds_.end()) endpoint = taskIds_.find(consumer);
    if (endpoint == taskIds_.end()) return false;
    const std::size_t index = endpoint->second - 1;
    workers[index % workers.size()][index / workers.size()].queues.push_back(queue);
    return true;
}

SimExecutor::SimExecutor(SimSystem* system, SimExecutionPlan plan, ThreadFactory launch)
    : system_(system), plan_(std::move(plan))
{
    if (!launch) launch = [](std::function<void()> run) { return std::thread(run); };
    threads_.reserve(plan_.workers.size());
    try {
        for (std::size_t i = 0; i < plan_.workers.size(); ++i) {
            threads_.push_back(launch([this, i] { Worker(i); }));
        }
    } catch (...) {
        Stop();
        throw;
    }
}

SimExecutor::~SimExecutor() { Stop(); }

void SimExecutor::Stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    ready_.notify_all();
    for (auto& thread : threads_) if (thread.joinable()) thread.join();
}

void SimExecutor::Execute(SimPhase phase)
{
    std::unique_lock<std::mutex> lock(mutex_);
    phase_ = phase;
    exception_ = nullptr;
    remaining_ = threads_.size();
    ++batch_;
    ready_.notify_all();
    done_.wait(lock, [this] { return remaining_ == 0; });
    if (exception_) std::rethrow_exception(exception_);
}

void SimExecutor::Worker(std::size_t index)
{
    std::size_t observed = 0;
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        ready_.wait(lock, [this, observed] { return stopping_ || batch_ != observed; });
        if (stopping_) return;
        observed = batch_;
        const SimPhase phase = phase_;
        lock.unlock();
        std::exception_ptr failure;
        try {
            for (const auto& task : plan_.workers[index]) {
                SimTaskScope scope(system_, phase, task.id);
                if (phase == SimPhase::Work) task.root->Work();
                else {
                    // Boundary publication never overlaps module Work. Xfer
                    // commits local state and must not query queue state.
                    for (SimObj* queue : task.queues) queue->Xfer();
                    task.root->Xfer();
                }
            }
        } catch (...) {
            failure = std::current_exception();
        }
        lock.lock();
        if (failure && !exception_) exception_ = failure;
        if (--remaining_ == 0) done_.notify_one();
    }
}
} // namespace parallel_gfsim
