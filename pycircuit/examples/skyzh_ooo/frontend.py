"""Fetch, dispatch and operand views; all resource tests use old state."""
from .logic import *


@ac.signal
def fetch(pc, memory) -> Instruction:
    address = pc.value
    assert address < 0x400000 and address % 4 == 0
    page = memory[address >> 8].value
    return decode(page.words[(address >> 2) & 63])


@ac.signal
def operands(rename, registers, rob) -> OperandView:
    result = OperandView()
    for i in range(1, 32):
        r = rename[i].value
        if r.busy:
            entry = rob[r.tag - 1].value
            if entry.ready:
                result.registers[i] = Operand(value=entry.value)
            else:
                result.registers[i] = Operand(tag=r.tag)
        else:
            result.registers[i] = Operand(value=registers[i].value)
    return result


@ac.signal
def dispatch(pc, instruction, values, head, tail, stations, predictor) -> Dispatch:
    address = pc.value
    ins = instruction.value
    front = head.value
    rear = tail.value
    result = Dispatch(next_pc=address, next_tail=rear)
    # AUIPC is decoded but never dispatched by the pinned Issue.cpp.
    if ins.opcode == 0x17:
        return result
    required = ac.u32(2) if ins.opcode == 0x67 else ac.u32(1)
    begin = ac.u32(7) if ins.opcode == 3 else ac.u32(4) if ins.opcode == 0x23 else ac.u32(0)
    end = ac.u32(10) if ins.opcode == 3 else ac.u32(7) if ins.opcode == 0x23 else ac.u32(4)
    selected: ac.array[ac.u32, 2] = [0, 0]
    count = ac.u32(0)
    for i in range(begin, end):
        if count < required and stations[i].empty():
            selected[count] = i
            count = count + 1
    if count != required:
        return result
    next_tail = rear
    for i in range(required):
        next_tail = next_rob(next_tail)
        if next_tail == front:
            return result
    result.count = required
    result.next_tail = next_tail
    result.next_pc = address + 4
    sources = values.value.registers
    slot = Station(op=alu_op(ins), left=sources[ins.rs1], right=sources[ins.rs2], rob=rear, pc=address)
    entry = ROBEntry(ins=ins, pc=address, dest=ins.rd)
    if ins.opcode == 0x13:
        slot.right = Operand(value=ins.imm)
    elif ins.opcode == 0x37:
        slot.left = Operand()
        slot.right = Operand(value=ins.imm)
    elif ins.opcode == 3 or ins.opcode == 0x23:
        slot.op = ins.funct3
        slot.address = ins.imm
        if ins.opcode == 3:
            slot.right = Operand()
    elif ins.opcode == 0x63:
        entry.dest = address
        page = predictor[address >> 8].value
        history = ac.u32(page.history[address & 255]) & 3
        counter_page = predictor[(address + history) >> 8].value
        take = counter_page.counters[(address + history) & 255] >= 2
        result.next_pc = address + ins.imm if take else address + 4
        entry.predicted = result.next_pc
    elif ins.opcode == 0x6f or ins.opcode == 0x67:
        slot.left = Operand(value=4)
        slot.right = Operand(value=address)
        if ins.opcode == 0x6f:
            result.next_pc = address + ins.imm
        else:
            slot.left = Operand(value=address)
            slot.right = Operand(value=4)
            entry.ins.opcode = 0x6f
            jump = Allocation(valid=True, station=selected[1], rob=next_rob(rear),
                              slot=Station(op=ADD, left=sources[ins.rs1], right=Operand(value=ins.imm),
                                           rob=next_rob(rear), pc=address),
                              entry=ROBEntry(ins=ins, pc=address))
            result.allocations[1] = jump
    result.allocations[0] = Allocation(valid=True, station=selected[0], rob=rear, slot=slot, entry=entry)
    result.rd = ins.rd
    result.rename_tag = rear
    result.rename = ins.rd != 0
    return result


@ac.module
def Frontend(pc, allocation, retirement):
    @ac.rule
    def advance():
        issue = allocation.value
        commit = retirement.value
        pc.value = commit.next_pc if commit.flush else issue.next_pc
    advance()
