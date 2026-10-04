#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gfsim {
using Tick = std::uint64_t;
using ModuleId = std::size_t;
using RuleId = std::size_t;
using QueueId = std::size_t;
using SignalId = std::size_t;
struct NeedInput : std::runtime_error {
    NeedInput() : runtime_error("required Queue input is empty") {}
};
inline Tick checkedAdd(Tick a, Tick b) {
    if (b > std::numeric_limits<Tick>::max() - a)
        throw std::overflow_error("GFSim counter overflow");
    return a + b;
}
struct Stats {
    Tick moduleWork{}, ruleWork{}, signalWork{}, changeNotifications{};
    Tick arbitrationAttempts{}, deltaRounds{}, queueChecks{}, accepted{}, events{}, dueEvents{};
};
struct WakeRequest {
    ModuleId module;
    Tick delay;
};
// Shared by Queues with the same fixed sources. Slot zero is reserved; the
// table covers only the interval between the first and last real RuleId.
struct SourceSlots {
    RuleId first{};
    std::vector<std::size_t> indices;
    std::size_t index(RuleId rule) const {
        if (!rule)
            return 0;
        auto offset = rule - first;
        if (offset >= indices.size() || indices[offset] == std::numeric_limits<std::size_t>::max())
            throw std::logic_error("undeclared Queue source");
        return indices[offset];
    }
};
class Simulator;
struct TestAccess;
class ResourceBase {
  public:
    ResourceBase() = default;
    virtual ~ResourceBase() = default;
    ResourceBase(const ResourceBase &) = delete;
    ResourceBase &operator=(const ResourceBase &) = delete;

  private:
    friend class Simulator;
    friend class QueueBase;
    Simulator *sim_{};
    std::vector<ModuleId> readers_;
    std::vector<SignalId> signalReaders_;
};
struct ModuleRecord {
    void *object{};
    void (*work)(void *){};
    std::optional<Tick> workedTick;
    Tick calls{};
    std::vector<RuleId> rules;
};
struct RuleRecord {
    ModuleId owner{};
    std::vector<QueueId> participants;
    std::vector<WakeRequest> wakeRequests;
    bool complete{};
    std::optional<Tick> selectedTick, acceptedTick;
    Tick calls{};
    bool hasEffects() const { return !participants.empty() || !wakeRequests.empty(); }
};
class QueueBase : public ResourceBase {
  public:
    QueueId id() const { return id_; }
    Tick stateVersion() const { return version_; }
    std::optional<RuleId> popRule() const { return popRule_; }
    std::optional<RuleId> pushRule() const { return pushRule_; }
    virtual std::size_t size() const = 0;
    virtual std::size_t capacity() const = 0;
    bool empty() const { return size() == 0; }
    bool full() const { return size() == capacity(); }
    virtual std::size_t sourceCount() const = 0;

  protected:
    void prepare(RuleId, bool readsTarget, bool first);
    void changed() { version_ = checkedAdd(version_, 1); }
    void bindSlots(std::vector<RuleId> sources);
    std::size_t slotIndex(RuleId rule) const {
        if (!sourceSlots_)
            throw std::logic_error("Queue sources are not frozen");
        return sourceSlots_->index(rule);
    }

  private:
    friend class Simulator;
    friend struct TestAccess;
    QueueId id_{};
    Tick version_{};
    std::optional<RuleId> popRule_, pushRule_;
    std::optional<Tick> usedTick_;
    const SourceSlots *sourceSlots_{};
    virtual void registerSource(RuleId, unsigned) = 0;
    virtual void freeze() = 0;
    virtual bool acceptedPopFor(RuleId) const = 0;
    virtual bool canAccept(RuleId) const = 0;
    virtual void accept(RuleId) = 0;
    virtual void cancel(RuleId) = 0;
    virtual bool xfer() = 0;
};
class SignalBase : public ResourceBase {
  public:
    SignalId id() const { return id_; }
    Tick evaluations() const { return evaluations_; }

  private:
    friend class Simulator;
    friend struct TestAccess;
    SignalId id_{};
    Tick evaluations_{};
    virtual bool evaluate() = 0;
};
enum Operation : unsigned { Pop = 1, Push = 2, Revise = 4 };

class Simulator {
  public:
    using Work = void (*)(void *);
    Simulator() = default;
    ~Simulator(); // Registered resources must outlive the Simulator.
    Simulator(const Simulator &) = delete;
    Simulator &operator=(const Simulator &) = delete;
    ModuleId addModule(void *object, Work work);
    template <auto Method, class T> ModuleId addModule(T &object) {
        return addModule(&object, [](void *p) { (static_cast<T *>(p)->*Method)(); });
    }
    RuleId addRule(ModuleId owner);
    QueueId addQueue(QueueBase &queue);
    SignalId addSignal(SignalBase &signal);
    void declareResource(ModuleId, ResourceBase &);
    void declareInput(SignalBase &, ResourceBase &);
    void bind(RuleId rule, QueueBase &queue, unsigned operations);
    void freeze();
    std::span<const RuleId> step();
    bool beginRule(RuleId rule);
    void completeRule(RuleId);
    void abortRule(RuleId);
    void requestWakeup(RuleId, ModuleId, Tick delay);
    Tick tick() const { return tick_; }
    bool failed() const { return failed_; }
    bool frozen() const { return frozen_; }
    const Stats &stats() const { return stats_; }
    const ModuleRecord &module(ModuleId m) const { return modules_.at(m); }
    const RuleRecord &rule(RuleId r) const { return rules_.at(r); }
    std::span<QueueBase *const> queues() const { return queues_; }
    std::size_t moduleCount() const { return modules_.size(); }
    std::size_t ruleCount() const { return rules_.size() - 1; }
    std::vector<std::pair<Tick, ModuleId>> events() const;

  private:
    friend class QueueBase;
    friend struct TestAccess;
    struct Tasks {
        std::vector<std::size_t> ids;
        std::vector<bool> queued;
        void init(std::size_t n) {
            ids.reserve(n);
            queued.resize(n);
        }
        void add(std::size_t id) {
            if (!queued[id]) {
                queued[id] = true;
                ids.push_back(id);
            }
        }
        void clear() {
            for (auto id : ids)
                queued[id] = false;
            ids.clear();
        }
    };
    enum class Phase { Idle, Work, Arbitration, Xfer, Signal };
    std::vector<ModuleRecord> modules_;
    std::vector<RuleRecord> rules_{1};
    std::vector<QueueBase *> queues_;
    std::vector<SignalBase *> signals_;
    std::vector<SignalId> signalOrder_;
    std::map<std::vector<RuleId>, SourceSlots> sourceSlots_;
    Tasks moduleTasks_, ruleTasks_, nextRules_, signalTasks_;
    std::vector<QueueId> used_;
    std::vector<RuleId> accepted_;
    using Event = std::pair<Tick, ModuleId>;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events_;
    std::optional<ModuleId> activeModule_;
    std::optional<RuleId> activeRule_;
    Tick tick_{};
    bool frozen_{}, failed_{};
    Phase phase_{Phase::Idle};
    Stats stats_;
    void constructing() const;
    void executing(RuleId) const;
    void prepare(RuleId, QueueBase &, bool, bool);
    void discard(RuleId);
    void notifyChanged(ResourceBase &);
    void evaluate(SignalId, bool initial);
    bool pending(RuleId) const;
    void work(ModuleId);
    void arbitrate(RuleId);
};
} // namespace gfsim
