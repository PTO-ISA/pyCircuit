#pragma once
#include "../common.hpp"
namespace circuits {
struct Request {
    std::int64_t seq{};
    std::size_t address{};
    bool write{};
    Word data{};
    bool operator==(const Request &) const = default;
};
struct Meta {
    bool valid{};
    Word count{};
    bool operator==(const Meta &) const = default;
};
struct CellValue {
    Meta meta{};
    Word data{};
    bool operator==(const CellValue &) const = default;
};
struct Busy {
    Tick due{};
    std::int64_t seq{};
    Word data{};
    bool operator==(const Busy &) const = default;
};
inline void json(std::ostream &o, const Request &r) {
    jsonList(o, r.seq, r.address, r.write, r.data);
}
inline void json(std::ostream &o, const Meta &m) {
    jsonList(o, m.valid, m.count);
}
inline void json(std::ostream &o, const CellValue &c) {
    jsonList(o, c.meta, c.data);
}
inline void json(std::ostream &o, const Busy &b) {
    jsonList(o, b.due, b.seq, b.data);
}
struct Dispatch : Module {
    RuleId rid{};
    Queue<Request> &input;
    std::vector<Queue<Request> *> outputs;
    Dispatch(Queue<Request> &i, std::vector<Queue<Request> *> o)
        : input(i), outputs(std::move(o)) {}
    void Work() { workDispatch(); }
    void workDispatch() {
        if (!e->beginRule(rid))
            return;
        auto p = read(input, rid);
        if (!p) {
            e->abortRule(rid);
            return;
        }
        input.proposePop(rid);
        outputs[p->address % outputs.size()]->proposePush(rid, *p);
        e->completeRule(rid);
    }
};
struct Bank : Module {
    RuleId service{}, finish{}, audit{};
    Queue<Request> &requests;
    std::vector<Queue<CellValue> *> cells;
    Queue<Busy> &busy;
    Queue<Value> &output;
    std::size_t banks;
    Tick latency;
    ParameterCache<Tick> serviceArgs, finishArgs;
    Bank(Queue<Request> &r, std::vector<Queue<CellValue> *> c, Queue<Busy> &b, Queue<Value> &o,
         std::size_t count, Tick delay)
        : requests(r), cells(std::move(c)), busy(b), output(o), banks(count), latency(delay) {}
    void Work() {
        e->recordRead(mid, busy);
        if (busy.empty())
            workService(e->tick());
        else {
            workAudit();
            workFinish(e->tick());
        }
    }
    void workService(Tick now) {
        auto r = service;
        if (!e->beginRule(r, serviceArgs, now))
            return;
        auto p = read(requests, r);
        if (!p) {
            e->abortRule(r);
            return;
        }
        auto &cell = *cells.at(p->address / banks);
        auto old = read(cell, r);
        if (!old) {
            e->abortRule(r);
            return;
        }
        requests.proposePop(r);
        if (p->write) {
            cell.proposeRevise<&CellValue::data>(r, p->data);
            cell.proposeRevise<&CellValue::meta>(r, Meta{true, old->meta.count + 1});
        }
        busy.proposePush(r, {checkedAdd(now, latency), p->seq, p->write ? p->data : old->data});
        e->requestWakeup(r, mid, latency);
        e->completeRule(r);
    }
    void workAudit() {
        if (!e->beginRule(audit))
            return;
        auto p = read(busy, audit);
        if (!p) {
            e->abortRule(audit);
            return;
        }
        busy.proposeRevise<&Busy::due>(audit, p->due);
        e->completeRule(audit);
    }
    void workFinish(Tick now) {
        auto r = finish;
        if (!e->beginRule(r, finishArgs, now))
            return;
        auto p = read(busy, r);
        if (!p) {
            e->abortRule(r);
            return;
        }
        if (now < p->due)
            e->requestWakeup(r, mid, p->due - now);
        else {
            busy.proposePop(r);
            output.proposePush(r, {p->seq, p->data});
        }
        e->completeRule(r);
    }
};
inline std::unique_ptr<Netlist> memory(const std::vector<Request> &requests, std::size_t banks = 2,
                                       std::size_t depth = 8, Tick latency = 4, Tick period = 5,
                                       bool cache = true, bool reverse = false) {
    auto n = std::make_unique<Netlist>();
    auto &b = *n;
    auto &front = b.queue<Request>();
    std::vector<Queue<Request> *> requestQueues;
    for (std::size_t i = 0; i < banks; ++i)
        requestQueues.push_back(&b.queue<Request>());
    std::vector<Timed<Request>> schedule;
    for (std::size_t i = 0; i < requests.size(); ++i)
        schedule.push_back({i / 4, requests[i]});
    source(b, schedule, front);
    auto &dispatcher = b.module<Dispatch>(front, requestQueues);
    dispatcher.rid = b.rule(dispatcher, {&front}, {requestQueues.begin(), requestQueues.end()});
    std::vector<Queue<Value> *> responses;
    for (std::size_t i = 0; i < banks; ++i) {
        std::vector<Queue<CellValue> *> table;
        for (std::size_t j = 0; j < depth; ++j)
            table.push_back(&b.queue<CellValue>(1, {{{false, 0}, 0}}));
        auto &busy = b.queue<Busy>();
        auto &response = b.queue<Value>();
        auto &m = b.module<Bank>(*requestQueues[i], table, busy, response, banks, latency);
        m.service = b.rule(m, {requestQueues[i]}, {&busy}, {table.begin(), table.end()});
        m.finish = b.rule(m, {&busy}, {&response});
        m.audit = b.rule(m, {}, {}, {&busy});
        responses.push_back(&response);
    }
    auto &merged = b.queue<Value>();
    merge(b, responses, merged);
    auto &out = b.queue<Receipt<Value, 1>>(requests.size() + 1);
    sink(b, std::array{&merged}, out, period);
    b.finish(cache, reverse);
    return n;
}
} // namespace circuits
