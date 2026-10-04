#pragma once
#include <array>
#include <gfsim/queue.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>

namespace circuits {
using namespace gfsim;
using Word = std::uint32_t;
struct Value {
    std::int64_t seq{};
    Word value{};
    bool operator==(const Value &) const = default;
};
template <class T> struct Timed {
    Tick due{};
    T value{};
    bool operator==(const Timed &) const = default;
};
template <class T, std::size_t N> struct Receipt {
    Tick tick{};
    std::array<T, N> values{};
    bool operator==(const Receipt &) const = default;
};
template <class T>
    requires std::is_arithmetic_v<T>
void json(std::ostream &o, const T &v) {
    if constexpr (std::is_same_v<T, bool>)
        o << (v ? "true" : "false");
    else
        o << +v;
}
template <class... T> void jsonList(std::ostream &o, const T &...values);
template <class T, std::size_t N> void json(std::ostream &o, const std::array<T, N> &v) {
    o << '[';
    bool first = true;
    for (const auto &x : v) {
        if (!first)
            o << ',';
        first = false;
        json(o, x);
    }
    o << ']';
}
template <class T> void json(std::ostream &o, const std::vector<T> &v) {
    o << '[';
    bool first = true;
    for (const auto &x : v) {
        if (!first)
            o << ',';
        first = false;
        json(o, x);
    }
    o << ']';
}
inline void json(std::ostream &o, const Value &v) { jsonList(o, v.seq, v.value); }
template <class T> void json(std::ostream &o, const Timed<T> &v) { jsonList(o, v.due, v.value); }
template <class T, std::size_t N> void json(std::ostream &o, const Receipt<T, N> &v) {
    jsonList(o, v.tick, v.values);
}
template <class... T> void jsonList(std::ostream &o, const T &...values) {
    o << '[';
    std::size_t n = 0;
    ((o << (n++ ? "," : ""), json(o, values)), ...);
    o << ']';
}
struct Module {
    Simulator *e{};
    ModuleId mid{};
    std::vector<QueueBase *> resources;
    template <class T> const T *read(Queue<T> &q, std::optional<RuleId> = {}) {
        return q.tryPeek();
    }
    bool arbitrate(RuleId r) { return e->arbitrateRule(r); }
};
struct Netlist {
    struct Entry {
        Module *module;
        void *object;
        Simulator::Work work;
        Simulator::Arbitrate arbitrate;
    };
    struct Binding {
        RuleId rule;
        QueueBase *queue;
        unsigned operations;
    };
    std::vector<std::unique_ptr<QueueBase>> queues;
    std::vector<std::shared_ptr<void>> objects;
    std::vector<Entry> modules;
    std::vector<Module *> owners;
    std::vector<Binding> bindings;
    std::vector<std::function<void(std::ostream &)>> observers;
    std::unique_ptr<Simulator> sim;
    QueueBase *output{};
    template <class T> Queue<T> &queue(std::size_t capacity = 1, std::vector<T> initial = {}) {
        auto q = std::make_unique<Queue<T>>(capacity, std::move(initial));
        auto *p = q.get();
        queues.push_back(std::move(q));
        observers.push_back([p](std::ostream &o) {
            o << '[';
            for (std::size_t i = 0; i < p->size(); ++i) {
                if (i)
                    o << ',';
                json(o, p->at(i));
            }
            o << ']';
        });
        return *p;
    }
    template <class T, class... Args> T &module(Args &&...args) {
        auto obj = std::make_shared<T>(std::forward<Args>(args)...);
        auto *p = obj.get();
        p->mid = modules.size();
        objects.push_back(obj);
        modules.push_back({p, p, [](void *instance) { static_cast<T *>(instance)->Work(); },
                           [](void *instance, Simulator &, RuleId r) {
                               return static_cast<T *>(instance)->arbitrate(r);
                           }});
        return *p;
    }
    RuleId rule(Module &m, std::vector<QueueBase *> pops = {}, std::vector<QueueBase *> pushes = {},
                std::vector<QueueBase *> revises = {}) {
        owners.push_back(&m);
        RuleId r = owners.size();
        for (auto *q : pops)
            bindings.push_back({r, q, Pop});
        for (auto *q : pushes)
            bindings.push_back({r, q, Push});
        for (auto *q : revises)
            bindings.push_back({r, q, Revise});
        return r;
    }
    void finish(bool cache = true, bool reverse = false) {
        sim = std::make_unique<Simulator>(cache);
        if (reverse)
            std::reverse(modules.begin(), modules.end());
        for (auto &entry : modules) {
            entry.module->mid = sim->addModule(entry.object, entry.work);
            entry.module->e = sim.get();
        }
        // The generated arbitration trampoline dispatches through the ordinary Module instance.
        for (auto *owner : owners)
            sim->addRule(owner->mid, modules[owner->mid].arbitrate);
        for (auto &q : queues)
            sim->addQueue(*q);
        for (auto b : bindings) {
            sim->bind(b.rule, *b.queue, b.operations);
            sim->declareResource(owners[b.rule - 1]->mid, *b.queue);
        }
        for (auto &entry : modules)
            for (auto *q : entry.module->resources)
                sim->declareResource(entry.module->mid, *q);
        sim->freeze();
    }
    std::string snapshot() const {
        std::ostringstream o;
        o << '[';
        for (std::size_t i = 0; i < observers.size(); ++i) {
            if (i)
                o << ',';
            observers[i](o);
        }
        o << ']';
        return o.str();
    }
    std::string trace(std::span<const RuleId> accepted) const {
        std::ostringstream o;
        std::vector<RuleId> sorted(accepted.begin(), accepted.end());
        std::sort(sorted.begin(), sorted.end());
        o << "{\"accepted\":";
        json(o, sorted);
        o << ",\"queues\":" << snapshot() << ",\"versions\":[";
        for (std::size_t i = 0; i < queues.size(); ++i) {
            if (i)
                o << ',';
            o << queues[i]->stateVersion();
        }
        o << "],\"events\":[";
        bool first = true;
        for (auto [tick, mid] : sim->events()) {
            if (!first)
                o << ',';
            first = false;
            jsonList(o, tick, mid);
        }
        o << "],\"modules\":[";
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (i)
                o << ',';
            o << sim->module(i).calls;
        }
        o << "],\"rules\":[0";
        for (RuleId i = 1; i <= sim->ruleCount(); ++i)
            o << ',' << sim->rule(i).calls;
        o << "],\"reads\":[";
        for (ModuleId m = 0; m < modules.size(); ++m) {
            if (m)
                o << ',';
            o << '[';
            first = true;
            for (const auto &q : queues)
                if (sim->reads(m, *q)) {
                    if (!first)
                        o << ',';
                    first = false;
                    o << q->id();
                }
            o << ']';
        }
        const auto &s = sim->stats();
        o << "],\"stats\":{\"rule_work\":" << s.ruleWork << ",\"cache_hits\":" << s.cacheHits
          << ",\"max_stack\":" << s.maxStack << "}}";
        return o.str();
    }
};
template <class T> struct Source : Module {
    RuleId rid{};
    Queue<Timed<T>> &rom;
    Queue<T> &output;
    ParameterCache<Tick> args;
    Source(Queue<Timed<T>> &r, Queue<T> &o) : rom(r), output(o) {}
    void Work() { workSend(e->tick()); }
    void workSend(Tick now) {
        if (!e->beginRule(rid, args, now))
            return;
        auto p = read(rom, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        if (now < p->due)
            e->requestWakeup(rid, mid, p->due - now);
        else {
            rom.proposePop(rid);
            output.proposePush(rid, p->value);
        }
        e->completeRule(rid);
    }
};
template <class T> Source<T> &source(Netlist &n, std::vector<Timed<T>> schedule, Queue<T> &output) {
    auto &rom = n.queue<Timed<T>>(std::max<std::size_t>(1, schedule.size()), schedule);
    auto &m = n.module<Source<T>>(rom, output);
    m.rid = n.rule(m, {&rom}, {&output});
    return m;
}
template <class T, std::size_t N> struct Sink : Module {
    RuleId rid{}, timer{};
    std::array<Queue<T> *, N> inputs;
    Queue<Receipt<T, N>> &output;
    Tick period;
    ParameterCache<Tick> receiveArgs, timerArgs;
    Sink(std::array<Queue<T> *, N> in, Queue<Receipt<T, N>> &out, Tick p)
        : inputs(in), output(out), period(p) {}
    void Work() {
        Tick now = e->tick();
        if (now % period) {
            auto delay = period - now % period;
            if (e->beginRule(timer, timerArgs, delay)) {
                e->requestWakeup(timer, mid, delay);
                e->completeRule(timer);
            }
        } else
            workReceive(now);
    }
    void workReceive(Tick now) {
        if (!e->beginRule(rid, receiveArgs, now))
            return;
        Receipt<T, N> result;
        result.tick = now;
        for (std::size_t i = 0; i < N; ++i) {
            auto p = read(*inputs[i], rid);
            if (!p) {
                e->abortRule(rid);
                return;
            }
            result.values[i] = *p;
            inputs[i]->proposePop(rid);
        }
        output.proposePush(rid, result);
        e->completeRule(rid);
    }
};
template <class T, std::size_t N>
void sink(Netlist &n, std::array<Queue<T> *, N> inputs, Queue<Receipt<T, N>> &output,
          Tick period = 1) {
    auto &m = n.module<Sink<T, N>>(inputs, output, period);
    std::vector<QueueBase *> pops(inputs.begin(), inputs.end());
    m.rid = n.rule(m, pops, {&output});
    m.timer = n.rule(m);
    n.output = &output;
}
template <class T> struct Config : Module {
    RuleId rid{};
    Queue<T> &commands;
    Queue<T> &reg;
    Config(Queue<T> &c, Queue<T> &r) : commands(c), reg(r) {}
    void Work() { workUpdate(); }
    void workUpdate() {
        if (!e->beginRule(rid))
            return;
        auto p = read(commands, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        commands.proposePop(rid);
        reg.proposeRevise(rid, *p);
        e->completeRule(rid);
    }
};
template <class T> void config(Netlist &n, std::vector<Timed<T>> schedule, Queue<T> &reg) {
    auto &commands = n.queue<T>();
    source(n, std::move(schedule), commands);
    auto &m = n.module<Config<T>>(commands, reg);
    m.rid = n.rule(m, {&commands}, {}, {&reg});
}
template <class T> struct Merge : Module {
    RuleId rid{};
    std::vector<Queue<T> *> inputs;
    Queue<T> &output;
    Merge(std::vector<Queue<T> *> in, Queue<T> &out) : inputs(std::move(in)), output(out) {}
    void Work() { workMerge(); }
    void workMerge() {
        if (!e->beginRule(rid))
            return;
        for (auto *q : inputs) {
            auto p = read(*q, rid);
            if (p) {
                output.proposePush(rid, *p);
                q->proposePop(rid);
                break;
            }
        }
        e->completeRule(rid);
    }
};
template <class T> void merge(Netlist &n, std::vector<Queue<T> *> inputs, Queue<T> &output) {
    auto &m = n.module<Merge<T>>(inputs, output);
    m.rid = n.rule(m, {inputs.begin(), inputs.end()}, {&output});
}
inline std::vector<Word> readWords(std::istream &in) {
    std::size_t count;
    in >> count;
    std::vector<Word> result(count);
    for (auto &x : result)
        in >> x;
    return result;
}
} // namespace circuits
