"""Handwritten examples of generated Module members; no decorators or discovery."""

from engine import NeedInput


class Move:
    def __init__(self, mid, rid, source, target, *, control=None, iterations=0,
                 drop_zero=False, enabled=None):
        self.mid, self.rid, self.source, self.target = mid, rid, source, target
        self.control, self.iterations = control, iterations
        self.drop_zero, self.enabled = drop_zero, enabled

    def Work(self):
        if self.enabled is not None and not self.enabled(self.engine.tick):
            return
        if self.control is not None:
            self.engine.record_read(self.mid, self.control.qid)
            if self.control.peek() < 0:
                return
        self.work_move()

    def work_move(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            e.record_read(self.mid, self.source.qid, self.rid)
            value = self.source.peek()
            self.source.propose_pop(self.rid)
            if self.drop_zero and value == 0:
                e.complete_rule(self.rid)
                return
            result = sum((value * 17 + i) % 97 for i in range(self.iterations)) if self.iterations else value
            self.target.propose_push(self.rid, result)
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)

    def arbitrate_move(self):
        return self.engine.arbitrate_rule(self.rid)


class Drain:
    def __init__(self, mid, rid, queue, enabled=None):
        self.mid, self.rid, self.queue, self.enabled = mid, rid, queue, enabled

    def Work(self):
        if self.enabled is None or self.enabled(self.engine.tick):
            self.work_drain()

    def work_drain(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            e.record_read(self.mid, self.queue.qid, self.rid)
            self.queue.peek()
            self.queue.propose_pop(self.rid)
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)

    def arbitrate_drain(self):
        return self.engine.arbitrate_rule(self.rid)


class Push:
    def __init__(self, mid, rid, queues, values, enabled=None):
        self.mid, self.rid, self.queues = mid, rid, tuple(queues)
        self.values, self.enabled = values, enabled

    def Work(self):
        if self.enabled is None or self.enabled(self.engine.tick):
            self.work_push(*self.values(self.engine.tick))

    def work_push(self, index, value):
        if not self.engine.begin_rule(self.rid, (index, value)):
            return
        self.queues[index].propose_push(self.rid, value)
        self.engine.complete_rule(self.rid)

    def arbitrate_push(self):
        return self.engine.arbitrate_rule(self.rid)


class Revise:
    def __init__(self, mid, rid, queues, values):
        self.mid, self.rid, self.queues, self.values = mid, rid, tuple(queues), values

    def Work(self):
        self.work_revise(*self.values(self.engine.tick))

    def work_revise(self, *values):
        e = self.engine
        if not e.begin_rule(self.rid, values):
            return
        try:
            for queue, value in zip(self.queues, values, strict=True):
                queue.propose_revise(self.rid, value)
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)

    def arbitrate_revise(self):
        return self.engine.arbitrate_rule(self.rid)


class ReadSwitch:
    def __init__(self, mid, mode, data):
        self.mid, self.mode, self.data = mid, mode, tuple(data)

    def Work(self):
        self.engine.record_read(self.mid, self.mode.qid)
        queue = self.data[self.mode.peek()]
        self.engine.record_read(self.mid, queue.qid)
        queue.try_peek()


class BranchMove:
    def __init__(self, mid, rule_a, rule_b, mode, inputs, outputs):
        self.mid, self.rule_a, self.rule_b, self.mode = mid, rule_a, rule_b, mode
        self.inputs, self.outputs = tuple(inputs), tuple(outputs)

    def Work(self):
        self.engine.record_read(self.mid, self.mode.qid)
        if self.mode.peek() == 0:
            self.work_a()
        else:
            self.work_b()

    def work_a(self):
        if self.engine.begin_rule(self.rule_a):
            self._move(self.rule_a, 0)

    def work_b(self):
        if self.engine.begin_rule(self.rule_b):
            self._move(self.rule_b, 1)

    def _move(self, rid, index):
        e, source, target = self.engine, self.inputs[index], self.outputs[index]
        try:
            e.record_read(self.mid, source.qid, rid)
            value = source.peek()
            source.propose_pop(rid)
            target.propose_push(rid, value)
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_a(self):
        return self.engine.arbitrate_rule(self.rule_a)

    def arbitrate_b(self):
        return self.engine.arbitrate_rule(self.rule_b)


class RandomProcessor:
    def __init__(self, mid, rule_a, rule_b, mode, index, bias, inputs, outputs):
        self.mid, self.rule_a, self.rule_b = mid, rule_a, rule_b
        self.mode, self.index, self.bias = mode, index, bias
        self.inputs, self.outputs = tuple(inputs), tuple(outputs)

    def Work(self):
        e = self.engine
        e.record_read(self.mid, self.mode.qid)
        e.record_read(self.mid, self.bias.qid)
        if self.mode.peek() == 0:
            self.work_a(self.bias.peek())
        else:
            self.work_b(self.bias.peek())

    def work_a(self, offset):
        if self.engine.begin_rule(self.rule_a, (offset,)):
            self._compute(self.rule_a, offset)

    def work_b(self, offset):
        if self.engine.begin_rule(self.rule_b, (offset,)):
            self._compute(self.rule_b, offset)

    def _compute(self, rid, offset):
        e = self.engine
        try:
            e.record_read(self.mid, self.index.qid, rid)
            source = self.inputs[self.index.peek()]
            e.record_read(self.mid, source.qid, rid)
            value = source.peek()
            source.propose_pop(rid)
            if (value + offset) % 7 == 0:
                e.complete_rule(rid)
                return
            self.outputs[(value + offset) % 2].propose_push(rid, (value, offset))
        except NeedInput:
            e.abort_rule(rid)
            return
        e.complete_rule(rid)

    def arbitrate_a(self):
        return self.engine.arbitrate_rule(self.rule_a)

    def arbitrate_b(self):
        return self.engine.arbitrate_rule(self.rule_b)
