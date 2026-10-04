#include <gfsim/simulator.hpp>
#include <string>

namespace gfsim {
thread_local Simulator *Simulator::active_ = nullptr;
namespace {
void bump(Tick &counter) { counter = checkedAdd(counter, 1); }
} // namespace
Simulator::~Simulator() {
    for (auto *q : queues_)
        q->sim_ = nullptr;
    for (auto *s : signals_)
        s->sim_ = nullptr;
}
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
    entries_.push_back({owner, arbitrate, modules_[owner].rules.size()});
    modules_[owner].rules.push_back(rules_.size());
    rules_.emplace_back();
    return rules_.size() - 1;
}
QueueId Simulator::addQueue(QueueBase &q) {
    constructing();
    if (q.sim_)
        throw std::logic_error("Queue already attached");
    q.id_ = q.resourceId_ = queues_.size();
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
    if (auto *signal = dynamic_cast<SignalBase *>(&resource)) {
        declareResource(m, *signal);
        return;
    }
    constructing();
    if (m >= modules_.size() || resource.sim_ != this)
        throw std::invalid_argument("invalid Module resource declaration");
    auto &resources = modules_[m].resources;
    if (std::find(resources.begin(), resources.end(), &resource) == resources.end())
        resources.push_back(&resource);
}
void Simulator::declareResource(ModuleId m, SignalBase &s) {
    constructing();
    if (m >= modules_.size() || s.sim_ != this)
        throw std::invalid_argument("invalid Module Signal declaration");
    s.moduleBindings_.push_back(m);
}
void Simulator::declareInput(RuleId r, SignalBase &s) {
    constructing();
    if (!r || r >= rules_.size() || s.sim_ != this)
        throw std::invalid_argument("invalid Rule Signal declaration");
    declareResource(entries_[r].owner, s);
    s.ruleBindings_.push_back(r);
}
void Simulator::declareInput(SignalBase &s, QueueBase &q) {
    constructing();
    if (s.sim_ != this || q.sim_ != this)
        throw std::invalid_argument("invalid Signal input declaration");
    s.inputs_.push_back(q.id_);
}
void Simulator::bind(RuleId r, QueueBase &q, unsigned ops) {
    constructing();
    if (!r || r >= rules_.size() || q.sim_ != this || !ops || (ops & ~7U))
        throw std::invalid_argument("invalid binding");
    q.registerSource(r, ops);
    bindings_.push_back({r, &q});
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
            q->moduleSlots_.assign(modules_.size(), noSlot);
            q->freeze();
        }
        for (auto *s : signals_) {
            s->resourceId_ = queues_.size() + s->id_;
            s->moduleSlots_.assign(modules_.size(), noSlot);
            std::sort(s->inputs_.begin(), s->inputs_.end());
            s->inputs_.erase(std::unique(s->inputs_.begin(), s->inputs_.end()), s->inputs_.end());
            for (auto qid : s->inputs_)
                queues_[qid]->signalReaders_.push_back(s->id_);
        }
        for (ModuleId mid = 0; mid < modules_.size(); ++mid) {
            auto &m = modules_[mid];
            std::sort(m.resources.begin(), m.resources.end(),
                      [](auto *a, auto *b) { return a->resourceId_ < b->resourceId_; });
            m.wordCount = (m.rules.size() + 63) / 64;
            m.controlReads.resize(m.resources.size());
            m.ruleReaders.resize(m.resources.size() * m.wordCount);
            m.dirtyWords.resize(m.wordCount);
            m.selected.reserve(m.rules.size());
            m.previous.reserve(m.rules.size());
            for (std::size_t i = 0; i < m.resources.size(); ++i) {
                auto &resource = *m.resources[i];
                resource.moduleSlots_[mid] = i;
                resource.readers_.push_back({mid, i});
            }
        }
        for (auto *s : signals_) {
            auto &bindings = s->moduleBindings_;
            std::sort(bindings.begin(), bindings.end());
            bindings.erase(std::unique(bindings.begin(), bindings.end()), bindings.end());
            for (auto mid : bindings) {
                s->moduleSlots_[mid] = s->dependents_.size();
                s->dependents_.push_back({mid, {}});
            }
            for (auto rid : s->ruleBindings_) {
                const auto &entry = entries_[rid];
                auto &mask = s->dependents_[s->moduleSlots_[entry.owner]].dirtyMask;
                mask.resize(modules_[entry.owner].wordCount);
                mask[entry.local / 64] |= std::uint64_t{1} << (entry.local % 64);
            }
            // Repeated Rule declarations collapse into the fixed masks.
            s->moduleBindings_.clear();
            s->ruleBindings_.clear();
        }
        for (auto binding : bindings_)
            if (binding.queue->moduleSlots_[entries_[binding.rule].owner] == noSlot)
                throw std::logic_error("bound Queue missing Module resource declaration");
        signalTasks_.init(signals_.size());
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
void ResourceBase::observe() const {
    if (Simulator::active_)
        Simulator::active_->observe(*this);
    else if (sim_ && sim_->failed_)
        throw std::logic_error("read on failed Simulator");
}
void Simulator::observe(const ResourceBase &resource) {
    if (failed_)
        throw std::logic_error("read on failed Simulator");
    if (resource.sim_ != this)
        throw std::logic_error("read of unregistered or foreign resource");
    if (phase_ == Phase::Signal) {
        if (resource.resourceId_ >= queues_.size())
            throw std::logic_error("Signal helper cannot read another Signal");
#ifndef NDEBUG
        const auto &s = *signals_[*activeSignal_];
        if (!std::binary_search(s.inputs_.begin(), s.inputs_.end(), resource.resourceId_))
            throw std::logic_error("undeclared Signal input");
#endif
        return;
    }
    if (phase_ != Phase::Work)
        return; // Testbench and scheduler observation.
    if (!activeModule_)
        throw std::logic_error("read outside Module context");
    if (resource.resourceId_ >= queues_.size()) {
#ifndef NDEBUG
        const auto &s = *signals_[resource.resourceId_ - queues_.size()];
        const auto slot = s.moduleSlots_[*activeModule_];
        if (slot == noSlot)
            throw std::logic_error("undeclared Module Signal");
        if (activeRule_) {
            const auto local = entries_[*activeRule_].local;
            const auto &mask = s.dependents_[slot].dirtyMask;
            if (mask.empty() || !(mask[local / 64] & (std::uint64_t{1} << (local % 64))))
                throw std::logic_error("undeclared Rule Signal");
        }
#endif
        return; // Signal reads never change dynamic Queue subscriptions.
    }
    auto &m = modules_[*activeModule_];
    auto slot = resource.moduleSlots_[*activeModule_];
    if (slot == noSlot)
        throw std::logic_error("undeclared Module resource");
    if (!activeRule_) {
        m.controlReads[slot] = m.readGen;
        return;
    }
    const auto local = entries_[*activeRule_].local;
    const auto bit = std::uint64_t{1} << (local % 64);
    auto &word = m.ruleReaders[slot * m.wordCount + local / 64];
    if (!(word & bit)) {
        word |= bit;
        rules_[*activeRule_].readSlots.push_back(slot);
    }
}
bool Simulator::reads(ModuleId mid, const ResourceBase &resource) const {
    if (!frozen_ || resource.sim_ != this)
        return false;
    auto slot = resource.moduleSlots_.at(mid);
    if (slot == noSlot)
        return false;
    if (resource.resourceId_ >= queues_.size())
        return true; // Static Signal activation relation, even before first Work.
    const auto &m = modules_[mid];
    if (m.readGen && m.controlReads[slot] == m.readGen)
        return true;
    for (std::size_t w = 0; w < m.wordCount; ++w)
        if (m.ruleReaders[slot * m.wordCount + w])
            return true;
    return false;
}
bool Simulator::dirty(RuleId r) const {
    const auto &entry = entries_.at(r);
    if (!r || !frozen_)
        throw std::logic_error("invalid Rule dirty query");
    return (modules_[entry.owner].dirtyWords[entry.local / 64] >> (entry.local % 64)) & 1;
}
void Simulator::clearReads(RuleId r) {
    const auto &entry = entries_[r];
    auto &m = modules_[entry.owner];
    const auto mask = ~(std::uint64_t{1} << (entry.local % 64));
    for (auto slot : rules_[r].readSlots)
        m.ruleReaders[slot * m.wordCount + entry.local / 64] &= mask;
    rules_[r].readSlots.clear();
    m.dirtyWords[entry.local / 64] &= mask;
}
void Simulator::notifyChanged(ResourceBase &resource) {
    bump(stats_.changeNotifications);
    for (auto reader : resource.readers_) {
        auto &m = modules_[reader.module];
        bool live = m.readGen && m.controlReads[reader.slot] == m.readGen;
        for (std::size_t w = 0; w < m.wordCount; ++w) {
            bump(stats_.readerChecks);
            auto word = m.ruleReaders[reader.slot * m.wordCount + w];
            m.dirtyWords[w] |= word;
            live |= word != 0;
        }
        if (live)
            wakeup(reader.module, tick_ + 1);
    }
}
void Simulator::evaluate(SignalId id, bool initial) {
    auto &s = *signals_[id];
    bump(s.evaluations_);
    phase_ = Phase::Signal;
    activeSignal_ = id;
    bump(stats_.signalWork);
    bool changed = s.evaluate();
    activeSignal_.reset();
    if (changed && !initial) {
        bump(stats_.changeNotifications);
        for (const auto &dependent : s.dependents_) {
            auto &m = modules_[dependent.module];
            for (std::size_t w = 0; w < dependent.dirtyMask.size(); ++w) {
                bump(stats_.readerChecks);
                m.dirtyWords[w] |= dependent.dirtyMask[w];
            }
            wakeup(dependent.module, tick_ + 1);
        }
    }
}
void QueueBase::prepare(RuleId r, bool readsTarget, bool first) {
    if (!sim_)
        throw std::logic_error("unattached Queue");
    sim_->prepare(r, *this, readsTarget, first);
}
void Simulator::prepare(RuleId r, QueueBase &q, bool readsTarget, bool first) {
    executing(r);
    if (readsTarget) {
        observe(q);
        if (q.empty())
            throw NeedInput();
    }
    if (first)
        rules_[r].participants.push_back(q.id_);
}
void Simulator::discard(RuleId r) {
    auto &record = rules_[r];
    for (auto q : record.participants)
        queues_[q]->cancel(r);
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
    if (!sameArgs) {
        const auto &entry = entries_[r];
        modules_[entry.owner].dirtyWords[entry.local / 64] |= std::uint64_t{1}
                                                              << (entry.local % 64);
    }
    if (cache_ && record.complete && !dirty(r)) {
        bump(stats_.cacheHits);
        return false;
    }
    discard(r);
    clearReads(r);
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
    bump(record.readGen);
    record.previous.swap(record.selected);
    record.selected.clear();
    record.workedTick = tick_;
    activeModule_ = m;
    bump(stats_.moduleWork);
    bump(record.calls);
    try {
        moduleEntries_[m].work(moduleEntries_[m].object);
    } catch (const NeedInput &) {
        if (activeRule_)
            throw std::logic_error("Rule must catch NeedInput and abort explicitly");
        // Missing Module control input stops selection, preserving earlier Rules.
    }
    if (activeRule_)
        throw std::logic_error("generated Rule omitted complete/abort");
    activeModule_.reset();
    for (auto r : record.selected)
        ruleTasks_.add(r, tick_ + 1);
    for (auto r : record.previous)
        if (rules_[r].selectedTick != tick_) {
            discard(r);
            clearReads(r);
        }
    record.previous.clear();
}
bool Simulator::pending(RuleId r) const {
    const auto &record = rules_[r];
    return record.complete && record.hasEffects() && record.acceptedTick != tick_ && !dirty(r);
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
    if (!frozen_ || failed_ || phase_ != Phase::Idle || active_)
        throw std::logic_error("Simulator is not runnable");
    active_ = this;
    try {
        checkedAdd(tick_, 1);
        accepted_.clear();
        changed_.clear();
        if (!tick_) {
            for (SignalId s = 0; s < signals_.size(); ++s)
                evaluate(s, true);
            for (ModuleId m = 0; m < modules_.size(); ++m)
                moduleTasks_.add(m, tick_ + 1);
        }
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
        for (auto q : changed_) {
            notifyChanged(*queues_[q]);
            for (auto sid : queues_[q]->signalReaders_)
                signalTasks_.add(sid, tick_ + 1);
        }
        while (signalTasks_.pending())
            evaluate(signalTasks_.take(), false);
        signalTasks_.clear();
        for (auto r : accepted_)
            discard(r);
        used_.clear();
        moduleTasks_.clear();
        ruleTasks_.clear();
        ++tick_;
        phase_ = Phase::Idle;
        active_ = nullptr;
        return accepted_;
    } catch (...) {
        active_ = nullptr;
        activeSignal_.reset();
        activeRule_.reset();
        activeModule_.reset();
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
