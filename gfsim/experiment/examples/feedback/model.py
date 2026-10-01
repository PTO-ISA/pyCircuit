"""Feedback: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class Feedback(Module):
    def __init__(self, mid, rid, source, target, output):
        super().__init__(mid)
        self.rid, self.source, self.target, self.output = rid, source, target, output

    def Work(self):
        self.work_hop()

    def work_hop(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            seq, hops = self.read(self.source, rid)
            self.source.propose_pop(rid)
            if hops == 0:
                self.output.propose_push(rid, (seq, hops))
            else:
                self.target.propose_push(rid, (seq, hops - 1))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_hop(self):
        return self.engine.arbitrate_rule(self.rid)


def feedback(tokens=((0, 5),), self_loop=False, reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    # Initial state seeds the feedback network; no second producer on a ring Queue.
    size = 1 if self_loop else 2
    ring = [b.queue(initial=(tokens[i],) if i < len(tokens) else ()) for i in range(size)]
    exits = [b.queue() for _ in range(size)]
    for i in range(size):
        m = Feedback(b.mid, b.rid, ring[i], ring[(i + 1) % size], exits[i])
        b.modules.append(m)
        b.rule(m, m.work_hop, m.arbitrate_hop, (ring[i],), (ring[(i + 1) % size], exits[i]))
    merged, output = b.queue(), b.queue(len(tokens) + 1)
    b.merge(exits, merged)
    b.sink((merged,), output)
    return b.finish(output, tokens, {"ring": ring}, cache, reverse)
