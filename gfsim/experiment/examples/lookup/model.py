"""Lookup: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class Lookup(Module):
    """Live-configured coefficient lookup, with a pending output under backpressure."""
    def __init__(self, mid, rid, source, output, selector, table):
        super().__init__(mid)
        self.rid, self.source, self.output = rid, source, output
        self.selector, self.table = selector, table

    def Work(self):
        index = self.read(self.selector)
        self.work_lookup(index)

    def work_lookup(self, index):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid, (index,)):
            return
        try:
            seq, value = self.read(self.source, rid)
            coefficient = self.read(self.table[index], rid)
            self.source.propose_pop(rid)
            self.output.propose_push(rid, (seq, value * coefficient & 0xffff))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_lookup(self):
        return self.engine.arbitrate_rule(self.rid)


def lookup(values, indices, updates, period=11, depth=64,
           reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    source, target, selector = b.queue(), b.queue(), b.queue(initial=(0,))
    b.source(tuple((i, (i, v)) for i, v in enumerate(values)), source)
    b.config(indices, selector)
    table = [b.queue(initial=(i + 1,)) for i in range(depth)]
    # Separate configuration port per table element, each with one revise Rule.
    for index, schedule in updates:
        b.config(schedule, table[index])
    m = Lookup(b.mid, b.rid, source, target, selector, table)
    b.modules.append(m)
    b.rule(m, m.work_lookup, m.arbitrate_lookup, (source,), (target,))
    output = b.queue(len(values) + 1)
    b.sink((target,), output, period)
    return b.finish(output, values, {"lookup": m, "table": table}, cache, reverse)
