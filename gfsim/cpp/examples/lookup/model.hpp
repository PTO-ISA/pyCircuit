#pragma once
#include "../common.hpp"
namespace circuits {
struct Lookup : Module {
    RuleId rid{};
    Queue<Value> &input;
    Queue<Value> &output;
    Queue<Word> &selector;
    std::vector<Queue<Word> *> table;
    ParameterCache<Word> args;
    Lookup(Queue<Value> &i, Queue<Value> &o, Queue<Word> &s, std::vector<Queue<Word> *> t)
        : input(i), output(o), selector(s), table(std::move(t)) {}
    void Work() {
        auto p = read(selector);
        if (p)
            workLookup(*p);
    }
    void workLookup(Word index) {
        if (!e->beginRule(rid, args, index))
            return;
        auto p = read(input, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        auto coefficient = read(*table.at(index), rid);
        if (!coefficient) {
            e->abortRule(rid);
            return;
        }
        input.proposePop(rid);
        output.proposePush(rid, {p->seq, (p->value * *coefficient) & 0xffffU});
        e->completeRule(rid);
    }
};
inline std::unique_ptr<Netlist>
lookup(const std::vector<Word> &values, const std::vector<Timed<Word>> &indices,
       const std::vector<std::pair<std::size_t, std::vector<Timed<Word>>>> &updates,
       Tick period = 11, std::size_t depth = 64, bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    auto &input = b.queue<Value>();
    auto &target = b.queue<Value>();
    auto &selector = b.queue<Word>(1, {0});
    std::vector<Timed<Value>> schedule;
    for (std::size_t i = 0; i < values.size(); ++i)
        schedule.push_back({i, {static_cast<std::int64_t>(i), values[i]}});
    source(b, schedule, input);
    config(b, indices, selector);
    std::vector<Queue<Word> *> table;
    for (std::size_t i = 0; i < depth; ++i)
        table.push_back(&b.queue<Word>(1, {static_cast<Word>(i + 1)}));
    for (const auto &[index, changes] : updates)
        config(b, changes, *table.at(index));
    auto &m = b.module<Lookup>(input, target, selector, table);
    m.rid = b.rule(m, {&input}, {&target});
    auto &out = b.queue<Receipt<Value, 1>>(values.size() + 1);
    sink(b, std::array{&target}, out, period);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
