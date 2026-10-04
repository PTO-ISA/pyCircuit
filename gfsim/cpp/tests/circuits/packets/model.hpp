#pragma once
#include "../common.hpp"
namespace circuits {
struct Mode {
    Word mode{}, bias{}, epoch{};
    bool operator==(const Mode &) const = default;
};
inline void json(std::ostream &o, const Mode &m) { jsonList(o, m.mode, m.bias, m.epoch); }
struct PacketLanes : Module {
    std::array<RuleId, 2> rules{};
    Queue<Mode> &control;
    std::array<Queue<Value> *, 2> inputs, outputs;
    std::array<ParameterCache<Word>, 2> args;
    PacketLanes(Queue<Mode> &c, std::array<Queue<Value> *, 2> i, std::array<Queue<Value> *, 2> o)
        : control(c), inputs(i), outputs(o) {
        resources.push_back(&c);
    }
    void Work() {
        auto p = read(control);
        if (!p)
            return;
        if (p->mode != 1)
            workLane(0, p->bias);
        if (p->mode != 0)
            workLane(1, p->bias);
    }
    void workLane(std::size_t lane, Word bias) {
        auto r = rules[lane];
        if (!e->beginRule(r, args[lane], bias))
            return;
        auto p = read(*inputs[lane], r);
        if (!p) {
            e->abortRule(r);
            return;
        }
        inputs[lane]->proposePop(r);
        if (p->value % 7)
            outputs[lane]->proposePush(r, {p->seq, (p->value + bias) & 0xffffU});
        e->completeRule(r);
    }
};
inline std::unique_ptr<Netlist> packets(const std::array<std::vector<Word>, 2> &lanes,
                                        const std::vector<Timed<Mode>> &changes, Tick period = 7,
                                        bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    std::array<Queue<Value> *, 2> inputs{&b.queue<Value>(), &b.queue<Value>()},
        outputs{&b.queue<Value>(), &b.queue<Value>()};
    for (std::size_t lane = 0; lane < 2; ++lane) {
        std::vector<Timed<Value>> s;
        for (std::size_t i = 0; i < lanes[lane].size(); ++i)
            s.push_back({i / 2, {static_cast<std::int64_t>(lane * 10000 + i), lanes[lane][i]}});
        source(b, s, *inputs[lane]);
    }
    auto &control = b.queue<Mode>(1, {{2, 0, 0}});
    config(b, changes, control);
    auto &m = b.module<PacketLanes>(control, inputs, outputs);
    for (std::size_t i = 0; i < 2; ++i)
        m.rules[i] = b.rule(m, {inputs[i]}, {outputs[i]});
    auto &merged = b.queue<Value>();
    merge(b, std::vector<Queue<Value> *>{outputs.begin(), outputs.end()}, merged);
    auto &out = b.queue<Receipt<Value, 1>>(lanes[0].size() + lanes[1].size() + 1);
    sink(b, std::array{&merged}, out, period);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
