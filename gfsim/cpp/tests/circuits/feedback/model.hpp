#pragma once
#include "../common.hpp"
namespace circuits {
struct Feedback : Module {
    RuleId rid{};
    Queue<Value> &input;
    Queue<Value> &target;
    Queue<Value> &output;
    Feedback(Queue<Value> &i, Queue<Value> &t, Queue<Value> &o) : input(i), target(t), output(o) {}
    void Work() { workHop(); }
    void workHop() {
        if (!e->beginRule(rid))
            return;
        auto p = read(input, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        input.proposePop(rid);
        if (!p->value)
            output.proposePush(rid, *p);
        else
            target.proposePush(rid, {p->seq, p->value - 1});
        e->completeRule(rid);
    }
};
inline std::unique_ptr<Netlist> feedback(const std::vector<Value> &tokens, bool selfLoop = false,
                                         bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    std::size_t count = selfLoop ? 1 : 2;
    std::vector<Queue<Value> *> ring, exits;
    for (std::size_t i = 0; i < count; ++i)
        ring.push_back(&b.queue<Value>(1, i < tokens.size() ? std::vector<Value>{tokens[i]}
                                                            : std::vector<Value>{}));
    for (std::size_t i = 0; i < count; ++i)
        exits.push_back(&b.queue<Value>());
    for (std::size_t i = 0; i < count; ++i) {
        auto &m = b.module<Feedback>(*ring[i], *ring[(i + 1) % count], *exits[i]);
        m.rid = b.rule(m, {ring[i]}, {ring[(i + 1) % count], exits[i]});
    }
    auto &merged = b.queue<Value>();
    auto &out = b.queue<Receipt<Value, 1>>(tokens.size() + 1);
    merge(b, exits, merged);
    sink(b, std::array{&merged}, out);
    b.finish(reverse);
    return n;
}
} // namespace circuits
