"""Shared example components and explicit netlist construction helpers."""

from dataclasses import dataclass

from construction import assemble
from engine import NeedInput, Queue, RuleEntry
from reference import Queue as ReferenceQueue, Reference


class Module:
    def __init__(self, mid):
        self.mid = mid
        self.engine = None

    def read(self, queue, rid=None):
        self.engine.record_read(self.mid, queue.qid, rid)
        return queue.peek()


class Source(Module):
    """ROM of (earliest_tick, value) requests; waiting is a pure event Rule."""
    def __init__(self, mid, rid, rom, output):
        super().__init__(mid)
        self.rid, self.rom, self.output = rid, rom, output

    def Work(self):
        self.work_send(self.engine.tick)  # Explicit time argument, not hidden Rule input.

    def work_send(self, now):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid, (now,)):
            return
        try:
            due, value = self.read(self.rom, rid)
            if now < due:
                e.request_wakeup(rid, self.mid, due - now)
            else:
                self.rom.propose_pop(rid)
                self.output.propose_push(rid, value)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_send(self):
        return self.engine.arbitrate_rule(self.rid)


class Sink(Module):
    """Periodically ready receiver; results go to an observable output FIFO."""
    def __init__(self, mid, rid, timer, inputs, output, period=1):
        super().__init__(mid)
        self.rid, self.timer = rid, timer
        self.inputs, self.output, self.period = inputs, output, period

    def Work(self):
        now = self.engine.tick
        if now % self.period:
            self.work_timer(self.period - now % self.period)
        else:
            self.work_receive(now)

    def work_timer(self, delay):
        if self.engine.begin_rule(self.timer, (delay,)):
            self.engine.request_wakeup(self.timer, self.mid, delay)
            self.engine.complete_rule(self.timer)

    def work_receive(self, now):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid, (now,)):
            return
        try:
            values = []  # Generated fixed aggregate, one field per input port.
            for queue in self.inputs:
                values.append(self.read(queue, rid))
                queue.propose_pop(rid)
            self.output.propose_push(rid, (now, tuple(values)))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_receive(self):
        return self.engine.arbitrate_rule(self.rid)

    def arbitrate_timer(self):
        return self.engine.arbitrate_rule(self.timer)


class Config(Module):
    def __init__(self, mid, rid, commands, register):
        super().__init__(mid)
        self.rid, self.commands, self.register = rid, commands, register

    def Work(self):
        self.work_update()

    def work_update(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        try:
            value = self.read(self.commands, rid)
            self.commands.propose_pop(rid)
            self.register.propose_revise(rid, value)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_update(self):
        return self.engine.arbitrate_rule(self.rid)


class Merge(Module):
    """User-written input priority in a single consumer/producer Rule."""
    def __init__(self, mid, rid, inputs, output):
        super().__init__(mid)
        self.rid, self.inputs, self.output = rid, inputs, output

    def Work(self):
        self.work_merge()

    def work_merge(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return
        for queue in self.inputs:
            e.record_read(self.mid, queue.qid, rid)
            if not queue.empty():
                self.output.propose_push(rid, queue.peek())
                queue.propose_pop(rid)
                break
        e.complete_rule(rid)

    def arbitrate_merge(self):
        return self.engine.arbitrate_rule(self.rid)


@dataclass
class Circuit:
    sim: object
    output: object
    inputs: tuple
    named: dict  # Testbench handles only, never inspected by scheduling.


class Netlist:
    def __init__(self, reference=False, evaluate=None):
        self.reference, self.evaluate = reference, evaluate
        self.queues, self.modules, self.rules = [], [], [None]

    @property
    def mid(self):
        return len(self.modules)

    @property
    def rid(self):
        return len(self.rules)

    def queue(self, capacity=1, initial=()):
        queue = (ReferenceQueue if self.reference else Queue)(capacity, initial)
        queue.qid = len(self.queues)
        self.queues.append(queue)
        return queue

    def rule(self, module, work, arbitrate, pops=(), pushes=(), revises=()):
        self.rules.append(RuleEntry(module.mid, work, arbitrate,
                                    tuple(q.qid for q in pops),
                                    tuple(q.qid for q in pushes),
                                    tuple(q.qid for q in revises)))

    def source(self, schedule, output):
        rom = self.queue(max(1, len(schedule)), schedule)
        m = Source(self.mid, self.rid, rom, output)
        self.modules.append(m)
        self.rule(m, m.work_send, m.arbitrate_send, (rom,), (output,))
        return m

    def sink(self, inputs, output, period=1):
        m = Sink(self.mid, self.rid, self.rid + 1, inputs, output, period)
        self.modules.append(m)
        self.rule(m, m.work_receive, m.arbitrate_receive, inputs, (output,))
        self.rule(m, m.work_timer, m.arbitrate_timer)
        return m

    def config(self, schedule, register):
        commands = self.queue()
        self.source(schedule, commands)
        m = Config(self.mid, self.rid, commands, register)
        self.modules.append(m)
        self.rule(m, m.work_update, m.arbitrate_update, (commands,), revises=(register,))
        return m

    def merge(self, inputs, output):
        m = Merge(self.mid, self.rid, inputs, output)
        self.modules.append(m)
        self.rule(m, m.work_merge, m.arbitrate_merge, inputs, (output,))
        return m

    def finish(self, output, inputs=(), named=None, cache=True, reverse=False):
        if reverse:
            # Reassign ModuleIds and table order, preserving Rule/Queue identity.
            self.modules.reverse()
            remap = {}
            for mid, m in enumerate(self.modules):
                remap[m.mid] = mid
                m.mid = mid
            self.rules = [None] + [RuleEntry(remap[r.module_id], r.work, r.arbitrate,
                                            r.pops, r.pushes, r.revises) for r in self.rules[1:]]
        sim = (Reference(self.queues, self.modules, self.rules, self.evaluate) if self.reference else
               assemble(self.queues, self.modules, self.rules, cache))
        return Circuit(sim, output, tuple(inputs), named or {})
