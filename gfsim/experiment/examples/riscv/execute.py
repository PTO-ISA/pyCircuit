"""Execute: explicit generated-style Rule and runtime calls."""

from .isa import sources
from .records import (INVALID, ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, SW,
                      BEQ, BNE, JAL, JALR, HALT, LOAD, STORE, NO_MEMORY,
                      Executed, FrontControl, Redirect, ModelError, u32, signed)


class Execute:
    def __init__(self, mid, rid, source, output, control, redirect, busy, writeback):
        self.mid, self.rid, self.engine = mid, rid, None
        self.source = source
        self.output = output
        self.control = control
        self.redirect = redirect
        self.busy = busy
        self.writeback = writeback

    def Work(self):
        self.work_execute()

    def work_execute(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        decoded = self.source.try_peek()
        if decoded is None:
            e.abort_rule(rid)
            return
        self.source.propose_pop(rid)
        control = self.control.peek()
        if decoded.epoch != control.epoch:
            e.complete_rule(rid)
            return
        inst, pc = decoded.instruction, decoded.pc
        if inst.op == INVALID:
            raise ModelError(f'illegal instruction 0x{inst.word:08x} at PC 0x{pc:x}')
        rs1, rs2 = sources(inst)
        values = []
        for rs, captured in ((rs1, decoded.left), (rs2, decoded.right)):
            if rs == 0:
                values.append(0)
                continue
            newest = self.output.try_peek()
            if newest is not None and newest.rd == rs:
                if newest.memory == LOAD:
                    e.abort_rule(rid)
                    return
                values.append(newest.result)
                continue
            busy = self.busy.peek()
            if busy.valid and busy.instruction.rd == rs:
                e.abort_rule(rid)
                return
            wb = self.writeback.try_peek()
            values.append(wb.value if wb is not None and wb.rd == rs else captured)
        a, b = values
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
            self.redirect.propose_push(rid, Redirect(target, control.epoch + 1))
            self.control.propose_revise(rid, FrontControl(control.epoch + 1, False))
        if inst.op == HALT:
            self.control.propose_revise(rid, FrontControl(control.epoch + 1, True))
        self.output.propose_push(rid, Executed(
            pc, inst.word, inst.rd, u32(result), memory, address, b, inst.op == HALT))

        e.complete_rule(rid)

    def arbitrate_execute(self):
        return self.engine.arbitrate_rule(self.rid)
