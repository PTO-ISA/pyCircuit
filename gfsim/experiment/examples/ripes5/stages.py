"""Plain generated-style Modules; each Rule spells out its runtime operations.

Pipeline links still model registers with revise, not consuming FIFO inputs.
All persistent state is bound explicitly; combinational helpers receive values.
"""
from ..riscv.isa import decode
from ..riscv.records import LW, SW, JAL, JALR, u32
from .logic import Slot, Event, Store, control, execute, writer


class Fetch:
    def __init__(self, mid, rid, *, pc, if_id, id_ex, ex_mem, mem_wb, words):
        self.mid, self.rid, self.engine = mid, rid, None
        self.pc = pc
        self.if_id, self.id_ex = if_id, id_ex
        self.ex_mem, self.mem_wb = ex_mem, mem_wb
        self.words = words

    def Work(self):
        self.work_fetch()

    def work_fetch(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        e.record_read(self.mid, self.if_id.qid, rid)
        id_slot = self.if_id.try_peek()
        if id_slot is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.id_ex.qid, rid)
        ex = self.id_ex.try_peek()
        if ex is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.ex_mem.qid, rid)
        mem = self.ex_mem.try_peek()
        if mem is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.mem_wb.qid, rid)
        wb = self.mem_wb.try_peek()
        if wb is None:
            e.abort_rule(rid)
            return
        c = control(id_slot, ex, mem, wb)

        e.record_read(self.mid, self.pc.qid, rid)
        pc = self.pc.try_peek()
        if pc is None:
            e.abort_rule(rid)
            return
        if c['pc_enable']:
            self.pc.propose_revise(rid, c['target'] if c['target'] is not None else u32(pc + 4))
            if c['flush_ifid']:
                new = Slot()
            else:
                if pc % 4:
                    raise ValueError(f'unaligned instruction address outside scope: {pc:#x}')
                word = self.words[pc // 4] if 0 <= pc < 4 * len(self.words) else 0
                new = Slot(True, pc, word)
            self.if_id.propose_revise(rid, new)

        # Explicit clock events also cover unchanged values and stable loops.
        e.request_wakeup(rid, self.mid, 1)
        e.complete_rule(rid)

    def arbitrate_fetch(self):
        return self.engine.arbitrate_rule(self.rid)


class Decode:
    def __init__(self, mid, rid, *, if_id, id_ex, ex_mem, mem_wb, registers):
        self.mid, self.rid, self.engine = mid, rid, None
        self.if_id, self.id_ex = if_id, id_ex
        self.ex_mem, self.mem_wb = ex_mem, mem_wb
        self.registers = registers

    def Work(self):
        self.work_decode()

    def work_decode(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        e.record_read(self.mid, self.if_id.qid, rid)
        old = self.if_id.try_peek()
        if old is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.id_ex.qid, rid)
        ex = self.id_ex.try_peek()
        if ex is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.ex_mem.qid, rid)
        mem = self.ex_mem.try_peek()
        if mem is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.mem_wb.qid, rid)
        wb = self.mem_wb.try_peek()
        if wb is None:
            e.abort_rule(rid)
            return
        # This Rule independently recomputes control from registered current.
        c = control(old, ex, mem, wb)

        if c['flush_idex']:
            new = Slot(stalled=c['stall'])
        else:
            ins = decode(old.word)
            values = []
            for idx in (ins.rs1, ins.rs2):
                queue = self.registers[idx]
                e.record_read(self.mid, queue.qid, rid)
                value = queue.try_peek()
                if value is None:
                    e.abort_rule(rid)
                    return
                values.append(wb.value if idx and idx == writer(wb) else value)
            new = Slot(old.valid, old.pc, old.word, *values)
        self.id_ex.propose_revise(rid, new)

        e.request_wakeup(rid, self.mid, 1)
        e.complete_rule(rid)

    def arbitrate_decode(self):
        return self.engine.arbitrate_rule(self.rid)


class Execute:
    def __init__(self, mid, rid, *, id_ex, ex_mem, mem_wb):
        self.mid, self.rid, self.engine = mid, rid, None
        self.id_ex, self.ex_mem, self.mem_wb = id_ex, ex_mem, mem_wb

    def Work(self):
        self.work_execute()

    def work_execute(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        e.record_read(self.mid, self.id_ex.qid, rid)
        ex = self.id_ex.try_peek()
        if ex is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.ex_mem.qid, rid)
        mem = self.ex_mem.try_peek()
        if mem is None:
            e.abort_rule(rid)
            return
        e.record_read(self.mid, self.mem_wb.qid, rid)
        wb = self.mem_wb.try_peek()
        if wb is None:
            e.abort_rule(rid)
            return

        result, data, _, _, _ = execute(ex, mem, wb)
        self.ex_mem.propose_revise(rid, ex._replace(result=result, right=data))

        e.request_wakeup(rid, self.mid, 1)
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

        e.record_read(self.mid, self.ex_mem.qid, rid)
        mem = self.ex_mem.try_peek()
        if mem is None:
            e.abort_rule(rid)
            return
        ins = decode(mem.word)
        value = u32(mem.pc + 4) if ins.op in (JAL, JALR) else mem.result
        if ins.op in (LW, SW):
            index, offset = divmod(mem.result - self.data_base, 4)
            if offset or not 0 <= index < len(self.data):
                raise ValueError(f'data access outside aligned data region: {mem.result:#x}')
            queue = self.data[index]
            if ins.op == LW:
                e.record_read(self.mid, queue.qid, rid)
                value = queue.try_peek()
                if value is None:
                    e.abort_rule(rid)
                    return
            else:
                queue.propose_revise(rid, mem.right)
                e.record_read(self.mid, self.store.qid, rid)
                old = self.store.try_peek()
                if old is None:
                    e.abort_rule(rid)
                    return
                self.store.propose_revise(rid, Store(old.sequence + 1, mem.result, mem.right))
        self.mem_wb.propose_revise(rid, mem._replace(value=value))

        e.request_wakeup(rid, self.mid, 1)
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

        e.record_read(self.mid, self.mem_wb.qid, rid)
        wb = self.mem_wb.try_peek()
        if wb is None:
            e.abort_rule(rid)
            return
        rd = writer(wb)
        if rd:
            self.registers[rd].propose_revise(rid, wb.value)
        if wb.valid and wb.pc % 4 == 0 and 0 <= wb.pc < self.code_size:
            e.record_read(self.mid, self.retirement.qid, rid)
            old = self.retirement.try_peek()
            if old is None:
                e.abort_rule(rid)
                return
            self.retirement.propose_revise(rid, Event(old.sequence + 1, wb.pc, wb.word,
                                                     rd, wb.value if rd else 0))

        e.request_wakeup(rid, self.mid, 1)
        e.complete_rule(rid)

    def arbitrate_writeback(self):
        return self.engine.arbitrate_rule(self.rid)
