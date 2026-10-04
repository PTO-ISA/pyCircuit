#pragma once
#include "../common.hpp"
namespace circuits {
struct Compute : Module {
    RuleId rid{};
    Queue<Value> &input;
    Queue<Value> &output;
    Queue<Word> *control;
    unsigned iterations;
    Compute(Queue<Value> &i, Queue<Value> &o, Queue<Word> *c, unsigned n)
        : input(i), output(o), control(c), iterations(n) {
        if (c)
            resources.push_back(c);
    }
    void Work() {
        if (control && !read(*control))
            return;
        workCompute();
    }
    void workCompute() {
        if (!e->beginRule(rid))
            return;
        auto p = read(input, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        Value v = *p;
        input.proposePop(rid);
        for (unsigned i = 0; i < iterations; ++i)
            v.value = v.value * 1664525U + 1013904223U;
        ++v.value;
        output.proposePush(rid, v);
        e->completeRule(rid);
    }
};
inline std::unique_ptr<Netlist> pipeline(const std::vector<Word> &values, int length = 4,
                                         Tick period = 5, unsigned iterations = 0,
                                         const std::vector<Word> &control = {},
                                         bool prefill = false, std::size_t capacity = 1,
                                         bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    std::vector<Queue<Value> *> links;
    for (int i = 0; i <= length; ++i)
        links.push_back(&b.queue<Value>(
            capacity, prefill ? std::vector<Value>{{-i - 1, static_cast<Word>(100 + i)}}
                              : std::vector<Value>{}));
    std::vector<Timed<Value>> schedule;
    for (std::size_t i = 0; i < values.size(); ++i)
        schedule.push_back({i / 3, {static_cast<std::int64_t>(i), values[i]}});
    source(b, schedule, *links[0]);
    Queue<Word> *reg = nullptr;
    if (!control.empty()) {
        reg = &b.queue<Word>(1, {0});
        std::vector<Timed<Word>> changes;
        for (std::size_t i = 0; i < control.size(); ++i)
            changes.push_back({i, control[i]});
        config(b, changes, *reg);
    }
    for (int i = 0; i < length; ++i) {
        auto &m = b.module<Compute>(*links[i], *links[i + 1], reg, iterations);
        m.rid = b.rule(m, {links[i]}, {links[i + 1]});
    }
    auto &out = b.queue<Receipt<Value, 1>>(values.size() + length + 16);
    sink(b, std::array{links.back()}, out, period);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
