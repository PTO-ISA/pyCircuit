"""Decode: explicit generated-style Rule and runtime calls."""

from .isa import decode, sources
from .records import Decoded, LW


class Decode:
    def __init__(self, mid, rid, source, output, control, registers, writeback):
        self.mid, self.rid, self.engine = mid, rid, None
        self.source = source
        self.output = output
        self.control = control
        self.registers = registers
        self.writeback = writeback

    def Work(self):
        self.work_decode()

    def work_decode(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        fetched = self.source.try_peek()
        if fetched is None:
            e.abort_rule(rid)
            return
        self.source.propose_pop(rid)
        control = self.control.peek()
        if fetched.epoch != control.epoch:
            e.complete_rule(rid)  # Commit the wrong-path discard pop.
            return
        instruction = decode(fetched.word)
        rs1, rs2 = sources(instruction)
        previous = self.output.try_peek()
        if (previous is not None and previous.instruction.op == LW
                and previous.instruction.rd != 0
                and previous.instruction.rd in (rs1, rs2)):
            e.abort_rule(rid)  # Existing CPU readiness condition; retain subscriptions.
            return
        values = []
        for rs in (rs1, rs2):
            if rs == 0:
                values.append(0)
                continue
            wb = self.writeback.try_peek()
            if wb is not None and wb.rd == rs:
                values.append(wb.value)
            else:
                queue = self.registers[rs]
                values.append(queue.peek())
        self.output.propose_push(rid, Decoded(fetched.pc, instruction, fetched.epoch, *values))

        e.complete_rule(rid)

    def arbitrate_decode(self):
        return self.engine.arbitrate_rule(self.rid)
