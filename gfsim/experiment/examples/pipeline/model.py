"""Pipeline: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class Compute(Module):
    def __init__(self, mid, rid, source, output, control=None, iterations=0):
        super().__init__(mid)
        self.rid, self.source, self.output = rid, source, output
        self.control, self.iterations = control, iterations

    def Work(self):
        if self.control is not None:
            self.read(self.control)  # Work changes; Rule's numerical inputs do not.
        self.work_compute()

    def work_compute(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            seq, value = self.read(self.source, rid)
            self.source.propose_pop(rid)
            for _ in range(self.iterations):
                value = (value * 1664525 + 1013904223) & 0xffffffff
            self.output.propose_push(rid, (seq, (value + 1) & 0xffffffff))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_compute(self):
        return self.engine.arbitrate_rule(self.rid)


def pipeline(values, length=4, period=5, iterations=0, control=(), prefill=False, capacity=1,
             reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    links = [b.queue(capacity, initial=((-i - 1, 100 + i),) if prefill else ())
             for i in range(length + 1)]
    source = b.source(tuple((i // 3, (i, v)) for i, v in enumerate(values)), links[0])
    register = b.queue(initial=(0,)) if control else None
    if control:
        b.config(tuple((tick, value) for tick, value in enumerate(control)), register)
    computes = []
    for i in range(length):
        m = Compute(b.mid, b.rid, links[i], links[i + 1], register, iterations)
        b.modules.append(m)
        b.rule(m, m.work_compute, m.arbitrate_compute, (links[i],), (links[i + 1],))
        computes.append(m)
    output = b.queue(len(values) + length + 16)
    sink = b.sink((links[-1],), output, period)
    return b.finish(output, values, {"links": links, "computes": computes,
                                   "source": source, "sink": sink}, cache, reverse)
