"""Packets: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class PacketLanes(Module):
    """Module selects independent lanes; each Rule branches on packet content."""
    def __init__(self, mid, a, b, control, inputs, outputs):
        super().__init__(mid)
        self.a, self.b, self.control = a, b, control
        self.inputs, self.outputs = inputs, outputs

    def Work(self):
        mode, bias, _epoch = self.read(self.control)
        if mode != 1:
            self.work_a(bias)
        if mode != 0:
            self.work_b(bias)

    def work_a(self, bias):
        if self.engine.begin_rule(self.a, (bias,)):
            self._body(self.a, 0, bias)

    def work_b(self, bias):
        if self.engine.begin_rule(self.b, (bias,)):
            self._body(self.b, 1, bias)

    def _body(self, rid, lane, bias):
        try:
            seq, value = self.read(self.inputs[lane], rid)
            self.inputs[lane].propose_pop(rid)
            if value % 7 != 0:  # Intentional packet drop is a complete pop-only firing.
                self.outputs[lane].propose_push(rid, (seq, (value + bias) & 0xffff))
        except NeedInput:
            self.engine.abort_rule(rid)
            return
        self.engine.complete_rule(rid)

    def arbitrate_a(self):
        return self.engine.arbitrate_rule(self.a)

    def arbitrate_b(self):
        return self.engine.arbitrate_rule(self.b)


def packets(lanes, changes, period=7, reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    inputs, outputs = [b.queue() for _ in range(2)], [b.queue() for _ in range(2)]
    for lane in range(2):
        b.source(tuple((i // 2, (lane * 10000 + i, v))
                       for i, v in enumerate(lanes[lane])), inputs[lane])
    control = b.queue(initial=((2, 0, 0),))
    b.config(changes, control)
    m = PacketLanes(b.mid, b.rid, b.rid + 1, control, inputs, outputs)
    b.modules.append(m)
    b.rule(m, m.work_a, m.arbitrate_a, (inputs[0],), (outputs[0],))
    b.rule(m, m.work_b, m.arbitrate_b, (inputs[1],), (outputs[1],))
    merged = b.queue()
    merger = b.merge(outputs, merged)
    output = b.queue(sum(map(len, lanes)) + 1)
    b.sink((merged,), output, period)
    return b.finish(output, lanes, {"lanes": m, "control": control,
                                   "inputs": inputs, "outputs": outputs,
                                   "merger": merger}, cache, reverse)
