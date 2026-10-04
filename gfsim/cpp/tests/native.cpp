#include "check.hpp"
#include "circuits/common.hpp"
#include <gfsim/signal.hpp>
#include <limits>
using namespace circuits;
namespace gfsim {
struct TestAccess {
    template <class T> static auto slots(const Queue<T> &q) { return q.slots_.data(); }
    static auto sourceSlots(const QueueBase &q) { return q.sourceSlots_; }
    static auto slotIndex(const QueueBase &q, RuleId rule) { return q.slotIndex(rule); }
    static void tick(Simulator &s, Tick t) { s.tick_ = t; }
    static void version(QueueBase &q, Tick t) { q.version_ = t; }
    static void signalEvaluations(SignalBase &s, Tick t) { s.evaluations_ = t; }
    static void counter(Simulator &s, Tick t) { s.stats_.moduleWork = t; }
    static auto tasks(const Simulator &s) {
        return std::tuple{s.moduleTasks_.ids.capacity(), s.ruleTasks_.ids.capacity(),
                          s.nextRules_.ids.capacity(), s.signalTasks_.ids.capacity()};
    }
};
} // namespace gfsim
void testSourceSlots() {
    struct Model {
        Queue<Word> a{1, {0}}, c{1, {0}}, unused;
        Queue<std::array<Word, 2>> b{1, {{0, 0}}};
        Simulator sim;
        RuleId first{}, last{};
        void Work() {
            if (sim.beginRule(first)) {
                b.proposeReviseWith(first, [](auto &value) { value[0] = 10; });
                sim.completeRule(first);
            }
            if (sim.beginRule(last)) {
                a.proposeRevise(last, Word{1});
                b.proposeReviseWith(last, [](auto &value) { value[1] = 2; });
                c.proposeRevise(last, Word{3});
                sim.completeRule(last);
            }
        }
    } m;
    auto mid = m.sim.addModule<&Model::Work>(m);
    // Leave gaps in the global RuleIds and register sources out of order.
    m.sim.addRule(mid);
    m.first = m.sim.addRule(mid);
    auto missing = m.sim.addRule(mid);
    m.last = m.sim.addRule(mid);
    for (auto *q : std::array<QueueBase *, 4>{&m.a, &m.b, &m.c, &m.unused})
        m.sim.addQueue(*q);
    m.sim.bind(m.last, m.a, Revise);
    m.sim.bind(m.last, m.b, Revise);
    m.sim.bind(m.first, m.b, Revise);
    m.sim.bind(m.last, m.c, Revise);
    m.sim.bind(m.last, m.b, Revise); // Repeated declarations share one slot.
    m.sim.freeze();
    CHECK(TestAccess::slotIndex(m.a, m.last) == 1);
    CHECK(TestAccess::slotIndex(m.b, m.last) == 2);
    CHECK(TestAccess::sourceSlots(m.a) == TestAccess::sourceSlots(m.c));
    CHECK(TestAccess::sourceSlots(m.a)->indices.size() == 1);
    CHECK(TestAccess::slotIndex(m.unused, 0) == 0);
    for (auto invalid : {RuleId{1}, missing, m.last + 1, std::numeric_limits<RuleId>::max()})
        throws<std::logic_error>([&] { TestAccess::slotIndex(m.b, invalid); });
    throws<std::logic_error>([&] { TestAccess::slotIndex(m.unused, m.last); });
    for (int tick = 0; tick < 3; ++tick) {
        m.sim.step();
        CHECK(m.a.peek() == 1 && m.b.peek() == std::array<Word, 2>{10, 2} && m.c.peek() == 3);
        CHECK(m.sim.rule(m.last).participants.empty());
    }
}
struct NestedMeta {
    bool valid{};
    std::array<std::int16_t, 3> lanes{};
    bool operator==(const NestedMeta &) const = default;
};
struct Nested {
    NestedMeta meta{};
    std::uint64_t count{};
    bool operator==(const Nested &) const = default;
};
void json(std::ostream &o, const NestedMeta &m) {
    jsonList(o, m.valid, m.lanes);
}
void json(std::ostream &o, const Nested &n) {
    jsonList(o, n.meta, n.count);
}
// One Module invokes two independent Rules with a required control read between.
struct PartialModule : Module {
    RuleId first{}, second{};
    Queue<Word> &input;
    Queue<Word> &gate;
    Queue<Word> &output;
    Queue<Nested> &reg;
    PartialModule(Queue<Word> &i, Queue<Word> &g, Queue<Word> &o, Queue<Nested> &r)
        : input(i), gate(g), output(o), reg(r) {
        resources.push_back(&g);
    }
    void Work() {
        workFirst();
        auto p = read(gate);
        if (!p)
            return;
        workSecond(Nested{{true, {1, -2, 3}}, *p});
    }
    void workFirst() {
        if (!e->beginRule(first))
            return;
        auto p = read(input, first);
        if (!p) {
            e->abortRule(first);
            return;
        }
        input.proposePop(first);
        output.proposePush(first, *p + 10);
        e->completeRule(first);
    }
    void workSecond(Nested value) {
        if (!e->beginRule(second))
            return;
        auto p = read(reg, second);
        if (!p) {
            e->abortRule(second);
            return;
        }
        reg.proposeRevise<&Nested::meta, &NestedMeta::valid>(second, value.meta.valid);
        reg.proposeRevise<&Nested::meta, &NestedMeta::lanes>(second, value.meta.lanes);
        reg.proposeRevise<&Nested::count>(second, value.count);
        e->completeRule(second);
    }
};
void testComponents() {
    Netlist n;
    auto &input = n.queue<Word>();
    auto &gate = n.queue<Word>();
    auto &output = n.queue<Word>();
    auto &reg = n.queue<Nested>(1, {{}});
    source(n, std::vector<Timed<Word>>{{0, 2}, {2, 4}}, input);
    source(n, std::vector<Timed<Word>>{{4, 9}}, gate);
    auto &m = n.module<PartialModule>(input, gate, output, reg);
    m.first = n.rule(m, {&input}, {&output});
    m.second = n.rule(m, {}, {}, {&reg});
    auto &sinkOutput = n.queue<Receipt<Word, 1>>(4);
    sink(n, std::array{&output}, sinkOutput);
    n.finish();
    auto tasks = TestAccess::tasks(*n.sim);
    const auto *inputSlots = TestAccess::slots(input);
    const auto *registerSlots = TestAccess::slots(reg);
    std::vector<std::size_t> slots;
    for (auto &q : n.queues) {
        slots.push_back(q->sourceCount());
    }
    for (int t = 0; t < 12; ++t) {
        auto accepted = n.sim->step();
        if (t == 1) {
            CHECK(std::find(accepted.begin(), accepted.end(), m.first) != accepted.end());
            CHECK(gate.empty());
        }
    }
    CHECK(sinkOutput.size() == 2);
    CHECK(sinkOutput.at(0).values[0] == 12);
    CHECK(sinkOutput.at(1).values[0] == 14);
    CHECK(reg.peek() == Nested{{true, {1, -2, 3}}, 9});
    CHECK(reg.stateVersion() == 1);
    CHECK(n.sim->events().empty());
    CHECK(n.sim->rule(m.second).calls == 2); // Last no-op revise does not self-wake.
    CHECK(tasks == TestAccess::tasks(*n.sim));
    CHECK(inputSlots == TestAccess::slots(input));
    CHECK(registerSlots == TestAccess::slots(reg));
    for (std::size_t i = 0; i < n.queues.size(); ++i) {
        CHECK(slots[i] == n.queues[i]->sourceCount());
    }
    throws<std::logic_error>([&] { n.sim->bind(m.first, input, Pop); });
}
struct Boundary : Module {
    RuleId rid{};
    Queue<Word> &q;
    int mode;
    Boundary(Queue<Word> &q, int m) : q(q), mode(m) {}
    void Work() {
        if (!e->beginRule(rid))
            return;
        if (mode == 0)
            throw std::runtime_error("Work failure");
        if (mode == 1) {
            read(q, rid);
            q.proposeRevise(rid, Word{2});
        }
        if (mode == 2) {
            e->requestWakeup(rid, mid, 1);
            e->requestWakeup(rid, mid, std::numeric_limits<Tick>::max());
        }
        if (mode == 3) {
            q.proposePop(rid);
            q.proposePop(rid);
        }
        if (mode == 4)
            return;
        if (mode == 5)
            e->requestWakeup(rid, mid, 0);
        e->completeRule(rid);
    }
};
void testBoundaries() {
    const Tick max = std::numeric_limits<Tick>::max();
    for (int mode : {0, 1, 2, 3, 4, 5, 6, 8}) {
        Netlist n;
        auto &q = n.queue<Word>(1, {1});
        auto &m = n.module<Boundary>(q, mode);
        m.rid = n.rule(m, {&q}, {}, {&q});
        n.finish();
        if (mode == 1)
            TestAccess::version(q, max);
        if (mode == 2)
            n.sim->step(); // tick 0 + max is legal; tick 1 + max must fail.
        if (mode == 6)
            TestAccess::tick(*n.sim, max);
        if (mode == 8)
            TestAccess::counter(*n.sim, max);
        throws<std::exception>([&] { n.sim->step(); });
        CHECK(n.sim->failed());
        throws<std::logic_error>([&] { n.sim->step(); });
        throws<std::logic_error>([&] { n.sim->completeRule(m.rid); });
    }
    // Empty, effect-free completed Rule never fires and generates no event.
    Netlist n;
    auto &q = n.queue<Word>(1, {1});
    auto &m = n.module<Boundary>(q, 8);
    m.rid = n.rule(m);
    n.finish();
    CHECK(n.sim->step().empty());
    CHECK(!n.sim->rule(m.rid).acceptedTick);
    CHECK(n.sim->step().empty());
    CHECK(n.sim->stats().moduleWork == 1);
    CHECK(checkedAdd(max - 1, 1) == max);
    throws<std::overflow_error>([&] { checkedAdd(max, 1); });
}
struct ThrowValue {
    int value{};
    inline static bool fail = false;
    ThrowValue() = default;
    explicit ThrowValue(int v) : value(v) {};
    ThrowValue(const ThrowValue &) = default;
    ThrowValue &operator=(const ThrowValue &v) {
        if (fail)
            throw std::runtime_error("Xfer assignment failure");
        value = v.value;
        return *this;
    }
    bool operator==(const ThrowValue &) const = default;
};
void json(std::ostream &o, const ThrowValue &v) {
    o << v.value;
}
struct ThrowXfer : Module {
    RuleId rid{};
    Queue<ThrowValue> &q;
    explicit ThrowXfer(Queue<ThrowValue> &queue) : q(queue) {}
    void Work() {
        if (!e->beginRule(rid))
            return;
        read(q, rid);
        q.proposeRevise(rid, ThrowValue{2});
        ThrowValue::fail = true;
        e->completeRule(rid);
    }
};
void testXferException() {
    Netlist n;
    auto &q = n.queue<ThrowValue>(1, {ThrowValue{1}});
    auto &m = n.module<ThrowXfer>(q);
    m.rid = n.rule(m, {}, {}, {&q});
    n.finish();
    throws<std::runtime_error>([&] { n.sim->step(); });
    ThrowValue::fail = false;
    CHECK(n.sim->failed());
    throws<std::logic_error>([&] { n.sim->step(); });
}
struct SignalLifetime {
    Queue<Word> q{1, {3}, true};
    Signal<Word> value{this, [](void *p) { return static_cast<SignalLifetime *>(p)->q.peek(); }};
};
void testLifetimes() {
    SignalLifetime resources;
    {
        Simulator sim;
        sim.addQueue(resources.q);
        sim.addSignal(resources.value);
        sim.declareInput(resources.value, resources.q);
        sim.freeze();
        sim.step();
        CHECK(resources.value.value() == 3);
    }
    // Resource observation after runtime destruction has no dangling callback.
    CHECK(resources.q.peek() == 3 && resources.value.value() == 3);
    SignalLifetime overflow;
    Simulator sim;
    sim.addQueue(overflow.q);
    sim.addSignal(overflow.value);
    sim.declareInput(overflow.value, overflow.q);
    sim.freeze();
    TestAccess::signalEvaluations(overflow.value, std::numeric_limits<Tick>::max());
    throws<std::overflow_error>([&] { sim.step(); });
    CHECK(sim.failed());
}
void testLongPipeline();
void testSemantics();
int main() {
    try {
        testComponents();
        testSourceSlots();
        testBoundaries();
        testXferException();
        testLifetimes();
        testLongPipeline();
        testSemantics();
        std::cout << "native component, boundary, and 1100-stage tests passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
