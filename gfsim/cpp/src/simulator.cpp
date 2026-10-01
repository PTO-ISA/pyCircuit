#include <gfsim/simulator.hpp>
#include <string>

namespace gfsim {
namespace {
void bump(Tick &counter) {
    counter = checkedAdd(counter, 1);
}
} // namespace
void Simulator::constructing() const {
    if (frozen_ || failed_)
        throw std::logic_error("Simulator construction is closed");
}
ModuleId Simulator::addModule(void *object, Work work) {
    constructing();
    if (!object || !work)
        throw std::invalid_argument("null Module entry");
    moduleEntries_.push_back({object, work});
    modules_.emplace_back();
    return modules_.size() - 1;
}
RuleId Simulator::addRule(ModuleId owner, Arbitrate arbitrate) {
    constructing();
    if (owner >= modules_.size())
        throw std::out_of_range("Rule owner");
    entries_.push_back({owner, arbitrate});
    rules_.emplace_back();
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
void Simulator::bind(RuleId r, QueueBase &q, unsigned ops) {
    constructing();
    if (!r || r >= rules_.size() || q.sim_ != this || !ops || (ops & ~7U))
        throw std::invalid_argument("invalid binding");
    q.registerSource(r, ops);
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
            q->readers_.resize(modules_.size());
            q->freeze();
        }
        moduleTasks_.init(modules_.size());
        ruleTasks_.init(rules_.size());
        visited_.resize(rules_.size());
        visiting_.resize(rules_.size());
        used_.reserve(queues_.size());
        changed_.reserve(queues_.size());
        accepted_.reserve(rules_.size());
        stack_.reserve(rules_.size());
        frozen_ = true;
    } catch (...) {
        failed_ = true;
        throw;
    }
}
void Simulator::executing(RuleId r) const {
    if (failed_ || phase_ != Phase::Work || activeRule_ != r || !rules_.at(r).executing)
        throw std::logic_error("operation outside executing Rule");
}
void Simulator::recordRead(ModuleId m, QueueBase &q, std::optional<RuleId> r) {
    if (failed_ || phase_ != Phase::Work || activeModule_ != m || q.sim_ != this)
        throw std::logic_error("read outside owning Module Work");
    q.readers_[m] = checkedAdd(modules_[m].readGen, 1);
    if (r) {
        executing(*r);
        auto &deps = rules_[*r].deps;
        for (const auto &d : deps) {
            bump(stats_.depSearches);
            if (d.queue == q.id_)
                return;
        }
        deps.push_back({q.id_, q.version_});
    }
}
void QueueBase::prepare(RuleId r, bool readsTarget) {
    if (!sim_)
        throw std::logic_error("unattached Queue");
    sim_->prepare(r, *this, readsTarget);
}
void Simulator::prepare(RuleId r, QueueBase &q, bool readsTarget) {
    executing(r);
    if (readsTarget) {
        recordRead(entries_[r].owner, q, r);
        if (q.empty())
            throw std::logic_error("empty target; generated code must abort explicitly");
    }
    auto &participants = rules_[r].participants;
    if (std::find(participants.begin(), participants.end(), q.id_) == participants.end())
        participants.push_back(q.id_);
}
void Simulator::discard(RuleId r) {
    auto &record = rules_[r];
    for (auto q : record.participants)
        queues_[q]->cancel(r);
    record.deps.clear();
    record.participants.clear();
    record.wakeRequests.clear();
    record.complete = record.executing = false;
}
bool Simulator::begin(RuleId r, bool sameArgs) {
    if (failed_ || !r || r >= rules_.size() || phase_ != Phase::Work ||
        activeModule_ != entries_[r].owner || activeRule_)
        throw std::logic_error("invalid or nested Rule Work");
    auto &record = rules_[r];
    if (record.selectedTick == tick_)
        return false;
    record.selectedTick = tick_;
    modules_[entries_[r].owner].selected.push_back(r);
    bool valid = cache_ && record.complete && record.hasEffects() && sameArgs;
    if (valid)
        for (auto d : record.deps) {
            bump(stats_.versionChecks);
            if (queues_[d.queue]->version_ != d.version) {
                valid = false;
                break;
            }
        }
    if (valid) {
        bump(stats_.cacheHits);
        for (auto d : record.deps)
            recordRead(entries_[r].owner, *queues_[d.queue]);
        return false;
    }
    discard(r);
    record.executing = true;
    activeRule_ = r;
    bump(stats_.ruleWork);
    bump(record.calls);
    return true;
}
void Simulator::completeRule(RuleId r) {
    executing(r);
    rules_[r].complete = true;
    rules_[r].executing = false;
    activeRule_.reset();
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
void Simulator::wakeup(ModuleId m, Tick time) {
    events_.emplace(time, m);
    bump(stats_.events);
}
void Simulator::work(ModuleId m) {
    auto &record = modules_[m];
    checkedAdd(record.readGen, 1);
    record.previous.swap(record.selected);
    record.selected.clear();
    record.workedTick = tick_;
    activeModule_ = m;
    bump(stats_.moduleWork);
    bump(record.calls);
    moduleEntries_[m].work(moduleEntries_[m].object);
    if (activeRule_)
        throw std::logic_error("generated Rule omitted complete/abort");
    activeModule_.reset();
    for (auto r : record.selected)
        ruleTasks_.add(r, tick_ + 1);
    for (auto r : record.previous)
        if (rules_[r].selectedTick != tick_)
            discard(r);
    record.previous.clear();
    bump(record.readGen);
}
bool Simulator::pending(RuleId r) const {
    const auto &record = rules_[r];
    return record.complete && record.hasEffects() && record.acceptedTick != tick_;
}
bool Simulator::arbitrateRule(RuleId r) {
    if (failed_ || phase_ != Phase::Arbitration || arbitrating_ != r)
        throw std::logic_error("arbitration outside scheduler entry");
    if (!pending(r))
        return false;
    auto &record = rules_[r];
    for (auto q : record.participants) {
        bump(stats_.queueChecks);
        if (!queues_[q]->canAccept(r))
            return false;
    }
    // Validate arithmetic before confirming any part of this Rule.
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
    bump(stats_.accepted);
    for (auto request : record.wakeRequests)
        wakeup(request.module, checkedAdd(tick_, request.delay));
    return true;
}
void Simulator::visit(RuleId root) {
    const Tick tag = tick_ + 1;
    if (!pending(root) || visited_[root] == tag)
        return;
    stack_.clear();
    stack_.push_back({root, 0});
    visited_[root] = tag;
    visiting_[root] = true;
    while (!stack_.empty()) {
        stats_.maxStack = std::max(stats_.maxStack, static_cast<Tick>(stack_.size()));
        auto &frame = stack_.back();
        const RuleId r = frame.rule;
        const auto &record = rules_[r];
        if (frame.cursor < record.participants.size()) {
            auto &q = *queues_[record.participants[frame.cursor++]];
            auto child = q.popRule_;
            if (!q.pendingPush(r) || q.pushSpace(r) || !child || *child == r || !pending(*child) ||
                !q.pendingPop(*child))
                continue;
            bump(stats_.capacityEdges);
            if (visited_[*child] == tag) {
                if (visiting_[*child])
                    throw CapacityCycle("tick " + std::to_string(tick_) +
                                        ": necessary capacity cycle at Rule " +
                                        std::to_string(*child));
                continue;
            }
            visited_[*child] = tag;
            visiting_[*child] = true;
            stack_.push_back({*child, 0});
            continue;
        }
        bump(stats_.dfsVisits);
        arbitrating_ = r;
        const auto &entry = entries_[r];
        bool success = entry.arbitrate
                           ? entry.arbitrate(moduleEntries_[entry.owner].object, *this, r)
                           : arbitrateRule(r);
        arbitrating_.reset();
        if (success != (record.acceptedTick == tick_))
            throw std::logic_error("arbitration entry returned inconsistent result");
        visiting_[r] = false;
        stack_.pop_back();
        if (success) {
            accepted_.push_back(r);
            for (auto qid : record.participants) {
                auto &q = *queues_[qid];
                auto producer = q.pushRule_;
                // Accepted pop is identified by its unique source and accepted summary.
                if (q.popRule_ == r && producer && pending(*producer) && q.pendingPush(*producer)) {
                    if (q.acceptedPopFor(r))
                        ruleTasks_.add(*producer, tag);
                }
            }
        }
    }
}
std::span<const RuleId> Simulator::step() {
    if (!frozen_ || failed_ || phase_ != Phase::Idle)
        throw std::logic_error("Simulator is not runnable");
    try {
        checkedAdd(tick_, 1);
        accepted_.clear();
        changed_.clear();
        if (!tick_)
            for (ModuleId m = 0; m < modules_.size(); ++m)
                moduleTasks_.add(m, tick_ + 1);
        while (!events_.empty() && events_.top().first <= tick_) {
            moduleTasks_.add(events_.top().second, tick_ + 1);
            events_.pop();
            bump(stats_.dueEvents);
        }
        phase_ = Phase::Work;
        while (moduleTasks_.pending())
            work(moduleTasks_.take());
        phase_ = Phase::Arbitration;
        while (ruleTasks_.pending())
            visit(ruleTasks_.take());
        phase_ = Phase::Xfer;
        for (auto q : used_)
            if (queues_[q]->xfer())
                changed_.push_back(q);
        for (auto q : changed_)
            for (ModuleId m = 0; m < modules_.size(); ++m) {
                bump(stats_.readerChecks);
                auto gen = queues_[q]->readers_[m];
                if (gen && gen == modules_[m].readGen)
                    wakeup(m, tick_ + 1);
            }
        for (auto r : accepted_)
            discard(r);
        used_.clear();
        moduleTasks_.clear();
        ruleTasks_.clear();
        ++tick_;
        phase_ = Phase::Idle;
        return accepted_;
    } catch (...) {
        failed_ = true;
        throw;
    }
}
std::vector<std::pair<Tick, ModuleId>> Simulator::events() const {
    auto copy = events_;
    std::vector<Event> result;
    result.reserve(copy.size());
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    return result;
}
} // namespace gfsim
