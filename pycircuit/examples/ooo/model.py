from pycircuit import ac
from .types import *
from .frontend import Fetch, Dispatch, room
from .issue import Wakeup, Issue, oldest_ready
from .execute import Integer, Memory, Writeback
from .commit import Commit, Clock

@ac.module
def CPU(words: ac.vector[ac.u32], data_base: ac.u32,
        initial_registers: ac.vector[ac.u32], initial_data: ac.vector[ac.u32],
        wb_period: ac.u32, wb_closed: ac.u32):
    control = ac.queue[Control](initial=Control())
    front = ac.queue[Front](initial=Front())
    tail = ac.queue[Tail](initial=Tail())
    clock = ac.queue[ac.u64](initial=0)
    entries = [ac.queue[Entry](initial=Entry()) for i in range(8)]
    operands = [ac.queue[Operands](initial=Operands()) for i in range(8)]
    int_issued = [ac.queue[Tag](initial=Tag()) for i in range(8)]
    mem_issued = [ac.queue[Tag](initial=Tag()) for i in range(8)]
    int_results = [ac.queue[Completion](initial=Completion()) for i in range(8)]
    mem_results = [ac.queue[Completion](initial=Completion()) for i in range(8)]
    rename = [ac.queue[Tag](initial=Tag()) for i in range(32)]
    registers = [ac.queue[ac.u32](initial=v) for v in initial_registers]
    data = [ac.queue[ac.u32](initial=v) for v in initial_data]
    pending = ac.queue[Pending](initial=Pending())
    retirement = ac.queue[Retirement](initial=Retirement())
    flush = ac.queue[Flush](initial=Flush())
    has_room = room(control, tail)
    int_pick = oldest_ready(control, entries, operands, int_issued, False)
    mem_pick = oldest_ready(control, entries, operands, mem_issued, True)
    fetched = Fetch(control, front, words)
    dispatch = Dispatch(fetched, control, tail, has_room, entries, rename, registers, int_results, mem_results)
    wake = Wakeup(control, entries, operands, int_results, mem_results)
    int_requests = Issue(control, entries, operands, int_issued, int_pick)
    mem_requests = Issue(control, entries, operands, mem_issued, mem_pick)
    int_completed = Integer(int_requests, control)
    mem_completed = Memory(mem_requests, control, pending, data, data_base)
    int_writeback = Writeback(int_completed, control, entries, int_results, clock, wb_period, wb_closed)
    mem_writeback = Writeback(mem_completed, control, entries, mem_results, clock, wb_period, wb_closed)
    commit = Commit(control, entries, int_results, mem_results, registers, data, data_base, retirement, flush)
    timer = Clock(control, clock)
