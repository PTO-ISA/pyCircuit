"""WB: architectural register writes and one immutable retirement record."""

from .stage import Stage


class Writeback(Stage):
    def __init__(self, mid, rid, source, registers, retirement):
        super().__init__(mid, rid)
        self.source, self.registers, self.retirement = source, registers, retirement

    def work_stage(self):
        last = self.observe(self.retirement)
        if last.instruction.halt:
            return True
        instruction = self.take(self.source)
        if instruction is None:
            return False
        if instruction.rd:
            self.registers[instruction.rd].propose_revise(self.rid, instruction.value)
        # Constant field paths stand for generated C++ member pointers.
        self.retirement.propose_revise(self.rid, last.sequence + 1, (0,))
        self.retirement.propose_revise(self.rid, instruction, (1,))
        return True
