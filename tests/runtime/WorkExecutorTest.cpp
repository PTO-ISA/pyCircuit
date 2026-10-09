#include "gfsim/WorkExecutor.h"
#include "gfsim/SimSystem.h"
#include "gfsim/dff.h"
#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {
using namespace std::chrono_literals;
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}

TEST(WorkExecutorTest, PositiveWorkerCountAndExactOnceBorrowedTasks) {
  EXPECT_THROW(gfsim::WorkExecutor(0), std::invalid_argument);
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor executor(workers);
    EXPECT_EQ(executor.workers(), workers);
    std::array<std::atomic<unsigned>, 7> counts{};
    std::array<gfsim::WorkItem, 7> tasks{};
    for (std::size_t i = 0; i < tasks.size(); ++i)
      tasks[i] = {&counts[i],
                  [](void *p) { ++*static_cast<std::atomic<unsigned> *>(p); }};
    executor.run(tasks);
    executor.run(tasks);
    for (const auto &count : counts)
      EXPECT_EQ(count.load(), 2u);
    executor.run({});
    auto invalid = tasks;
    invalid.back().work = nullptr;
    EXPECT_THROW(executor.run(invalid), std::invalid_argument);
    for (const auto &count : counts)
      EXPECT_EQ(count.load(), 2u);
  }
}

TEST(WorkExecutorTest, TwoCallbacksActuallyOverlapAndJoinBeforeReturning) {
  struct Gate {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned entered = 0, finished = 0;
    bool timeout = false;
    static void work(void *p) {
      auto &gate = *static_cast<Gate *>(p);
      std::unique_lock lock(gate.mutex);
      ++gate.entered;
      gate.changed.notify_all();
      if (!gate.changed.wait_for(lock, 5s, [&] { return gate.entered == 2; }))
        gate.timeout = true;
      ++gate.finished;
    }
  } gate;
  gfsim::WorkExecutor executor(2);
  const std::array tasks{gfsim::WorkItem{&gate, &Gate::work},
                         gfsim::WorkItem{&gate, &Gate::work}};
  executor.run(tasks);
  EXPECT_FALSE(gate.timeout);
  EXPECT_EQ(gate.entered, 2u);
  EXPECT_EQ(gate.finished, 2u);
}

TEST(WorkExecutorTest, EveryTaskFinishesAndEarliestIndexFailureWins) {
  struct Order {
    std::mutex mutex;
    std::condition_variable changed;
    bool secondFinished = false;
  };
  struct Job {
    unsigned index;
    std::array<std::atomic<unsigned>, 3> &counts;
    Order &order;
    bool forceReverseCompletion;
    static void work(void *p) {
      auto &self = *static_cast<Job *>(p);
      ++self.counts[self.index];
      if (self.index == 0 && self.forceReverseCompletion) {
        std::unique_lock lock(self.order.mutex);
        if (!self.order.changed.wait_for(
                lock, 5s, [&] { return self.order.secondFinished; }))
          throw std::runtime_error("second task never finished");
      }
      if (self.index == 1) {
        {
          std::lock_guard lock(self.order.mutex);
          self.order.secondFinished = true;
        }
        self.order.changed.notify_all();
      }
      if (self.index < 2)
        throw std::runtime_error(std::to_string(self.index));
    }
  };
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor executor(workers);
    std::array<std::atomic<unsigned>, 3> counts{};
    Order order;
    std::array jobs{Job{0, counts, order, workers == 2},
                    Job{1, counts, order, workers == 2},
                    Job{2, counts, order, workers == 2}};
    std::array<gfsim::WorkItem, 3> tasks{};
    for (unsigned i = 0; i < tasks.size(); ++i)
      tasks[i] = {&jobs[i], &Job::work};
    for (unsigned run = 0; run < 2; ++run) {
      {
        std::lock_guard lock(order.mutex);
        order.secondFinished = false;
      }
      try {
        executor.run(tasks);
        FAIL() << "both throwing tasks were ignored";
      } catch (const std::runtime_error &error) {
        EXPECT_STREQ(error.what(), "0");
      }
      for (const auto &count : counts)
        EXPECT_EQ(count.load(), run + 1);
    }
  }
}

TEST(WorkExecutorTest, SamePoolNestedRunCannotDeadlock) {
  struct Parent {
    gfsim::WorkExecutor &executor;
    std::array<std::atomic<unsigned>, 2> children{};
    static void work(void *p) {
      auto &self = *static_cast<Parent *>(p);
      std::array<gfsim::WorkItem, 2> inner{};
      for (unsigned i = 0; i < inner.size(); ++i)
        inner[i] = {&self.children[i], [](void *q) {
                      ++*static_cast<std::atomic<unsigned> *>(q);
                    }};
      self.executor.run(inner);
    }
  };
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor executor(workers);
    Parent first{executor}, second{executor};
    const std::array tasks{gfsim::WorkItem{&first, &Parent::work},
                           gfsim::WorkItem{&second, &Parent::work}};
    executor.run(tasks);
    for (const auto &count : first.children)
      EXPECT_EQ(count.load(), 1u);
    for (const auto &count : second.children)
      EXPECT_EQ(count.load(), 1u);
  }
}

class ParallelRoot final : public gfsim::SimModule {
public:
  using Kernel = gfsim::dffe_kernel<gfsim::Bits<9>>;
  struct Leaf {
    Kernel::Current current;
    Kernel::Pending pending;
    gfsim::wire<gfsim::Bits<1>> clk = known<1>(1), rst = known<1>(0),
                                en = known<1>(1);
    gfsim::wire<gfsim::Bits<9>> data, init, output;
    Kernel::Inputs inputs() const noexcept {
      return {clk, rst, en, data, init};
    }
  };
  explicit ParallelRoot(gfsim::WorkExecutor &executor)
      : SimModule("root"), executor(executor) {
    leaves[0].init = known<9>(3);
    leaves[0].data = known<9>(11);
    leaves[1].init = known<9>(7);
    leaves[1].data = known<9>(22);
  }
  void Work() override {
    staged.store(false);
    struct Job {
      ParallelRoot *root;
      unsigned index;
    };
    std::array jobs{Job{this, 0}, Job{this, 1}};
    auto work = [](void *p) {
      auto &job = *static_cast<Job *>(p);
      if (job.index == 1) {
        std::unique_lock lock(job.root->mutex);
        if (!job.root->changed.wait_for(
                lock, 5s, [&] { return job.root->staged.load(); }))
          throw std::runtime_error("first module failed to finish Work");
      }
      auto &leaf = job.root->leaves[job.index];
      Kernel::work(leaf.current, leaf.inputs(), leaf.pending, {leaf.output});
      if (job.index == 0) {
        {
          std::lock_guard lock(job.root->mutex);
          job.root->staged.store(true);
        }
        job.root->changed.notify_all();
      }
    };
    const std::array tasks{gfsim::WorkItem{&jobs[0], work},
                           gfsim::WorkItem{&jobs[1], work}};
    executor.run(tasks);
  }
  void Xfer() noexcept override {
    ++transfers;
    for (auto &leaf : leaves)
      Kernel::xfer(leaf.current, leaf.pending, {leaf.output});
  }
  void DiscardNext() noexcept override {
    for (auto &leaf : leaves)
      Kernel::discard(leaf.current, leaf.pending);
  }
  void Reset() noexcept override {
    for (auto &leaf : leaves)
      Kernel::reset(leaf.current, leaf.inputs(), leaf.pending);
  }
  bool HasWork() const noexcept override { return true; }
  std::array<Leaf, 2> leaves;
  unsigned transfers = 0;
  std::atomic<bool> staged{false};

private:
  gfsim::WorkExecutor &executor;
  std::mutex mutex;
  std::condition_variable changed;
};
class System final : public gfsim::SimSystem {
  bool Precheck() noexcept override { return true; }
};

TEST(WorkExecutorTest, WorkerFailureDiscardsAllRegistersAndPendingClocks) {
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor executor(workers);
    ParallelRoot root(executor);
    System system;
    ASSERT_TRUE(system.AddModule(root));
    system.Build();
    system.Reset();
    ASSERT_EQ(system.state(), gfsim::SimSystemState::Ready);
    const unsigned transfers = root.transfers;
    root.leaves[1].en = gfsim::wire<gfsim::Bits<1>>::unknown();
    EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
    EXPECT_TRUE(root.staged.load());
    EXPECT_EQ(root.transfers, transfers);
    EXPECT_EQ(system.cycle(), 0u);
    for (unsigned i = 0; i < 2; ++i) {
      EXPECT_EQ(root.leaves[i].current.q.value(),
                gfsim::Bits<9>{i == 0 ? 3u : 7u});
      EXPECT_FALSE(root.leaves[i].current.clock);
      EXPECT_FALSE(root.leaves[i].pending.valid);
      EXPECT_FALSE(root.leaves[i].pending.clock);
    }
    root.leaves[1].en = known<1>(1);
    system.Reset();
    EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
    EXPECT_EQ(root.leaves[0].current.q.value(), gfsim::Bits<9>{11});
    EXPECT_EQ(root.leaves[1].current.q.value(), gfsim::Bits<9>{22});
  }
}
} // namespace
