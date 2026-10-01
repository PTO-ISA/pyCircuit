"""EX: operand forwarding, ALU, branch resolution and front-end invalidation."""

from .stage import Stage
from .isa import sources
from .records import (INVALID, ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, SW,
                      BEQ, BNE, JAL, JALR, HALT, LOAD, STORE, NO_MEMORY,
                      Executed, FrontControl, Redirect, ModelError, u32, signed)


class Execute(Stage):
    def __init__(self, mid, rid, source, output, control, redirect, busy, writeback):
        super().__init__(mid, rid)
        self.source, self.output, self.control, self.redirect = source, output, control, redirect
        self.busy, self.writeback = busy, writeback

    def operand(self, rs, captured):
        if rs == 0:
            return True, 0
        newest = self.observe(self.output)
        if newest is not None and newest.rd == rs:
            if newest.memory == LOAD:
                return False, 0
            return True, newest.result
        busy = self.observe(self.busy)
        if busy.valid and busy.instruction.rd == rs:
            return False, 0                # A load is still inside the memory unit.
        wb = self.observe(self.writeback)
        if wb is not None and wb.rd == rs:
            return True, wb.value
        return True, captured

    def work_stage(self):
        decoded = self.take(self.source)
        if decoded is None:
            return False
        control = self.observe(self.control)
        if decoded.epoch != control.epoch:
            return True
        inst, pc = decoded.instruction, decoded.pc
        if inst.op == INVALID:
            raise ModelError(f'illegal instruction 0x{inst.word:08x} at PC 0x{pc:x}')
        rs1, rs2 = sources(inst)
        ready, a = self.operand(rs1, decoded.left)
        if not ready:
            return False
        ready, b = self.operand(rs2, decoded.right)
        if not ready:
            return False
        result, memory, address, target = 0, NO_MEMORY, 0, None
        if inst.op == ADD: result = a + b
        elif inst.op == ADDI: result = a + inst.immediate
        elif inst.op == SUB: result = a - b
        elif inst.op == AND: result = a & b
        elif inst.op == OR: result = a | b
        elif inst.op == XOR: result = a ^ b
        elif inst.op == SLT: result = int(signed(a) < signed(b))
        elif inst.op == LUI: result = inst.immediate
        elif inst.op in (LW, SW):
            memory, address = (LOAD if inst.op == LW else STORE), u32(a + inst.immediate)
        elif inst.op in (BEQ, BNE):
            if (a == b) == (inst.op == BEQ):
                target = u32(pc + inst.immediate)
        elif inst.op in (JAL, JALR):
            result = pc + 4
            target = u32(pc + inst.immediate) if inst.op == JAL else u32(a + inst.immediate) & ~1
        if target is not None:
            if target % 4:
                raise ModelError(f'unaligned instruction target 0x{target:x}')
            self.redirect.propose_push(self.rid, Redirect(target, control.epoch + 1))
            self.control.propose_revise(self.rid, FrontControl(control.epoch + 1, False))
        if inst.op == HALT:
            self.control.propose_revise(self.rid, FrontControl(control.epoch + 1, True))
        self.output.propose_push(self.rid, Executed(
            pc, inst.word, inst.rd, u32(result), memory, address, b, inst.op == HALT))
        return True
