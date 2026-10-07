#ifndef GFSIM_WORKEXECUTOR_H
#define GFSIM_WORKEXECUTOR_H

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

namespace gfsim {

struct WorkItem {
  void *instance;
  void (*work)(void *);
};

// Tasks and their instances are borrowed and must remain valid until run
// returns. No hardware ownership is transferred.
class WorkExecutor {
public:
  explicit WorkExecutor(std::size_t workers = 1) : workers_(workers) {
    if (workers == 0)
      throw std::invalid_argument("WorkExecutor workers must be positive");
    if (workers == 1)
      return;
    try {
      threads_.reserve(workers);
      for (std::size_t i = 0; i < workers; ++i)
        threads_.emplace_back([this] { workerLoop(); });
    } catch (...) {
      stopAndJoin();
      throw;
    }
  }

  ~WorkExecutor() {
    std::lock_guard runLock(runMutex_);
    stopAndJoin();
  }

  WorkExecutor(const WorkExecutor &) = delete;
  WorkExecutor &operator=(const WorkExecutor &) = delete;
  WorkExecutor(WorkExecutor &&) = delete;
  WorkExecutor &operator=(WorkExecutor &&) = delete;

  std::size_t workers() const noexcept { return workers_; }

  void run(std::span<const WorkItem> items) {
    for (const WorkItem &item : items)
      if (!item.work)
        throw std::invalid_argument("WorkExecutor task needs a Work callback");

    // A delegated subtree can borrow this same pool. Joining a nested batch
    // from a pool worker would consume a worker waiting for itself.
    if (active_ == this) {
      runSerial(items);
      return;
    }
    std::lock_guard runLock(runMutex_);
    if (workers_ == 1) {
      ActiveScope scope(this);
      runSerial(items);
      return;
    }
    if (items.empty())
      return;

    std::unique_lock lock(mutex_);
    errors_.assign(items.size(), {});
    items_ = items;
    next_ = 0;
    pending_ = items.size();
    workReady_.notify_all();
    complete_.wait(lock, [this] { return pending_ == 0; });
    items_ = {};
    for (const auto &error : errors_)
      if (error)
        std::rethrow_exception(error);
  }

private:
  class ActiveScope {
  public:
    explicit ActiveScope(WorkExecutor *executor) : previous_(active_) {
      active_ = executor;
    }
    ~ActiveScope() { active_ = previous_; }

  private:
    WorkExecutor *previous_;
  };

  static void runSerial(std::span<const WorkItem> items) {
    std::exception_ptr first;
    for (const WorkItem &item : items) {
      try {
        item.work(item.instance);
      } catch (...) {
        if (!first)
          first = std::current_exception();
      }
    }
    if (first)
      std::rethrow_exception(first);
  }

  void workerLoop() {
    ActiveScope scope(this);
    std::unique_lock lock(mutex_);
    while (true) {
      workReady_.wait(lock, [this] { return stop_ || next_ < items_.size(); });
      if (stop_)
        return;
      const std::size_t index = next_++;
      const WorkItem item = items_[index];
      lock.unlock();
      std::exception_ptr error;
      try {
        item.work(item.instance);
      } catch (...) {
        error = std::current_exception();
      }
      lock.lock();
      errors_[index] = error;
      if (--pending_ == 0)
        complete_.notify_one();
    }
  }

  void stopAndJoin() noexcept {
    {
      std::lock_guard lock(mutex_);
      stop_ = true;
    }
    workReady_.notify_all();
    for (auto &thread : threads_)
      if (thread.joinable())
        thread.join();
  }

  inline static thread_local WorkExecutor *active_ = nullptr;
  const std::size_t workers_;
  std::vector<std::thread> threads_;
  std::mutex runMutex_;
  std::mutex mutex_;
  std::condition_variable workReady_;
  std::condition_variable complete_;
  std::span<const WorkItem> items_;
  std::vector<std::exception_ptr> errors_;
  std::size_t next_ = 0;
  std::size_t pending_ = 0;
  bool stop_ = false;
};

} // namespace gfsim

#endif // GFSIM_WORKEXECUTOR_H
