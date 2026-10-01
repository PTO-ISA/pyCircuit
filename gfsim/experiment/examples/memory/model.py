"""Memory: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class Dispatch(Module):
    def __init__(self, mid, rid, source, outputs):
        super().__init__(mid)
        self.rid, self.source, self.outputs = rid, source, outputs

    def Work(self):
        self.work_dispatch()

    def work_dispatch(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            request = self.read(self.source, rid)
            self.source.propose_pop(rid)
            self.outputs[request[1] % len(self.outputs)].propose_push(rid, request)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_dispatch(self):
        return self.engine.arbitrate_rule(self.rid)


class Bank(Module):
    """Bank memory: cell=((valid, write_count), data), busy=(due, seq, data).

    Only service revises table cells. Finish consumes the old busy slot while
    audit revises its metadata, exercising revise-before-pop across Rules.
    """
    def __init__(self, mid, service, finish, audit, requests, cells, busy, output,
                 banks, latency):
        super().__init__(mid)
        self.service, self.finish, self.audit = service, finish, audit
        self.requests, self.cells, self.busy, self.output = requests, cells, busy, output
        self.banks, self.latency = banks, latency

    def Work(self):
        self.engine.record_read(self.mid, self.busy.qid)
        if self.busy.empty():
            self.work_service(self.engine.tick)
        else:
            self.work_audit()
            self.work_finish(self.engine.tick)

    def work_service(self, now):
        e, rid = self.engine, self.service
        if not e.begin_rule(rid, (now,)):
            return
        try:
            seq, address, write, data = self.read(self.requests, rid)
            cell = self.cells[address // self.banks]
            (valid, count), old = self.read(cell, rid)
            self.requests.propose_pop(rid)
            if write:
                cell.propose_revise(rid, data, (1,))
                cell.propose_revise(rid, (True, count + 1), (0,))
            self.busy.propose_push(rid, (now + self.latency, seq, data if write else old))
            e.request_wakeup(rid, self.mid, self.latency)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def work_audit(self):
        if not self.engine.begin_rule(self.audit):
            return
        try:
            due = self.read(self.busy, self.audit)[0]
            self.busy.propose_revise(self.audit, due, (0,))  # No-op metadata update.
        except NeedInput:
            self.engine.abort_rule(self.audit)
            return
        self.engine.complete_rule(self.audit)

    def work_finish(self, now):
        e, rid = self.engine, self.finish
        if not e.begin_rule(rid, (now,)):
            return
        try:
            due, seq, value = self.read(self.busy, rid)
            if now < due:
                e.request_wakeup(rid, self.mid, due - now)
            else:
                self.busy.propose_pop(rid)
                self.output.propose_push(rid, (seq, value))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_service(self):
        return self.engine.arbitrate_rule(self.service)

    def arbitrate_finish(self):
        return self.engine.arbitrate_rule(self.finish)

    def arbitrate_audit(self):
        return self.engine.arbitrate_rule(self.audit)


def memory(requests, banks=2, depth=8, latency=4, period=5,
           reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    front, request_queues = b.queue(), [b.queue() for _ in range(banks)]
    b.source(tuple((i // 4, request) for i, request in enumerate(requests)), front)
    dispatch = Dispatch(b.mid, b.rid, front, request_queues)
    b.modules.append(dispatch)
    b.rule(dispatch, dispatch.work_dispatch, dispatch.arbitrate_dispatch,
           (front,), request_queues)
    cells, responses, services = [], [], []
    for bank in range(banks):
        table = [b.queue(initial=(((False, 0), 0),)) for _ in range(depth)]
        busy, response = b.queue(), b.queue()
        m = Bank(b.mid, b.rid, b.rid + 1, b.rid + 2, request_queues[bank],
                 table, busy, response, banks, latency)
        b.modules.append(m)
        b.rule(m, m.work_service, m.arbitrate_service, (request_queues[bank],),
               (busy,), table)
        b.rule(m, m.work_finish, m.arbitrate_finish, (busy,), (response,))
        b.rule(m, m.work_audit, m.arbitrate_audit, revises=(busy,))
        cells.append(table)
        responses.append(response)
        services.append(m)
    merged = b.queue()
    b.merge(responses, merged)
    output = b.queue(len(requests) + 1)
    b.sink((merged,), output, period)
    return b.finish(output, requests, {"cells": cells, "banks": services}, cache, reverse)
