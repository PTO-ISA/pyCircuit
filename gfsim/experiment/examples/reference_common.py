"""Independent behavior of shared example components and transaction helpers."""

from engine import NeedInput
from examples.common import Source, Sink, Config, Merge


class Transactions:
    def __init__(self, engine, module):
        self.engine, self.module = engine, module

    def read(self, queue):
        self.engine.record_read(self.module.mid, queue.qid)
        return queue.peek()

    def take(self, queue, rid):
        value = self.read(queue)
        queue.propose_pop(rid)
        return value

    def transaction(self, rid, body):
        self.engine.begin_rule(rid)
        try:
            body(rid)
        except NeedInput:
            self.engine.abort_rule(rid)
        else:
            self.engine.complete_rule(rid)


def evaluate_common(e, m):
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Source):
        def send(rid):
            due, value = read(m.rom)
            if due > e.tick:
                e.request_wakeup(rid, m.mid, due - e.tick)
            else:
                take(m.rom, rid)
                m.output.propose_push(rid, value)
        transaction(m.rid, send)
    elif isinstance(m, Sink):
        if e.tick % m.period:
            transaction(m.timer, lambda rid: e.request_wakeup(rid, m.mid, (-e.tick) % m.period))
        else:
            transaction(m.rid, lambda rid: m.output.propose_push(
                rid, (e.tick, tuple(take(q, rid) for q in m.inputs))))
    elif isinstance(m, Config):
        transaction(m.rid, lambda rid: m.register.propose_revise(rid, take(m.commands, rid)))
    elif isinstance(m, Merge):
        def merge(rid):
            for queue in m.inputs:
                e.record_read(m.mid, queue.qid)
                if queue.current:
                    m.output.propose_push(rid, take(queue, rid))
                    break
        transaction(m.rid, merge)
    else:
        return False
    return True
