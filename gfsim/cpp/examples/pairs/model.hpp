#pragma once
#include "../common.hpp"
namespace circuits {
struct PairALU : Module {
    RuleId rid{};
    std::array<Queue<Value> *, 2> inputs, outputs;
    PairALU(std::array<Queue<Value> *, 2> i, std::array<Queue<Value> *, 2> o)
        : inputs(i), outputs(o) {}
    void Work() { workPair(); }
    void workPair() {
        if (!e->beginRule(rid))
            return;
        auto a = read(*inputs[0], rid);
        if (!a) {
            e->abortRule(rid);
            return;
        }
        inputs[0]->proposePop(rid);
        outputs[0]->proposePush(rid, *a);
        e->requestWakeup(rid, mid, 3);
        auto b = read(*inputs[1], rid);
        if (!b) {
            e->abortRule(rid);
            return;
        }
        inputs[1]->proposePop(rid);
        outputs[1]->proposePush(rid, {b->seq, (a->value + b->value) & 0xffffU});
        e->completeRule(rid);
    }
};
inline std::unique_ptr<Netlist> pairs(const std::vector<Word> &left, const std::vector<Word> &right,
                                      Tick period = 9, bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    std::array<Queue<Value> *, 2> inputs{&b.queue<Value>(), &b.queue<Value>()},
        outputs{&b.queue<Value>(), &b.queue<Value>()};
    std::vector<Timed<Value>> a, c;
    for (std::size_t i = 0; i < left.size(); ++i)
        a.push_back({i, {static_cast<std::int64_t>(i), left[i]}});
    for (std::size_t i = 0; i < right.size(); ++i)
        c.push_back({5 + i * 3, {static_cast<std::int64_t>(i), right[i]}});
    source(b, a, *inputs[0]);
    source(b, c, *inputs[1]);
    auto &m = b.module<PairALU>(inputs, outputs);
    m.rid = b.rule(m, {inputs.begin(), inputs.end()}, {outputs.begin(), outputs.end()});
    auto &out = b.queue<Receipt<Value, 2>>(left.size() + 1);
    sink(b, outputs, out, period);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
