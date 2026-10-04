#pragma once
#include "../common.hpp"
namespace circuits {
struct Retry : Module {
    RuleId rid{};
    Queue<Value> &token;
    Queue<Word> &budget;
    Queue<Value> &output;
    Retry(Queue<Value> &t, Queue<Word> &b, Queue<Value> &o) : token(t), budget(b), output(o) {}
    void Work() { workRetry(); }
    void workRetry() {
        if (!e->beginRule(rid))
            return;
        auto v = read(token, rid);
        if (!v) {
            e->abortRule(rid);
            return;
        }
        auto remaining = read(budget, rid);
        if (!remaining) {
            e->abortRule(rid);
            return;
        }
        token.proposeRevise(rid, *v);
        token.proposePop(rid);
        if (*remaining) {
            budget.proposeRevise(rid, *remaining - 1);
            token.proposePush(rid, *v);
        } else
            output.proposePush(rid, *v);
        e->completeRule(rid);
    }
};
inline std::unique_ptr<Netlist> retry(Word attempts = 6, bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    auto &token = b.queue<Value>(1, {{8, 42}});
    auto &budget = b.queue<Word>(1, {attempts});
    auto &ready = b.queue<Value>();
    auto &m = b.module<Retry>(token, budget, ready);
    m.rid = b.rule(m, {&token}, {&token, &ready}, {&token, &budget});
    auto &out = b.queue<Receipt<Value, 1>>();
    sink(b, std::array{&ready}, out);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
