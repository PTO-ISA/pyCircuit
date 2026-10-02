"""Explicit singleton Queue state and generated-style static resource bindings."""
from engine import Queue, RuleEntry
from construction import assemble
from .logic import Slot, Event, Store, control
from .programs import validate
from .stages import Fetch, Decode, Execute, Memory, Writeback


class CPU:
    def __init__(self, case, *, cache=True, reverse=False):
        validate(case)
        self.words = tuple(case['words'])
        self.data_base = case['data_base']
        self.pc = Queue(initial=(0,))
        self.if_id = Queue(initial=(Slot(),))
        self.id_ex = Queue(initial=(Slot(),))
        self.ex_mem = Queue(initial=(Slot(),))
        self.mem_wb = Queue(initial=(Slot(),))
        self.links = (self.if_id, self.id_ex, self.ex_mem, self.mem_wb)
        self.registers = tuple(Queue(initial=(value,)) for value in case['registers'])
        self.data = tuple(Queue(initial=(value,)) for value in case['data'])
        self.retirement = Queue(initial=(Event(),))
        self.store = Queue(initial=(Store(),))
        self.queues = [self.pc, *self.links, *self.registers, *self.data,
                       self.retirement, self.store]
        names = ['PC', 'IF_ID', 'ID_EX', 'EX_MEM', 'MEM_WB']
        names += [f'x{i}' for i in range(len(self.registers))]
        names += [f'data[{self.data_base + i * 4:#x}]' for i in range(len(self.data))]
        names += ['retirement', 'store']
        self.names = dict(enumerate(names))
        for qid, queue in enumerate(self.queues):
            queue.qid = qid

        fetch = Fetch(0, 1, pc=self.pc, if_id=self.if_id, id_ex=self.id_ex,
                      ex_mem=self.ex_mem, mem_wb=self.mem_wb, words=self.words)
        decode = Decode(1, 2, if_id=self.if_id, id_ex=self.id_ex, ex_mem=self.ex_mem,
                        mem_wb=self.mem_wb, registers=self.registers)
        execute = Execute(2, 3, id_ex=self.id_ex, ex_mem=self.ex_mem, mem_wb=self.mem_wb)
        memory = Memory(3, 4, ex_mem=self.ex_mem, mem_wb=self.mem_wb, data=self.data,
                        data_base=self.data_base, store=self.store)
        writeback = Writeback(4, 5, mem_wb=self.mem_wb, registers=self.registers,
                              retirement=self.retirement, code_size=4 * len(self.words))
        self.stages = (fetch, decode, execute, memory, writeback)
        modules = list(reversed(self.stages)) if reverse else list(self.stages)
        for mid, module in enumerate(modules):
            module.mid = mid
        # RuleId is the table index. These are register-state bindings: no FIFO
        # pop/push is modeled in this behavior-preserving refactor.
        rules = [
            None,
            RuleEntry(fetch.mid, fetch.work_fetch, fetch.arbitrate_fetch,
                      pops=(), pushes=(), revises=(self.pc.qid, self.if_id.qid)),
            RuleEntry(decode.mid, decode.work_decode, decode.arbitrate_decode,
                      pops=(), pushes=(), revises=(self.id_ex.qid,)),
            RuleEntry(execute.mid, execute.work_execute, execute.arbitrate_execute,
                      pops=(), pushes=(), revises=(self.ex_mem.qid,)),
            RuleEntry(memory.mid, memory.work_memory, memory.arbitrate_memory,
                      pops=(), pushes=(), revises=(self.mem_wb.qid, self.store.qid)
                      + tuple(q.qid for q in self.data)),
            RuleEntry(writeback.mid, writeback.work_writeback, writeback.arbitrate_writeback,
                      pops=(), pushes=(), revises=(self.retirement.qid,)
                      + tuple(q.qid for q in self.registers[1:])),
        ]
        self.sim = assemble(self.queues, modules, rules, cache)

    def executable(self, pc):
        return pc % 4 == 0 and 0 <= pc < 4 * len(self.words)

    def word_at(self, pc):
        if pc % 4:
            raise ValueError(f'unaligned instruction address outside scope: {pc:#x}')
        return self.words[pc // 4] if self.executable(pc) else 0

    def data_index(self, address):
        index, offset = divmod(address - self.data_base, 4)
        if offset or not 0 <= index < len(self.data):
            raise ValueError(f'data access outside aligned data region: {address:#x}')
        return index

    def snapshot(self, retire=None, store=None):
        # External read-only observation; not part of any Rule.
        slots = [q.peek() for q in self.links]
        c = control(*slots)
        pc = self.pc.peek()
        next_fetch = c['target'] if c['target'] is not None else (pc + 4) & 0xffffffff
        stages = [dict(valid=self.executable(pc), pc=pc if self.executable(pc) else None)]
        stages += [dict(valid=s.valid and self.executable(s.pc),
                        pc=s.pc if s.valid and self.executable(s.pc) else None) for s in slots]
        return dict(cycle=self.sim.tick, stages=stages, fetch_pc=pc,
                    next_fetch=next_fetch, next_pc=next_fetch if c['pc_enable'] else pc,
                    control=c, stalled=[False, False] + [s.stalled for s in slots[1:]],
                    registers=[q.peek() for q in self.registers],
                    data=[q.peek() for q in self.data],
                    retired=self.retirement.peek().sequence, retire=retire, store=store)

    def step(self):
        previous, store_before = self.retirement.peek(), self.store.peek()
        self.sim.step()
        event, write = self.retirement.peek(), self.store.peek()
        retire = None if event.sequence == previous.sequence else dict(
            pc=event.pc, word=event.word, write=[event.rd, event.value] if event.rd else None)
        store = None if write.sequence == store_before.sequence else [write.address, write.value]
        return self.snapshot(retire, store)
