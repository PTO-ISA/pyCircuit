"""ID: decode, register read/WB bypass, and the immediate load-use interlock."""

from .stage import Stage
from .isa import decode, sources
from .records import Decoded, LW


class Decode(Stage):
    def __init__(self, mid, rid, source, output, control, registers, writeback):
        super().__init__(mid, rid)
        self.source, self.output, self.control = source, output, control
        self.registers, self.writeback = registers, writeback

    def read_operand(self, rs):
        if rs == 0:
            return 0
        wb = self.observe(self.writeback)
        if wb is not None and wb.rd == rs:
            return wb.value
        return self.observe(self.registers[rs])

    def work_stage(self):
        fetched = self.take(self.source)
        if fetched is None:
            return False
        if fetched.epoch != self.observe(self.control).epoch:
            return True                    # Complete the discard pop.
        instruction = decode(fetched.word)
        rs1, rs2 = sources(instruction)
        previous = self.observe(self.output)
        if (previous is not None and previous.instruction.op == LW
                and previous.instruction.rd != 0
                and previous.instruction.rd in (rs1, rs2)):
            return False                   # Not ready; cancel the provisional pop.
        self.output.propose_push(self.rid, Decoded(
            fetched.pc, instruction, fetched.epoch,
            self.read_operand(rs1), self.read_operand(rs2)))
        return True
