"""Retry: component behavior and complete circuit connections."""

from engine import NeedInput
from examples.common import Module, Netlist


class Retry(Module):
    """A seeded retry buffer recirculates identical payload until budget expires."""
    def __init__(self, mid, rid, token, budget, output):
        super().__init__(mid)
        self.rid, self.token, self.budget, self.output = rid, token, budget, output

    def Work(self):
        self.work_retry()

    def work_retry(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            value, remaining = self.read(self.token, rid), self.read(self.budget, rid)
            self.token.propose_revise(rid, value)  # Whole-value update of the old tail.
            self.token.propose_pop(rid)
            if remaining:
                self.budget.propose_revise(rid, remaining - 1)
                self.token.propose_push(rid, value)  # Identical bits, new element identity.
            else:
                self.output.propose_push(rid, value)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_retry(self):
        return self.engine.arbitrate_rule(self.rid)


def retry(attempts=6, reference=False, cache=True, reverse=False):
    from .reference import evaluate

    b = Netlist(reference, evaluate)
    token, budget, ready = b.queue(initial=((8, 42),)), b.queue(initial=(attempts,)), b.queue()
    m = Retry(b.mid, b.rid, token, budget, ready)
    b.modules.append(m)
    b.rule(m, m.work_retry, m.arbitrate_retry, (token,), (token, ready), (token, budget))
    output = b.queue()
    b.sink((ready,), output)
    return b.finish(output, named={"token": token, "budget": budget}, cache=cache, reverse=reverse)
