"""Pairs: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class PairALU(Module):
    def __init__(self, mid, rid, inputs, outputs):
        super().__init__(mid)
        self.rid, self.inputs, self.outputs = rid, inputs, outputs

    def Work(self):
        self.work_pair()

    def work_pair(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            seq, a = self.read(self.inputs[0], rid)
            self.inputs[0].propose_pop(rid)
            self.outputs[0].propose_push(rid, (seq, a))  # Cleared if RHS is absent.
            e.request_wakeup(rid, self.mid, 3)  # Must also disappear on abort.
            seq_b, b = self.read(self.inputs[1], rid)
            self.inputs[1].propose_pop(rid)
            self.outputs[1].propose_push(rid, (seq_b, (a + b) & 0xffff))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_pair(self):
        return self.engine.arbitrate_rule(self.rid)


def pairs(left, right, period=9, reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    inputs, outputs = [b.queue() for _ in range(2)], [b.queue() for _ in range(2)]
    b.source(tuple((i, (i, value)) for i, value in enumerate(left)), inputs[0])
    b.source(tuple((5 + i * 3, (i, value)) for i, value in enumerate(right)), inputs[1])
    m = PairALU(b.mid, b.rid, inputs, outputs)
    b.modules.append(m)
    b.rule(m, m.work_pair, m.arbitrate_pair, inputs, outputs)
    output = b.queue(len(left) + 1)
    b.sink(outputs, output, period)
    return b.finish(output, (left, right), {"alu": m}, cache, reverse)
