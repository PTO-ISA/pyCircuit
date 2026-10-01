"""Explicit wiring; the testbench only loads code and observes completed ticks."""

from engine import Queue, RuleEntry
from construction import assemble as assemble_netlist
from .isa import assemble
from .records import FrontControl, Busy, Retired
from .fetch import Fetch
from .decode import Decode
from .execute import Execute
from .memory import Memory
from .writeback import Writeback


class CPU:
    def __init__(self, words, sim, stages, links, registers, data, busy, retirement):
        self.words, self.sim, self.stages, self.links = words, sim, stages, links
        self.registers, self.data, self.busy, self.retirement = registers, data, busy, retirement
        self.retired, self.retire_ticks, self.trace = [], [], []
        self.work_history = []

    @property
    def halted(self):
        return self.retirement.peek().instruction.halt

    def step(self, trace=False):
        tick = self.sim.tick
        before = tuple(None if q.empty() else q.peek().pc for q in self.links)
        busy = self.busy.peek()
        sequence = self.retirement.peek().sequence
        accepted = self.sim.step()
        last = self.retirement.peek()
        if trace:
            self.trace.append((tick, before, busy.instruction.pc if busy.valid else None, accepted))
            self.work_history.append((tuple(self.sim.module_calls), tuple(self.sim.rule_calls)))
        if last.sequence != sequence:
            self.retired.append(last.instruction)
            self.retire_ticks.append(tick)
            return last.instruction
        return None

    def run(self, max_cycles=10000, trace=False):
        for _ in range(max_cycles):
            if self.halted:
                return self
            self.step(trace)
        if self.halted:
            return self
        raise TimeoutError(f'CPU did not HALT within {max_cycles} cycles')

    def register_values(self):
        return tuple(q.peek() for q in self.registers)

    def memory_values(self):
        return tuple(q.peek() for q in self.data)


def build_cpu(program, memory_latency=1, data_words=256, cache=True, reverse=False):
    if memory_latency < 1 or data_words < 1:
        raise ValueError('latency and data_words must be positive')
    words = assemble(program)
    queues = []

    def queue(initial=()):
        q = Queue(1, initial)
        q.qid = len(queues)
        queues.append(q)
        return q

    if_id, id_ex, ex_mem, mem_wb = links = tuple(queue() for _ in range(4))
    pc, control, redirect = queue((0,)), queue((FrontControl(),)), queue()
    registers = tuple(queue((0,)) for _ in range(32))
    data = tuple(queue((0,)) for _ in range(data_words))
    busy, retirement = queue((Busy(),)), queue((Retired(),))
    stages = (
        Fetch(0, 1, words, pc, control, redirect, if_id),
        Decode(1, 2, if_id, id_ex, control, registers, mem_wb),
        Execute(2, 3, id_ex, ex_mem, control, redirect, busy, mem_wb),
        Memory(3, 4, ex_mem, mem_wb, data, busy, memory_latency),
        Writeback(4, 5, mem_wb, registers, retirement),
    )
    bindings = (
        ((redirect,), (if_id,), (pc,)),
        ((if_id,), (id_ex,), ()),
        ((id_ex,), (ex_mem, redirect), (control,)),
        ((ex_mem,), (mem_wb,), data + (busy,)),
        ((mem_wb,), (), registers[1:] + (retirement,)),
    )
    modules = list(reversed(stages)) if reverse else list(stages)
    for mid, stage in enumerate(modules):
        stage.mid = mid
    entries = [None]
    for stage, (pops, pushes, revises) in zip(stages, bindings):
        entries.append(RuleEntry(stage.mid, stage.work_stage, stage.arbitrate,
                                 tuple(q.qid for q in pops), tuple(q.qid for q in pushes),
                                 tuple(q.qid for q in revises)))
    sim = assemble_netlist(queues, modules, entries, cache)
    return CPU(words, sim, stages, links, registers, data, busy, retirement)
