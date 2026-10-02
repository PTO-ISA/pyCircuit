"""Writeback: explicit generated-style Rule and runtime calls."""


class Writeback:
    def __init__(self, mid, rid, source, registers, retirement):
        self.mid, self.rid, self.engine = mid, rid, None
        self.source = source
        self.registers = registers
        self.retirement = retirement

    def Work(self):
        self.work_writeback()

    def work_writeback(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        last = self.retirement.peek()
        if last.instruction.halt:
            e.complete_rule(rid)
            return
        instruction = self.source.try_peek()
        if instruction is None:
            e.abort_rule(rid)
            return
        self.source.propose_pop(rid)
        if instruction.rd:
            self.registers[instruction.rd].propose_revise(rid, instruction.value)
        self.retirement.propose_revise(rid, last.sequence + 1, (0,))
        self.retirement.propose_revise(rid, instruction, (1,))

        e.complete_rule(rid)

    def arbitrate_writeback(self):
        return self.engine.arbitrate_rule(self.rid)
