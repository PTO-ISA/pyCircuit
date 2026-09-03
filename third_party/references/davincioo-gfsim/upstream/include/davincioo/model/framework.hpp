#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace davincioo {

#define GFSIM_ASSERT(condition)                                                                    \
  do {                                                                                             \
    if (!(condition)) {                                                                            \
      throw std::runtime_error("gfsim assertion failed: " #condition);                            \
    }                                                                                              \
  } while (false)

class SimObject {
public:
  explicit SimObject(std::string name) : name_(std::move(name)) {}
  virtual ~SimObject() = default;

  virtual void Build() {}
  virtual void Reset() {}
  virtual void Report() {}
  virtual void Work() {}
  virtual void Xfer() {}

  const std::string& Name() const noexcept { return name_; }
  void SetCurrentCycle(std::uint64_t cycle) noexcept { current_cycle_ = cycle; }
  std::uint64_t CurrentCycle() const noexcept { return current_cycle_; }
  void MarkProgress(std::size_t amount = 1) noexcept { progress_count_ += amount; }
  std::size_t ConsumeProgress() noexcept {
    const std::size_t amount = progress_count_;
    progress_count_ = 0;
    return amount;
  }

private:
  std::string name_;
  std::uint64_t current_cycle_ = 0;
  std::size_t progress_count_ = 0;
};

template <class T>
class SimQueue : public SimObject {
public:
  using value_type = T;
  using size_type = std::size_t;

  explicit SimQueue(size_type max_size = 1024, std::uint32_t latency = 1, std::string name = "sim_queue")
      : SimObject(std::move(name)), max_size_(max_size), latency_(latency) {
    GFSIM_ASSERT(max_size_ > 0);
  }

  void Build() override {}

  void Work() override {
    if (stalled_) {
      return;
    }

    bool moved = false;
    for (auto& delay : delay_queue_) {
      if (delay > 0) {
        --delay;
        moved = true;
      }
    }
    while (!write_queue_.empty() && !delay_queue_.empty() && delay_queue_.front() == 0 &&
           read_queue_.size() < max_size_) {
      read_queue_.push_back(std::move(write_queue_.front()));
      write_queue_.pop_front();
      delay_queue_.pop_front();
      moved = true;
    }
    GFSIM_ASSERT(write_queue_.size() == delay_queue_.size());
    if (moved) {
      MarkProgress();
    }
  }

  void Xfer() override {}

  void Reset() override {
    write_queue_.clear();
    read_queue_.clear();
    delay_queue_.clear();
    stalled_ = false;
    ConsumeProgress();
  }

  void SetLatency(std::uint32_t latency) { latency_ = latency; }
  std::uint32_t GetLatency() const noexcept { return latency_; }

  bool getStall() const noexcept { return stalled_; }
  void setStall() noexcept { stalled_ = true; }
  void setStall(bool stalled) noexcept { stalled_ = stalled; }
  void unsetStall() noexcept { stalled_ = false; }

  template <class... Args>
  T& Write(Args&&... args) {
    GFSIM_ASSERT(!Full());
    write_queue_.emplace_back(std::forward<Args>(args)...);
    delay_queue_.push_back(latency_);
    GFSIM_ASSERT(write_queue_.size() == delay_queue_.size());
    MarkProgress();
    return write_queue_.back();
  }

  template <class... Args>
  T& PrimeVisible(Args&&... args) {
    GFSIM_ASSERT(!Full());
    read_queue_.emplace_back(std::forward<Args>(args)...);
    MarkProgress();
    return read_queue_.back();
  }

  T Read() {
    GFSIM_ASSERT(!read_queue_.empty());
    T value = std::move(read_queue_.front());
    read_queue_.pop_front();
    MarkProgress();
    return value;
  }

  T& Front() {
    GFSIM_ASSERT(!read_queue_.empty());
    return read_queue_.front();
  }

  const T& Front() const {
    GFSIM_ASSERT(!read_queue_.empty());
    return read_queue_.front();
  }

  void Pop() {
    GFSIM_ASSERT(!read_queue_.empty());
    read_queue_.pop_front();
    MarkProgress();
  }

  bool Empty() const noexcept { return read_queue_.empty(); }
  bool EmptyW() const noexcept { return write_queue_.empty(); }
  bool Full() const noexcept { return read_queue_.size() + write_queue_.size() >= max_size_; }
  bool Full(std::uint32_t reserve) const noexcept { return read_queue_.size() + write_queue_.size() + reserve >= max_size_; }
  bool ToBeFull(std::uint32_t reserve = 0) const noexcept { return Full(reserve); }
  size_type Size() const noexcept { return read_queue_.size(); }
  size_type SizeW() const noexcept { return write_queue_.size(); }
  size_type Occupancy() const noexcept { return read_queue_.size() + write_queue_.size(); }
  std::int32_t MaxSize() const noexcept { return static_cast<std::int32_t>(max_size_); }

  std::deque<T>& GetRawWriteData() { return write_queue_; }
  const std::deque<T>& GetRawWriteData() const { return write_queue_; }

  std::deque<T>& GetRawReadData() { return read_queue_; }
  const std::deque<T>& GetRawReadData() const { return read_queue_; }

  template <class Function>
  void FlushIf(Function&& match) {
    for (size_type i = 0; i < write_queue_.size();) {
      if (std::invoke(match, write_queue_[i])) {
        write_queue_.erase(write_queue_.begin() + static_cast<std::ptrdiff_t>(i));
        delay_queue_.erase(delay_queue_.begin() + static_cast<std::ptrdiff_t>(i));
        MarkProgress();
      } else {
        ++i;
      }
    }

    for (auto it = read_queue_.begin(); it != read_queue_.end();) {
      if (std::invoke(match, *it)) {
        it = read_queue_.erase(it);
        MarkProgress();
      } else {
        ++it;
      }
    }
    GFSIM_ASSERT(write_queue_.size() == delay_queue_.size());
  }

  void InitMaxSize(std::uint32_t size) {
    GFSIM_ASSERT(size > 0);
    max_size_ = size;
  }

private:
  std::deque<T> write_queue_;
  std::deque<T> read_queue_;
  std::deque<std::uint32_t> delay_queue_;
  size_type max_size_ = 0;
  std::uint32_t latency_ = 1;
  bool stalled_ = false;
};

template <class T>
using SimUniqueQueue = SimQueue<std::unique_ptr<T>>;

template <class Derived, class PortT>
class Module : public SimObject {
public:
  using Queue = SimQueue<PortT>;
  using size_type = std::size_t;

  explicit Module(std::string name) : SimObject(std::move(name)) {}
  ~Module() override = default;

  void Build() override {
    BuildSelf();
    for (auto& submodule : submodules_) {
      GFSIM_ASSERT(submodule != nullptr);
      submodule->Build();
    }
    built_ = true;
  }

  void Reset() override {
    ResetSelf();
    for (auto& submodule : submodules_) {
      GFSIM_ASSERT(submodule != nullptr);
      submodule->Reset();
    }
    for (auto& queue : owned_queues_) {
      GFSIM_ASSERT(queue != nullptr);
      queue->Reset();
    }
  }

  void Report() override {
    ReportSelf();
    for (auto& submodule : submodules_) {
      GFSIM_ASSERT(submodule != nullptr);
      submodule->Report();
    }
  }

  void Work() override {
    WorkSelf();
    for (auto& submodule : submodules_) {
      GFSIM_ASSERT(submodule != nullptr);
      submodule->SetCurrentCycle(CurrentCycle());
      submodule->Work();
      MarkProgress(submodule->ConsumeProgress());
    }
    for (auto& queue : owned_queues_) {
      GFSIM_ASSERT(queue != nullptr);
      queue->SetCurrentCycle(CurrentCycle());
      queue->Work();
      MarkProgress(queue->ConsumeProgress());
    }
  }

  void Xfer() override {
    XferSelf();
    for (auto& submodule : submodules_) {
      GFSIM_ASSERT(submodule != nullptr);
      submodule->Xfer();
    }
    for (auto& queue : owned_queues_) {
      GFSIM_ASSERT(queue != nullptr);
      queue->Xfer();
    }
  }

  void BindInput(size_type idx, Queue* queue) { BindPort(idx, queue, inputs_); }
  void BindOutput(size_type idx, Queue* queue) { BindPort(idx, queue, outputs_); }
  void BindInner(size_type idx, Queue* queue) { BindPort(idx, queue, inners_); }

  template <class... Args>
  Queue* CreateOwnedQueue(Args&&... args) {
    owned_queues_.push_back(std::make_unique<Queue>(std::forward<Args>(args)...));
    return owned_queues_.back().get();
  }

  Derived& AddSubmodule(std::unique_ptr<Derived> submodule) {
    GFSIM_ASSERT(submodule != nullptr);
    submodules_.push_back(std::move(submodule));
    return *submodules_.back();
  }

  void ConnectInput(Derived& child, size_type child_input_idx, Queue* queue) { child.BindInput(child_input_idx, queue); }
  void ConnectOutput(Derived& child, size_type child_output_idx, Queue* queue) { child.BindOutput(child_output_idx, queue); }
  void ConnectInner(Derived& child, size_type child_inner_idx, Queue* queue) { child.BindInner(child_inner_idx, queue); }

  Queue* Input(size_type idx) {
    GFSIM_ASSERT(idx < inputs_.size());
    GFSIM_ASSERT(inputs_[idx] != nullptr);
    return inputs_[idx];
  }

  Queue* Output(size_type idx) {
    GFSIM_ASSERT(idx < outputs_.size());
    GFSIM_ASSERT(outputs_[idx] != nullptr);
    return outputs_[idx];
  }

  Queue* Inner(size_type idx) {
    GFSIM_ASSERT(idx < inners_.size());
    GFSIM_ASSERT(inners_[idx] != nullptr);
    return inners_[idx];
  }

protected:
  virtual void BuildSelf() {}
  virtual void ResetSelf() {}
  virtual void ReportSelf() {}
  virtual void WorkSelf() {}
  virtual void XferSelf() {}

  std::vector<Queue*> inputs_;
  std::vector<Queue*> outputs_;
  std::vector<Queue*> inners_;
  std::vector<std::unique_ptr<Derived>> submodules_;
  std::vector<std::unique_ptr<Queue>> owned_queues_;

private:
  void BindPort(size_type idx, Queue* queue, std::vector<Queue*>& slots) {
    GFSIM_ASSERT(queue != nullptr);
    if (slots.size() <= idx) {
      slots.resize(idx + 1, nullptr);
    }
    GFSIM_ASSERT(slots[idx] == nullptr || slots[idx] == queue);
    slots[idx] = queue;
  }

  bool built_ = false;
};

class SimSystem {
public:
  using size_type = std::size_t;

  virtual ~SimSystem() = default;

  void Build() {
    BuildSystem();
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->Build();
    }
    built_ = true;
  }

  void Reset() {
    ResetSystem();
    cycle_ = 0;
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->Reset();
    }
  }

  void Report() {
    ReportSystem();
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->Report();
    }
  }

  void Step() {
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->SetCurrentCycle(cycle_);
    }
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->Work();
    }
    for (auto* module : modules_) {
      GFSIM_ASSERT(module != nullptr);
      module->Xfer();
    }
    ++cycle_;
  }

  template <class ModuleT, class... Args>
  ModuleT& EmplaceOwnedModule(Args&&... args) {
    auto module = std::make_unique<ModuleT>(std::forward<Args>(args)...);
    ModuleT& ref = *module;
    modules_.push_back(&ref);
    owned_modules_.push_back(std::move(module));
    return ref;
  }

  std::uint64_t Cycle() const noexcept { return cycle_; }
  bool IsBuilt() const noexcept { return built_; }

protected:
  virtual void BuildSystem() {}
  virtual void ResetSystem() {}
  virtual void ReportSystem() {}

private:
  std::vector<SimObject*> modules_;
  std::vector<std::unique_ptr<SimObject>> owned_modules_;
  std::uint64_t cycle_ = 0;
  bool built_ = false;
};

}  // namespace davincioo

#define GFSIM_INPUT(name, idx)                                                                     \
  auto* name [[maybe_unused]] = this->Input(idx);                                                  \
  GFSIM_ASSERT((name) != nullptr)

#define GFSIM_OUTPUT(name, idx)                                                                    \
  auto* name [[maybe_unused]] = this->Output(idx);                                                 \
  GFSIM_ASSERT((name) != nullptr)

#define GFSIM_INNER(name, idx)                                                                     \
  auto* name [[maybe_unused]] = this->Inner(idx);                                                  \
  GFSIM_ASSERT((name) != nullptr)

#ifndef INPUT
#define INPUT(name, idx) GFSIM_INPUT(name, idx)
#endif

#ifndef OUTPUT
#define OUTPUT(name, idx) GFSIM_OUTPUT(name, idx)
#endif

#ifndef INNER
#define INNER(name, idx) GFSIM_INNER(name, idx)
#endif
