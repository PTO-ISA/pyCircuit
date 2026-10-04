#include "check.hpp"
#include "circuits/common.hpp"
#include "circuits/feedback/model.hpp"
#include "circuits/lookup/model.hpp"
#include "circuits/memory/model.hpp"
#include "circuits/packets/model.hpp"
#include "circuits/pairs/model.hpp"
#include "circuits/pipeline/model.hpp"
#include "circuits/retry/model.hpp"
#include <gfsim/signal.hpp>
using namespace circuits;
namespace {
struct Probe : Module {
    void Work() {}
};
struct PendingProducer : Module {
    RuleId rid{};
    Queue<Word> &output;
    Probe &probe;
    PendingProducer(Queue<Word> &q, Probe &p) : output(q), probe(p) {}
    void Work() { work_send(); }
    void work_send() {
        if (!e->beginRule(rid))
            return;
        output.proposePush(rid, 9);
        e->requestWakeup(rid, probe.mid, 2);
        e->completeRule(rid);
    }
};
struct Transfer : Module {
    RuleId rid{};
    Queue<Word> &input, &output;
    Transfer(Queue<Word> &i, Queue<Word> &o) : input(i), output(o) {}
    void Work() { work_transfer(); }
    void work_transfer() {
        if (!e->beginRule(rid))
            return;
        try {
            auto value = input.peek();
            input.proposePop(rid);
            output.proposePush(rid, value);
            e->completeRule(rid);
        } catch (const NeedInput &) {
            e->abortRule(rid);
        }
    }
};
struct DelayedSink : Module {
    RuleId timer{}, rid{};
    Queue<Word> &input, &output;
    const Tick due;
    ParameterCache<Tick> timerArgs;
    DelayedSink(Queue<Word> &i, Queue<Word> &o, Tick d) : input(i), output(o), due(d) {}
    void Work() {
        if (e->tick() < due) {
            work_timer(due - e->tick());
        } else
            work_drain();
    }
    void work_timer(Tick delay) {
        if (!e->beginRule(timer, timerArgs, delay))
            return;
        e->requestWakeup(timer, mid, delay);
        e->completeRule(timer);
    }
    void work_drain() {
        if (!e->beginRule(rid))
            return;
        try {
            output.proposePush(rid, input.peek());
            input.proposePop(rid);
            e->completeRule(rid);
        } catch (const NeedInput &) {
            e->abortRule(rid);
        }
    }
};
void capacityPropagation() {
    for (bool cache : {false, true})
        for (bool reverse : {false, true}) {
            Netlist n;
            auto &a = n.queue<Word>(1, {1}), &b = n.queue<Word>(1, {2}), &out = n.queue<Word>(8);
            auto &probe = n.module<Probe>();
            auto &p = n.module<PendingProducer>(a, probe);
            p.rid = n.rule(p, {}, {&a});
            auto &t = n.module<Transfer>(a, b);
            t.rid = n.rule(t, {&a}, {&b});
            auto &sink = n.module<DelayedSink>(b, out, 3);
            sink.timer = n.rule(sink);
            sink.rid = n.rule(sink, {&b}, {&out});
            n.finish(cache, reverse);
            for (int tick = 0; tick < 4; ++tick)
                n.sim->step();
            CHECK(out.peek() == 2 && a.peek() == 9 && b.peek() == 1);
            CHECK(n.sim->rule(p.rid).calls == 1 && n.sim->rule(t.rid).calls == 1);
            CHECK(n.sim->module(p.mid).calls == 1 && n.sim->module(t.mid).calls == 1);
            CHECK(!n.sim->reads(p.mid, a)); // Pure push doesn't subscribe to capacity.
            CHECK(n.sim->rule(t.rid).readSlots.size() == 1); // Commit keeps reads.
            auto events = n.sim->events();
            CHECK(std::find(events.begin(), events.end(),
                            std::pair<Tick, ModuleId>{5, probe.mid}) != events.end());
            CHECK(n.sim->module(probe.mid).calls == 1); // Delay starts at acceptance tick 3.
            CHECK(n.sim->stats().dfsVisits ==
                  6); // Two initial failures, timer, then three accepts.
        }
}
struct ManyRules : Module {
    Queue<Word> &mode, &left, &right;
    std::array<RuleId, 70> rules{};
    std::array<ParameterCache<Word>, 70> args;
    ManyRules(Queue<Word> &m, Queue<Word> &a, Queue<Word> &b) : mode(m), left(a), right(b) {
        resources = {&m, &a, &b};
    }
    void Work() {
        const auto config = mode.peek();
        for (std::size_t i = 0; i < rules.size(); ++i)
            if (config != 99 || i % 2 == 0)
                work_check(i, config);
    }
    void work_check(std::size_t i, Word config) {
        const auto rid = rules[i];
        if (!e->beginRule(rid, args[i], config))
            return;
        if (i % 2 == 0)
            (void)left.peek();
        else
            (void)right.peek();
        e->completeRule(rid); // Intentionally effect-free, but fully cacheable.
    }
};
void bitmapAndParameters() {
    Netlist n;
    auto &mode = n.queue<Word>(1, {0}), &left = n.queue<Word>(1, {1}),
         &right = n.queue<Word>(1, {2});
    config(n, {{2, 1}, {5, 99}}, mode);
    config(n, {{0, 7}}, left);
    auto &m = n.module<ManyRules>(mode, left, right);
    for (auto &r : m.rules)
        r = n.rule(m);
    n.finish();
    n.sim->step(); // Config source produces a command; all 70 rules read once.
    const auto *words = n.sim->module(m.mid).ruleReaders.data();
    n.sim->step(); // Left changes. Only even Rules become dirty, including bits 64+.
    for (std::size_t i = 0; i < m.rules.size(); ++i)
        CHECK(n.sim->dirty(m.rules[i]) == (i % 2 == 0));
    n.sim->step();
    for (std::size_t i = 0; i < m.rules.size(); ++i)
        CHECK(n.sim->rule(m.rules[i]).calls == (i % 2 == 0 ? 2 : 1));
    CHECK(n.sim->stats().cacheHits == 35);
    for (int i = 0; i < 5; ++i)
        n.sim->step();
    for (std::size_t i = 0; i < m.rules.size(); ++i) {
        const auto &r = n.sim->rule(m.rules[i]);
        CHECK(!r.acceptedTick); // No-effect completion does not fire.
        CHECK(r.calls == (i % 2 == 0 ? 4 : 2));
        CHECK(r.complete == (i % 2 == 0));
        CHECK(r.readSlots.size() == (i % 2 == 0 ? 1 : 0)); // Deselection removes reads.
        CHECK(!n.sim->dirty(m.rules[i]));
    }
    CHECK(n.sim->module(m.mid).ruleReaders.data() == words);
    CHECK(!n.sim->reads(m.mid, right));
}
struct SignalCircuit {
    Queue<Word> selector{1, {0}, true}, a{1, {7}, true}, b{1, {7}, true};
    Queue<Word> output{8};
    Signal<Word> mux{this, [](void *p) {
                         auto &c = *static_cast<SignalCircuit *>(p);
                         return c.selector.peek() ? c.b.peek() : c.a.peek();
                     }};
    Simulator sim;
    ModuleId update{}, reader{};
    RuleId write{}, consume{};
    ParameterCache<Tick> args;
    SignalCircuit() {
        update = sim.addModule<&SignalCircuit::updateWork>(*this);
        reader = sim.addModule<&SignalCircuit::readWork>(*this);
        write = sim.addRule(update);
        consume = sim.addRule(reader);
        for (QueueBase *q : std::initializer_list<QueueBase *>{&selector, &a, &b, &output})
            sim.addQueue(*q);
        sim.addSignal(mux);
        for (auto *q : {&selector, &a, &b}) {
            sim.declareResource(update, *q);
            sim.declareInput(mux, *q);
            sim.bind(write, *q, Revise);
        }
        sim.declareInput(consume, mux);
        sim.declareResource(reader, output);
        sim.bind(consume, output, Push);
        sim.freeze();
    }
    void updateWork() { work_update(sim.tick()); }
    void work_update(Tick now) {
        if (!sim.beginRule(write, args, now))
            return;
        if (now == 0)
            selector.proposeRevise(write, 1U); // Same output: no downstream notification.
        if (now == 1)
            a.proposeRevise(write, 9U); // Inactive input still triggers evaluation.
        if (now == 2)
            b.proposeRevise(write, 11U);
        if (now < 2)
            sim.requestWakeup(write, update, 1);
        sim.completeRule(write);
    }
    void readWork() {
        if (!sim.beginRule(consume))
            return;
        output.proposePush(consume, mux.value());
        sim.completeRule(consume);
    }
};
void signalSwitch() {
    SignalCircuit c;
    throws<std::logic_error>([&] { (void)c.mux.value(); });
    CHECK(!c.sim.reads(c.reader, c.output));
    c.sim.step();
    CHECK(c.mux.value() == 7 && c.mux.evaluations() == 2 && c.output.size() == 1);
    CHECK(!c.sim.dirty(c.consume) && c.sim.events().size() == 2);
    c.sim.step();
    CHECK(c.mux.evaluations() == 3 && c.sim.module(c.reader).calls == 1);
    CHECK(!c.sim.dirty(c.consume));
    c.sim.step();
    CHECK(c.mux.value() == 11 && c.mux.evaluations() == 4 && c.sim.dirty(c.consume));
    c.sim.step();
    CHECK(c.output.size() == 2 && c.output.at(1) == 11);
    CHECK(c.sim.module(c.reader).calls == 2);
}
struct StaticSignalReaders {
    Queue<Word> input{1, {0}, true}, blocked{1, {99}}, committed{8};
    Signal<Word> value{this, [](void *p) { return static_cast<StaticSignalReaders *>(p)->input.peek(); }};
    Simulator sim;
    ModuleId writer{}, reader{}, control{};
    RuleId write{}, plain{};
    std::vector<RuleId> rules;
    explicit StaticSignalReaders(bool cache) : sim(cache) {
        writer = sim.addModule<&StaticSignalReaders::writeWork>(*this);
        reader = sim.addModule<&StaticSignalReaders::readWork>(*this);
        control = sim.addModule<&StaticSignalReaders::controlWork>(*this);
        write = sim.addRule(writer);
        plain = sim.addRule(control);
        for (int i = 0; i < 130; ++i)
            rules.push_back(sim.addRule(reader));
        for (auto *q : {&input, &blocked, &committed})
            sim.addQueue(*q);
        sim.addSignal(value);
        sim.declareResource(writer, input);
        sim.bind(write, input, Revise);
        sim.declareInput(value, input);
        sim.declareInput(value, input); // Deduplicate all three kinds of connection.
        for (auto i : {0, 63, 64, 129}) {
            sim.declareInput(rules[i], value); // Also declares Module activation.
            sim.declareInput(rules[i], value);
        }
        sim.declareResource(control, value);
        sim.declareResource(control, value);
        sim.declareResource(reader, blocked);
        sim.declareResource(reader, committed);
        sim.bind(rules[63], blocked, Push);
        sim.bind(rules[64], committed, Push);
        sim.freeze();
    }
    void writeWork() {
        if (sim.beginRule(write)) {
            input.proposeRevise(write, input.peek() + 1);
            sim.completeRule(write);
        }
    }
    void readWork() {
        for (auto i : {0, 1, 63, 64, 129}) {
            if (sim.tick() == 1 && i != 1)
                continue; // Cancel every type of candidate; declarations survive.
            if (!sim.beginRule(rules[i]))
                continue;
            // The first path never reads the Signal, but all declared Rules dirty.
            Word v = sim.tick() && i != 1 ? value.value() : 0;
            if (i == 63)
                blocked.proposePush(rules[i], v);
            if (i == 64)
                committed.proposePush(rules[i], v);
            if (i == 129)
                sim.abortRule(rules[i]);
            else
                sim.completeRule(rules[i]);
        }
    }
    void controlWork() {
        // No Signal read in this path; Module-only declaration still activates it.
        if (sim.beginRule(plain))
            sim.completeRule(plain);
    }
};
void staticSignalLifecycle() {
    for (bool cache : {false, true}) {
        StaticSignalReaders c(cache);
        CHECK(c.sim.reads(c.reader, c.value) && c.sim.reads(c.control, c.value));
        CHECK(c.sim.module(c.reader).resources.size() == 2);
        for (int step = 0; step < 4; ++step) {
            c.sim.step();
            CHECK(c.value.evaluations() == Tick(step + 2));
            CHECK(c.sim.module(c.reader).calls == Tick(step + 1));
            CHECK(c.sim.module(c.control).calls == Tick(step + 1));
            for (std::size_t i = 0; i < c.rules.size(); ++i) {
                CHECK(c.sim.dirty(c.rules[i]) == (i == 0 || i == 63 || i == 64 || i == 129));
                CHECK(c.sim.rule(c.rules[i]).readSlots.empty());
            }
            CHECK(!c.sim.dirty(c.plain));
            CHECK(c.blocked.peek() == 99);
            CHECK(c.sim.events().size() == 3); // One event per dependent Module.
        }
        CHECK(c.sim.rule(c.rules[0]).calls == 3);
        CHECK(c.sim.rule(c.rules[63]).calls == 3);
        CHECK(c.sim.rule(c.rules[64]).calls == 3);
        CHECK(c.sim.rule(c.rules[129]).calls == 3);
        CHECK(c.sim.rule(c.rules[1]).calls == (cache ? 1 : 4));
        CHECK(c.sim.rule(c.plain).calls == (cache ? 1 : 4));
        CHECK(c.committed.size() == 3 && c.committed.at(1) == 2 && c.committed.at(2) == 3);
    }
}
struct Partial : Module {
    RuleId rid{};
    Queue<Word> &a, &b, &out;
    Partial(Queue<Word> &a, Queue<Word> &b, Queue<Word> &o) : a(a), b(b), out(o) {}
    void Work() {
        if (!e->beginRule(rid))
            return;
        try {
            out.proposePush(rid, a.peek());
            a.proposePop(rid);
            e->requestWakeup(rid, mid, 17);
            (void)b.peek();
            b.proposePop(rid);
            e->completeRule(rid);
        } catch (const NeedInput &) {
            e->abortRule(rid);
        }
    }
};
void partialAbort() {
    Netlist n;
    auto &a = n.queue<Word>(1, {3}), &b = n.queue<Word>(), &out = n.queue<Word>(4);
    auto &m = n.module<Partial>(a, b, out);
    m.rid = n.rule(m, {&a, &b}, {&out});
    source(n, std::vector<Timed<Word>>{{2, 5}}, b);
    n.finish();
    n.sim->step();
    CHECK(a.peek() == 3 && out.empty());
    CHECK(n.sim->rule(m.rid).readSlots.size() == 2);
    CHECK(n.sim->rule(m.rid).participants.empty() && n.sim->rule(m.rid).wakeRequests.empty());
    for (auto event : n.sim->events())
        CHECK(event.first != 17);
    for (int i = 0; i < 3; ++i)
        n.sim->step();
    CHECK(a.empty() && b.empty() && out.peek() == 3);
}
void circuitMatrix() {
    for (bool cache : {true, false})
        for (bool reverse : {false, true}) {
            auto self = retry(6, cache, reverse);
            for (int i = 0; i < 12; ++i)
                self->sim->step();
            CHECK(self->output->size() == 1);
            auto ring = feedback({{0, 0}, {1, 2}}, false, cache,
                                 reverse); // Static cycle; a selected exit breaks it.
            for (int i = 0; i < 15; ++i)
                ring->sim->step();
            CHECK(ring->output->size() == 2);
            auto cycle = feedback({{0, 2}, {1, 2}}, false, cache, reverse);
            throws<CapacityCycle>([&] { cycle->sim->step(); });
            CHECK(cycle->sim->failed());
            auto mem =
                memory({{0, 0, true, 42}, {1, 0, false, 0}, {2, 3, true, 99}, {3, 3, false, 0}}, 2,
                       2, 4, 3, cache, reverse);
            for (int i = 0; i < 70; ++i)
                mem->sim->step();
            auto &out = dynamic_cast<Queue<Receipt<Value, 1>> &>(*mem->output);
            CHECK(out.size() == 4);
            std::array<Word, 4> values{};
            for (std::size_t i = 0; i < out.size(); ++i)
                values.at(out.at(i).values[0].seq) = out.at(i).values[0].value;
            CHECK(values == std::array<Word, 4>{42, 42, 99, 99});
            auto pair = pairs({1, 2}, {3, 4}, 5, cache, reverse);
            for (int i = 0; i < 40; ++i)
                pair->sim->step();
            CHECK(pair->output->size() == 2);
            auto lookupNet = lookup({2, 3, 4}, {{2, 1}}, {{1, {{3, 10}}}}, 9, 3, cache, reverse);
            for (int i = 0; i < 40; ++i)
                lookupNet->sim->step();
            auto &lookupOut = dynamic_cast<Queue<Receipt<Value, 1>> &>(*lookupNet->output);
            CHECK(lookupOut.size() == 3);
            CHECK(lookupOut.at(2).values[0] == Value{2, 40});
        }
}
struct Switching : Module {
    RuleId rid{};
    Queue<Word> &mode, &a, &b, &output;
    ParameterCache<Word> args;
    Switching(Queue<Word> &m, Queue<Word> &a, Queue<Word> &b, Queue<Word> &o)
        : mode(m), a(a), b(b), output(o) {
        resources = {&m};
    }
    void Work() {
        const auto index = mode.peek();
        if (index < 2)
            work_choose(index);
    }
    void work_choose(Word index) {
        if (!e->beginRule(rid, args, index))
            return;
        try {
            auto &input = index ? b : a;
            output.proposePush(rid, input.peek());
            input.proposePop(rid);
            e->requestWakeup(rid, mid, 17);
            e->completeRule(rid);
        } catch (const NeedInput &) {
            e->abortRule(rid);
        }
    }
};
void replacementAndDeselection() {
    for (bool deselect : {false, true})
        for (bool reverse : {false, true}) {
            Netlist n;
            auto &mode = n.queue<Word>(1, {0}), &a = n.queue<Word>(1, {10}),
                 &b = n.queue<Word>(1, {20});
            auto &out = n.queue<Word>(1, {99}), &receipt = n.queue<Word>(4);
            std::vector<Timed<Word>> changes{{0, 1}};
            if (deselect)
                changes.push_back({2, 2});
            config(n, changes, mode);
            auto &m = n.module<Switching>(mode, a, b, out);
            m.rid = n.rule(m, {&a, &b}, {&out});
            auto &sink = n.module<DelayedSink>(out, receipt, 5);
            sink.timer = n.rule(sink);
            sink.rid = n.rule(sink, {&out}, {&receipt});
            n.finish(true, reverse);
            n.sim->step();
            CHECK(n.sim->reads(m.mid, a) && !n.sim->reads(m.mid, b));
            n.sim->step();
            n.sim->step();
            CHECK(!n.sim->reads(m.mid, a) && n.sim->reads(m.mid, b));
            CHECK(a.size() == 1 && b.size() == 1); // Both attempts are pending, nothing consumed.
            for (int i = 0; i < 3; ++i)
                n.sim->step();
            CHECK(receipt.peek() == 99 && a.peek() == 10);
            CHECK(n.sim->rule(m.rid).calls == 2);
            if (deselect) {
                CHECK(out.empty() && b.peek() == 20);
                CHECK(!n.sim->reads(m.mid, b) && !n.sim->dirty(m.rid));
                CHECK(n.sim->rule(m.rid).participants.empty());
            } else
                CHECK(out.peek() == 20 && b.empty());
            for (auto [tick, mid] : n.sim->events())
                if (mid == m.mid)
                    CHECK(tick == 6 || (!deselect && tick == 22));
        }
}
struct SumBarrier {
    Queue<Word> a{1, {1}, true}, b{1, {2}, true};
    Signal<Word> sum{this, [](void *p) {
                         auto &c = *static_cast<SumBarrier *>(p);
                         return c.a.peek() + c.b.peek();
                     }};
    Simulator sim;
    RuleId ra{}, rb{};
    SumBarrier() {
        auto ma = sim.addModule<&SumBarrier::WorkA>(*this),
             mb = sim.addModule<&SumBarrier::WorkB>(*this);
        ra = sim.addRule(ma);
        rb = sim.addRule(mb);
        sim.addQueue(a);
        sim.addQueue(b);
        sim.addSignal(sum);
        sim.declareInput(sum, a);
        sim.declareInput(sum, b);
        sim.declareResource(ma, a);
        sim.declareResource(mb, b);
        sim.bind(ra, a, Revise);
        sim.bind(rb, b, Revise);
        sim.freeze();
    }
    void WorkA() {
        if (sim.beginRule(ra)) {
            a.proposeRevise(ra, 3U);
            sim.completeRule(ra);
        }
    }
    void WorkB() {
        if (sim.beginRule(rb)) {
            b.proposeRevise(rb, 4U);
            sim.completeRule(rb);
        }
    }
};
void signalBarrier() {
    SumBarrier c;
    c.sim.step();
    CHECK(c.sum.value() == 7 && c.sum.evaluations() == 2);
    c.sim.step(); // No-op revisions do not change versions or reevaluate Signal.
    CHECK(c.sum.value() == 7 && c.sum.evaluations() == 2);
    CHECK(c.a.stateVersion() == 1 && c.b.stateVersion() == 1);
}
struct Forbidden {
    Queue<Word> q{1, {0}, true}, unregistered{1, {1}, true};
    Signal<Word> s{this, [](void *p) { return static_cast<Forbidden *>(p)->helper(); }};
    Signal<Word> other{this, [](void *) { return 1U; }};
    Simulator sim;
    ModuleId mid{};
    RuleId rid{};
    const int mode;
    explicit Forbidden(int mode) : mode(mode) {
        mid = sim.addModule<&Forbidden::Work>(*this);
        rid = sim.addRule(mid);
        sim.addQueue(q);
        sim.addSignal(s);
        sim.addSignal(other);
        if (mode != 0)
            sim.declareInput(s, q);
        if (mode != 4)
            sim.declareResource(mid, q);
        if (mode != 4)
            sim.bind(rid, q, Revise);
        if (mode == 7)
            sim.declareResource(mid, s); // Does not authorize Rule reads.
        sim.freeze();
    }
    Word helper() {
        if (mode == 5)
            return unregistered.peek();
        if (mode == 1)
            return other.value();
        if (mode == 2)
            q.proposeRevise(rid, 1U);
        if (mode == 3)
            sim.requestWakeup(rid, mid, 1);
        return q.peek();
    }
    void Work() {
        if (mode == 6)
            (void)s.value();
        else if (mode == 7) {
            sim.beginRule(rid);
            (void)s.value();
            sim.completeRule(rid);
        } else
            (void)q.peek();
    }
};
void declarationAndPurity() {
    for (int mode = 0; mode < 8; ++mode) {
#ifdef NDEBUG
        if (mode == 0 || mode >= 6) // Declaration completeness is checked only in Debug.
            continue;
#endif
        Forbidden f(mode);
        throws<std::logic_error>([&] { f.sim.step(); });
        CHECK(f.sim.failed());
        throws<std::logic_error>([&] { f.sim.step(); });
    }
    Queue<Word> q(1, {1}, true);
    Simulator s;
    Probe p;
    auto m = s.addModule<&Probe::Work>(p), r = s.addRule(m);
    s.addQueue(q);
    throws<std::logic_error>([&] { s.addQueue(q); });
    throws<std::logic_error>([&] { s.bind(r, q, Pop); });
    throws<std::logic_error>([&] { s.bind(r, q, Push); });
    s.bind(r, q, Revise);
    throws<std::logic_error>([&] { s.freeze(); }); // Missing Module declaration is not inferred.
    CHECK(s.failed());
}
} // namespace
void testSemantics() {
    capacityPropagation();
    bitmapAndParameters();
    signalSwitch();
    staticSignalLifecycle();
    partialAbort();
    circuitMatrix();
    declarationAndPurity();
    replacementAndDeselection();
    signalBarrier();
}
