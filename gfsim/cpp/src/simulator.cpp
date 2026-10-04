#include <gfsim/simulator.hpp>

namespace gfsim {
namespace {
void bump(Tick &counter) {
    counter = checkedAdd(counter, 1);
}
template <class T> void unique(std::vector<T> &values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}
} // namespace
Simulator::~Simulator() {
    for (auto *q : queues_) {
        q->sim_ = nullptr;
        q->sourceSlots_ = nullptr;
    }
    for (auto *s : signals_)
        s->sim_ = nullptr;
}
void QueueBase::bindSlots(std::vector<RuleId> sources) {
    auto [entry, inserted] = sim_->sourceSlots_.try_emplace(std::move(sources));
    auto &table = entry->second;
    if (inserted && entry->first.size() > 1) {
        const auto &rules = entry->first;
        table.first = rules[1];
        table.indices.assign(rules.back() - table.first + 1,
                             std::numeric_limits<std::size_t>::max());
        for (std::size_t i = 1; i < rules.size(); ++i)
            table.indices[rules[i] - table.first] = i;
    }
    sourceSlots_ = &table;
}
void Simulator::constructing() const {
    if (frozen_ || failed_)
        throw std::logic_error("Simulator construction is closed");
}
ModuleId Simulator::addModule(void *object, Work work) {
    constructing();
    if (!object || !work)
        throw std::invalid_argument("null Module entry");
    modules_.push_back({object, work, {}, 0, {}});
    return modules_.size() - 1;
}
RuleId Simulator::addRule(ModuleId owner) {
    constructing();
    if (owner >= modules_.size())
        throw std::out_of_range("Rule owner");
    modules_[owner].rules.push_back(rules_.size());
    rules_.emplace_back();
    rules_.back().owner = owner;
    return rules_.size() - 1;
}
QueueId Simulator::addQueue(QueueBase &q) {
    constructing();
    if (q.sim_)
        throw std::logic_error("Queue already attached");
    q.id_ = queues_.size();
    queues_.push_back(&q);
    q.sim_ = this;
    return q.id_;
}
SignalId Simulator::addSignal(SignalBase &s) {
    constructing();
    if (s.sim_)
        throw std::logic_error("Signal already attached");
    s.id_ = signals_.size();
    signals_.push_back(&s);
    s.sim_ = this;
    return s.id_;
}
void Simulator::declareResource(ModuleId m, ResourceBase &resource) {
    constructing();
    if (m >= modules_.size() || resource.sim_ != this)
        throw std::invalid_argument("invalid Module resource declaration");
    resource.readers_.push_back(m);
}
void Simulator::declareInput(SignalBase &s, ResourceBase &input) {
    constructing();
    if (s.sim_ != this || input.sim_ != this)
        throw std::invalid_argument("invalid Signal input declaration");
    input.signalReaders_.push_back(s.id_);
}
void Simulator::bind(RuleId r, QueueBase &q, unsigned ops) {
    constructing();
    if (!r || r >= rules_.size() || q.sim_ != this || !ops || (ops & ~7U))
        throw std::invalid_argument("invalid binding");
    q.registerSource(r, ops);
    declareResource(rules_[r].owner, q);
    // Unique pop/push sources are a model precondition, not a port arbiter.
    if (ops & Pop)
        q.popRule_ = r;
    if (ops & Push)
        q.pushRule_ = r;
}
void Simulator::freeze() {
    constructing();
    try {
        for (auto *q : queues_) {
            unique(q->readers_);
            unique(q->signalReaders_);
            q->freeze();
        }
        std::vector<std::size_t> indegree(signals_.size());
        for (auto *s : signals_) {
            unique(s->readers_);
            unique(s->signalReaders_);
            for (auto reader : s->signalReaders_)
                ++indegree[reader];
        }
        // Queue inputs are ready before Signal evaluation starts. Only
        // Signal -> Signal edges constrain the order within this phase.
        signalOrder_.reserve(signals_.size());
        for (SignalId s = 0; s < signals_.size(); ++s)
            if (!indegree[s])
                signalOrder_.push_back(s);
        for (std::size_t i = 0; i < signalOrder_.size(); ++i)
            for (auto reader : signals_[signalOrder_[i]]->signalReaders_)
                if (!--indegree[reader])
                    signalOrder_.push_back(reader);
        if (signalOrder_.size() != signals_.size())
            throw std::logic_error("Signal dependency cycle");
        moduleTasks_.init(modules_.size());
        ruleTasks_.init(rules_.size());
        nextRules_.init(rules_.size());
        signalTasks_.init(signals_.size());
        used_.reserve(queues_.size());
        accepted_.reserve(rules_.size());
        frozen_ = true;
    } catch (...) {
        failed_ = true;
        throw;
    }
}
void Simulator::executing(RuleId r) const {
    if (failed_ || phase_ != Phase::Work || activeRule_ != r)
        throw std::logic_error("operation outside executing Rule");
}
void Simulator::notifyChanged(ResourceBase &resource) {
    bump(stats_.changeNotifications);
    for (auto m : resource.readers_)
        moduleTasks_.add(m);
    for (auto s : resource.signalReaders_)
        signalTasks_.add(s);
}
void Simulator::evaluate(SignalId id, bool initial) {
    auto &s = *signals_[id];
    phase_ = Phase::Signal;
    bump(s.evaluations_);
    bump(stats_.signalWork);
    if (s.evaluate() && !initial)
        notifyChanged(s);
}
void QueueBase::prepare(RuleId r, bool readsTarget, bool first) {
    if (!sim_)
        throw std::logic_error("unattached Queue");
    sim_->prepare(r, *this, readsTarget, first);
}
void Simulator::prepare(RuleId r, QueueBase &q, bool readsTarget, bool first) {
    executing(r);
    if (readsTarget && q.empty())
        throw NeedInput();
    if (first)
        rules_[r].participants.push_back(q.id_);
}
void Simulator::discard(RuleId r) {
    auto &record = rules_[r];
    for (auto q : record.participants)
        queues_[q]->cancel(r);
    record.participants.clear();
    record.wakeRequests.clear();
    record.complete = false;
}
bool Simulator::beginRule(RuleId r) {
    if (failed_ || !r || r >= rules_.size() || phase_ != Phase::Work ||
        activeModule_ != rules_[r].owner || activeRule_)
        throw std::logic_error("invalid or nested Rule Work");
    auto &record = rules_[r];
    if (record.selectedTick == tick_)
        return false;
    record.selectedTick = tick_;
    activeRule_ = r;
    bump(stats_.ruleWork);
    bump(record.calls);
    return true;
}
void Simulator::completeRule(RuleId r) {
    executing(r);
    rules_[r].complete = true;
    activeRule_.reset();
    if (rules_[r].hasEffects())
        ruleTasks_.add(r);
}
void Simulator::abortRule(RuleId r) {
    executing(r);
    discard(r);
    activeRule_.reset();
}
void Simulator::requestWakeup(RuleId r, ModuleId m, Tick delay) {
    executing(r);
    if (m >= modules_.size() || !delay)
        throw std::invalid_argument("invalid future wake request");
    rules_[r].wakeRequests.push_back({m, delay});
}
void Simulator::work(ModuleId m) {
    auto &record = modules_[m];
    // An activation replaces all proposals owned by this Module. Sleeping
    // Modules retain their complete pending proposals across ticks.
    for (auto r : record.rules)
        discard(r);
    record.workedTick = tick_;
    activeModule_ = m;
    bump(stats_.moduleWork);
    bump(record.calls);
    try {
        record.work(record.object);
    } catch (const NeedInput &) {
        if (activeRule_)
            throw std::logic_error("Rule must catch NeedInput and abort explicitly");
    }
    if (activeRule_)
        throw std::logic_error("generated Rule omitted complete/abort");
    activeModule_.reset();
}
bool Simulator::pending(RuleId r) const {
    const auto &record = rules_[r];
    return record.complete && record.hasEffects() && record.acceptedTick != tick_;
}
void Simulator::arbitrate(RuleId r) {
    if (!pending(r))
        return;
    bump(stats_.arbitrationAttempts);
    auto &record = rules_[r];
    for (auto q : record.participants) {
        bump(stats_.queueChecks);
        if (!queues_[q]->canAccept(r))
            return;
    }
    // Check all event arithmetic before accepting any part of this Rule.
    for (auto request : record.wakeRequests)
        checkedAdd(tick_, request.delay);
    for (auto qid : record.participants) {
        auto &q = *queues_[qid];
        q.accept(r);
        if (q.usedTick_ != tick_) {
            q.usedTick_ = tick_;
            used_.push_back(qid);
        }
    }
    record.acceptedTick = tick_;
    accepted_.push_back(r);
    bump(stats_.accepted);
    for (auto request : record.wakeRequests) {
        events_.emplace(checkedAdd(tick_, request.delay), request.module);
        bump(stats_.events);
    }
    for (auto qid : record.participants) {
        auto &q = *queues_[qid];
        if (q.popRule_ == r && q.pushRule_ && q.acceptedPopFor(r) && pending(*q.pushRule_))
            nextRules_.add(*q.pushRule_);
    }
}
std::span<const RuleId> Simulator::step() {
    if (!frozen_ || failed_ || phase_ != Phase::Idle)
        throw std::logic_error("Simulator is not runnable");
    try {
        checkedAdd(tick_, 1);
        accepted_.clear();
        if (!tick_) {
            for (auto s : signalOrder_)
                evaluate(s, true);
            for (ModuleId m = 0; m < modules_.size(); ++m)
                moduleTasks_.add(m);
        }
        while (!events_.empty() && events_.top().first <= tick_) {
            moduleTasks_.add(events_.top().second);
            events_.pop();
            bump(stats_.dueEvents);
        }
        phase_ = Phase::Work;
        for (auto m : moduleTasks_.ids)
            work(m);
        moduleTasks_.clear(); // Xfer fills this buffer for the next tick.
        phase_ = Phase::Arbitration;
        while (!ruleTasks_.ids.empty()) {
            bump(stats_.deltaRounds);
            for (auto r : ruleTasks_.ids)
                arbitrate(r);
            ruleTasks_.clear();
            std::swap(ruleTasks_, nextRules_);
        }
        phase_ = Phase::Xfer;
        for (auto q : used_)
            if (queues_[q]->xfer())
                notifyChanged(*queues_[q]);
        // Upstream changes can activate only later Signals in this fixed
        // order. A diamond join sees final inputs and runs at most once.
        if (!signalTasks_.ids.empty())
            for (auto s : signalOrder_)
                if (signalTasks_.queued[s])
                    evaluate(s, false);
        signalTasks_.clear();
        for (auto r : accepted_) {
            auto &record = rules_[r];
            record.participants.clear();
            record.wakeRequests.clear();
            record.complete = false;
        }
        used_.clear();
        phase_ = Phase::Idle;
        ++tick_;
        return accepted_;
    } catch (...) {
        failed_ = true;
        throw;
    }
}
std::vector<std::pair<Tick, ModuleId>> Simulator::events() const {
    auto copy = events_;
    std::vector<Event> result;
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    return result;
}
} // namespace gfsim
