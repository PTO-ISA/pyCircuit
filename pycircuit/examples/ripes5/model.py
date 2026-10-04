from pycircuit import ac
from .logic import *

@ac.signal
def ex_signal(id_ex, ex_mem, mem_wb) -> ExResult:
    return execute(id_ex.value, ex_mem.value, mem_wb.value)

@ac.signal
def stall_signal(if_id, id_ex) -> bool:
    return loadUseStall(if_id.value, id_ex.value)

@ac.module
def Fetch(pc, if_id, ex_result, load_use_stall, words):
    @ac.rule
    def fetch():
        ex = ex_result.value
        stall = load_use_stall.value
        current = pc.value
        if not stall:
            pc.value = ex.next_slot.result if ex.redirect else current + 4
            next_slot = Slot()
            if not ex.redirect:
                assert current % 4 == 0
                word = words[current // 4] if current // 4 < len(words) else 0
                next_slot = Slot(True, current, word)
            if_id.value = next_slot

    fetch()

@ac.module
def Decode(if_id, id_ex, mem_wb, registers, ex_result, load_use_stall):
    @ac.rule
    def decode_stage():
        ex = ex_result.value
        stall = load_use_stall.value
        next_slot = Slot()
        if ex.redirect or stall:
            next_slot.stalled = stall
        else:
            old = if_id.value
            wb = mem_wb.value
            ins = decode(old.word)
            a = registers[ins.rs1].value
            b = registers[ins.rs2].value
            left = wb.value if ins.rs1 and ins.rs1 == writer(wb) else a
            right = wb.value if ins.rs2 and ins.rs2 == writer(wb) else b
            next_slot = Slot(old.valid, old.pc, old.word, left, right)
        id_ex.value = next_slot

    decode_stage()

@ac.module
def Execute(ex_mem, ex_result):
    @ac.rule
    def execute_stage():
        ex_mem.value = ex_result.value.next_slot

    execute_stage()

@ac.module
def Memory(ex_mem, mem_wb, data, data_base, store):
    @ac.rule
    def memory():
        mem = ex_mem.value
        ins = decode(mem.word)
        value = mem.pc + 4 if ins.op in (JAL, JALR) else mem.result
        if ins.op in (LW, SW):
            assert mem.result >= data_base
            assert (mem.result - data_base) % 4 == 0
            assert (mem.result - data_base) // 4 < len(data)
            queue = data[(mem.result - data_base) // 4]
            if ins.op == LW:
                value = queue.value
            else:
                queue.value = mem.right
                store.value = Store(store.value.sequence + ac.u64(1), mem.result, mem.right)
        mem.value = value
        mem_wb.value = mem

    memory()

@ac.module
def Writeback(mem_wb, registers, retirement, code_size):
    @ac.rule
    def writeback():
        wb = mem_wb.value
        rd = writer(wb)
        if rd:
            registers[rd].value = wb.value
        if wb.valid and wb.pc % 4 == 0 and wb.pc < code_size:
            retirement.value = Event(retirement.value.sequence + ac.u64(1), wb.pc, wb.word, rd, wb.value if rd else 0)

    writeback()

@ac.module
def CPU(words: ac.vector[ac.u32], data_base: ac.u32,
        initial_registers: ac.vector[ac.u32], initial_data: ac.vector[ac.u32]):
    pc = ac.queue[ac.u32](initial=0)
    if_id = ac.queue[Slot](initial=Slot())
    id_ex = ac.queue[Slot](initial=Slot())
    ex_mem = ac.queue[Slot](initial=Slot())
    mem_wb = ac.queue[Slot](initial=Slot())
    registers = [ac.queue[ac.u32](initial=v) for v in initial_registers]
    data = [ac.queue[ac.u32](initial=v) for v in initial_data]
    retirement = ac.queue[Event](initial=Event())
    store = ac.queue[Store](initial=Store())
    ex_result = ex_signal(id_ex, ex_mem, mem_wb)
    load_use_stall = stall_signal(if_id, id_ex)
    fetch = Fetch(pc, if_id, ex_result, load_use_stall, words)
    decode_stage = Decode(if_id, id_ex, mem_wb, registers, ex_result, load_use_stall)
    execute_stage = Execute(ex_mem, ex_result)
    memory = Memory(ex_mem, mem_wb, data, data_base, store)
    writeback = Writeback(mem_wb, registers, retirement, 4 * len(words))
