#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
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
struct CapacityCycle : std::runtime_error {
    using runtime_error::runtime_error;
};
inline Tick checkedAdd(Tick a, Tick b) {
    if (b > std::numeric_limits<Tick>::max() - a)
        throw std::overflow_error("GFSim counter overflow");
    return a + b;
}
struct Stats {
    Tick moduleWork{}, ruleWork{}, cacheHits{}, versionChecks{}, depSearches{}, readerChecks{};
    Tick dfsVisits{}, capacityEdges{}, queueChecks{}, accepted{}, events{}, dueEvents{}, maxStack{};
};
struct ReadDep {
    QueueId queue;
    Tick version;
};
struct WakeRequest {
    ModuleId module;
    Tick delay;
};
struct ModuleRecord {
    std::optional<Tick> workedTick;
    Tick readGen{};
    std::vector<RuleId> selected, previous;
    Tick calls{};
};
struct RuleRecord {
    std::vector<ReadDep> deps;
    std::vector<QueueId> participants;
    std::vector<WakeRequest> wakeRequests;
    bool complete{}, executing{};
    std::optional<Tick> selectedTick, acceptedTick;
    Tick calls{};
    bool hasEffects() const { return !participants.empty() || !wakeRequests.empty(); }
};
template <class T> struct ParameterCache {
    std::optional<T> candidate;
};
class Simulator;
struct TestAccess; // Narrow boundary tests; no runtime mutation API.
class QueueBase {
  public:
    QueueBase() = default;
    virtual ~QueueBase() = default;
    QueueBase(const QueueBase &) = delete;
    QueueBase &operator=(const QueueBase &) = delete;
    QueueId id() const { return id_; }
    Tick stateVersion() const { return version_; }
    std::span<const Tick> readers() const { return readers_; }
    std::optional<RuleId> popRule() const { return popRule_; }
    std::optional<RuleId> pushRule() const { return pushRule_; }
    virtual std::size_t size() const = 0;
    virtual std::size_t capacity() const = 0;
    bool empty() const { return size() == 0; }
    bool full() const { return size() == capacity(); }
    virtual std::size_t sourceCount() const = 0;

  protected:
    void prepare(RuleId, bool readsTarget);
    void changed() { version_ = checkedAdd(version_, 1); }

  private:
    friend class Simulator;
    friend struct TestAccess;
    Simulator *sim_{};
    QueueId id_{};
    Tick version_{};
    std::vector<Tick> readers_;
    std::optional<RuleId> popRule_, pushRule_;
    std::optional<Tick> usedTick_;
    virtual void registerSource(RuleId, unsigned) = 0;
    virtual void freeze() = 0;
    virtual bool pendingPop(RuleId) const = 0;
    virtual bool acceptedPopFor(RuleId) const = 0;
    virtual bool pendingPush(RuleId) const = 0;
    virtual bool pushSpace(RuleId) const = 0;
    virtual bool canAccept(RuleId) const = 0;
    virtual void accept(RuleId) = 0;
    virtual void cancel(RuleId) = 0;
    virtual bool xfer() = 0;
};
enum Operation : unsigned { Pop = 1, Push = 2, Revise = 4 };

class Simulator {
  public:
    using Work = void (*)(void *);
    using Arbitrate = bool (*)(void *, Simulator &, RuleId);
    explicit Simulator(bool cache = true) : cache_(cache) {}
    Simulator(const Simulator &) = delete;
    Simulator &operator=(const Simulator &) = delete;
    ModuleId addModule(void *object, Work work);
    template <auto Method, class T> ModuleId addModule(T &object) {
        return addModule(&object, [](void *p) { (static_cast<T *>(p)->*Method)(); });
    }
    RuleId addRule(ModuleId owner, Arbitrate arbitrate = nullptr);
    QueueId addQueue(QueueBase &queue);
    void bind(RuleId rule, QueueBase &queue, unsigned operations);
    void freeze();
    std::span<const RuleId> step(); // Valid until next step; no allocation for returned IDs.
    bool beginRule(RuleId rule) { return begin(rule, true); }
    template <class T> bool beginRule(RuleId rule, ParameterCache<T> &storage, const T &args) {
        bool execute = begin(rule, storage.candidate && *storage.candidate == args);
        if (execute)
            storage.candidate = args;
        return execute;
    }
    void completeRule(RuleId);
    void abortRule(RuleId);
    void recordRead(ModuleId, QueueBase &, std::optional<RuleId> = {});
    void requestWakeup(RuleId, ModuleId, Tick delay);
    bool arbitrateRule(RuleId);
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
    struct ModuleEntry {
        void *object;
        Work work;
    };
    struct RuleEntry {
        ModuleId owner;
        Arbitrate arbitrate;
    };
    struct Tasks {
        std::vector<std::size_t> ids;
        std::vector<Tick> tags;
        std::size_t count{}, cursor{};
        void init(std::size_t n) {
            ids.resize(n);
            tags.resize(n);
        }
        void add(std::size_t id, Tick tag) {
            if (tags.at(id) != tag) {
                tags[id] = tag;
                ids.at(count++) = id;
            }
        }
        bool pending() const { return cursor < count; }
        std::size_t take() { return ids[cursor++]; }
        void clear() { count = cursor = 0; }
    };
    struct Frame {
        RuleId rule;
        std::size_t cursor;
    };
    enum class Phase { Idle, Work, Arbitration, Xfer };
    std::vector<ModuleEntry> moduleEntries_;
    std::vector<RuleEntry> entries_{{}}; // Source 0 is reserved.
    std::vector<ModuleRecord> modules_;
    std::vector<RuleRecord> rules_{1};
    std::vector<QueueBase *> queues_;
    Tasks moduleTasks_, ruleTasks_;
    std::vector<Tick> visited_;
    std::vector<bool> visiting_;
    std::vector<QueueId> used_, changed_;
    std::vector<RuleId> accepted_;
    std::vector<Frame> stack_;
    using Event = std::pair<Tick, ModuleId>;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events_;
    std::optional<ModuleId> activeModule_;
    std::optional<RuleId> activeRule_, arbitrating_;
    Tick tick_{};
    bool cache_, frozen_{}, failed_{};
    Phase phase_{Phase::Idle};
    Stats stats_;
    void constructing() const;
    void executing(RuleId) const;
    void prepare(RuleId, QueueBase &, bool);
    bool begin(RuleId, bool sameArgs);
    void discard(RuleId);
    bool pending(RuleId) const;
    void work(ModuleId);
    void visit(RuleId);
    void wakeup(ModuleId, Tick);
};
} // namespace gfsim
