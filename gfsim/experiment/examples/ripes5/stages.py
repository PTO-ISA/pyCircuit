"""Plain generated-style Modules; each Rule spells out its runtime operations.

Pipeline links still model registers with revise, not consuming FIFO inputs.
All persistent state and the two shared Signals are bound explicitly.
"""
from .isa import decode
from .records import LW, SW, JAL, JALR, u32
from .logic import Slot, Event, Store, writer


class Fetch:
    def __init__(self, mid, rid, *, pc, if_id, ex_result, load_use_stall, words):
        self.mid, self.rid, self.engine = mid, rid, None
        self.pc = pc
        self.if_id = if_id
        self.ex_result, self.load_use_stall = ex_result, load_use_stall
        self.words = words

    def Work(self):
        self.work_fetch()

    def work_fetch(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        ex = self.ex_result.value
        stall = self.load_use_stall.value
        pc = self.pc.peek()
        if not stall:
            self.pc.propose_revise(rid, ex.next_slot.result if ex.redirect else u32(pc + 4))
            if ex.redirect:
                new = Slot()
            else:
                if pc % 4:
                    raise ValueError(f'unaligned instruction address outside scope: {pc:#x}')
                word = self.words[pc // 4] if 0 <= pc < 4 * len(self.words) else 0
                new = Slot(True, pc, word)
            self.if_id.propose_revise(rid, new)

        e.complete_rule(rid)

    def arbitrate_fetch(self):
        return self.engine.arbitrate_rule(self.rid)


class Decode:
    def __init__(self, mid, rid, *, if_id, id_ex, mem_wb, registers, ex_result, load_use_stall):
        self.mid, self.rid, self.engine = mid, rid, None
        self.if_id, self.id_ex = if_id, id_ex
        self.mem_wb = mem_wb
        self.ex_result, self.load_use_stall = ex_result, load_use_stall
        self.registers = registers

    def Work(self):
        self.work_decode()

    def work_decode(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        ex = self.ex_result.value
        stall = self.load_use_stall.value
        if ex.redirect or stall:
            new = Slot(stalled=stall)
        else:
            old = self.if_id.peek()
            wb = self.mem_wb.peek()
            ins = decode(old.word)
            values = []
            for idx in (ins.rs1, ins.rs2):
                queue = self.registers[idx]
                value = queue.peek()
                values.append(wb.value if idx and idx == writer(wb) else value)
            new = Slot(old.valid, old.pc, old.word, *values)
        self.id_ex.propose_revise(rid, new)

        e.complete_rule(rid)

    def arbitrate_decode(self):
        return self.engine.arbitrate_rule(self.rid)


class Execute:
    def __init__(self, mid, rid, *, ex_mem, ex_result):
        self.mid, self.rid, self.engine = mid, rid, None
        self.ex_mem, self.ex_result = ex_mem, ex_result

    def Work(self):
        self.work_execute()

    def work_execute(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        self.ex_mem.propose_revise(rid, self.ex_result.value.next_slot)

        e.complete_rule(rid)

    def arbitrate_execute(self):
        return self.engine.arbitrate_rule(self.rid)


class Memory:
    def __init__(self, mid, rid, *, ex_mem, mem_wb, data, data_base, store):
        self.mid, self.rid, self.engine = mid, rid, None
        self.ex_mem, self.mem_wb = ex_mem, mem_wb
        self.data, self.data_base, self.store = data, data_base, store

    def Work(self):
        self.work_memory()

    def work_memory(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        mem = self.ex_mem.peek()
        ins = decode(mem.word)
        value = u32(mem.pc + 4) if ins.op in (JAL, JALR) else mem.result
        if ins.op in (LW, SW):
            index, offset = divmod(mem.result - self.data_base, 4)
            if offset or not 0 <= index < len(self.data):
                raise ValueError(f'data access outside aligned data region: {mem.result:#x}')
            queue = self.data[index]
            if ins.op == LW:
                value = queue.peek()
            else:
                queue.propose_revise(rid, mem.right)
                old = self.store.peek()
                self.store.propose_revise(rid, Store(old.sequence + 1, mem.result, mem.right))
        self.mem_wb.propose_revise(rid, mem._replace(value=value))

        e.complete_rule(rid)

    def arbitrate_memory(self):
        return self.engine.arbitrate_rule(self.rid)


class Writeback:
    def __init__(self, mid, rid, *, mem_wb, registers, retirement, code_size):
        self.mid, self.rid, self.engine = mid, rid, None
        self.mem_wb, self.registers = mem_wb, registers
        self.retirement, self.code_size = retirement, code_size

    def Work(self):
        self.work_writeback()

    def work_writeback(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        wb = self.mem_wb.peek()
        rd = writer(wb)
        if rd:
            self.registers[rd].propose_revise(rid, wb.value)
        if wb.valid and wb.pc % 4 == 0 and 0 <= wb.pc < self.code_size:
            old = self.retirement.peek()
            self.retirement.propose_revise(rid, Event(old.sequence + 1, wb.pc, wb.word,
                                                     rd, wb.value if rd else 0))

        e.complete_rule(rid)

    def arbitrate_writeback(self):
        return self.engine.arbitrate_rule(self.rid)
