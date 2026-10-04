"""Independent ALU and LSU lanes, combined into a combinational broadcast bus."""
from .logic import *


@ac.signal
def alu(stations) -> Execution:
    result = Execution()
    for i in range(4):
        if not stations[i].empty():
            slot = stations[i].value
            if slot.left.tag == 0 and slot.right.tag == 0:
                result.lanes[i] = LaneEffect(complete=True, rob=slot.rob,
                                            value=alu_value(slot.op, slot.left.value, slot.right.value))
    return result


@ac.signal
def lsu(stations, stages, rob, head, memory) -> Execution:
    result = Execution()
    for i in range(4, 10):
        if not stations[i].empty():
            slot = stations[i].value
            state = stages[i].value
            effect = LaneEffect(rob=slot.rob, state=state)
            if state.phase == 0:
                if slot.left.tag == 0:
                    effect.address_valid = True
                    effect.address = slot.left.value + slot.address
                    effect.advance = True
                    effect.state.phase = 1
            elif i < 7:
                if slot.right.tag == 0:
                    effect.complete = True
                    effect.value = slot.right.value
                    effect.advance = True
                    effect.state.phase = 0
            elif state.phase == 1:
                blocked = False
                older = head.value
                for step in range(8):
                    if older != slot.rob:
                        entry = rob[older - 1].value
                        if entry.ins.opcode == 0x23 and entry.dest == slot.address:
                            blocked = True
                        older = next_rob(older)
                if not blocked:
                    address = slot.address
                    width = 1 << (slot.op & 3)
                    assert address + width <= 0x400000 and address % width == 0
                    page = memory[address >> 8].value
                    word = page.words[(address >> 2) & 63]
                    value = word >> ((address & 3) * 8)
                    if slot.op == 0 or slot.op == 4:
                        # Native aarch64 plain char is unsigned (known R05).
                        value = value & 255
                    elif slot.op == 1 or slot.op == 5:
                        value = value & 65535
                        if slot.op == 1:
                            value = sign_extend(value, 16)
                    effect.state.buffer = value
                    effect.state.phase = 2
                    effect.advance = True
            else:
                effect.complete = True
                effect.value = state.buffer
                effect.advance = True
                effect.state.phase = 0
            result.lanes[i] = effect
    return result


@ac.signal
def broadcast(integer, memory) -> Execution:
    result = memory.value
    alu_results = integer.value
    for i in range(4):
        result.lanes[i] = alu_results.lanes[i]
    return result


@ac.signal
def retire(rob, head) -> Retirement:
    result = Retirement(rob=head.value)
    if rob[result.rob - 1].empty():
        return result
    entry = rob[result.rob - 1].value
    if not entry.ready:
        return result
    result.valid = True
    result.entry = entry
    result.next_pc = entry.pc + 4
    op = entry.ins.opcode
    if op == 0x23:
        result.store = True
    elif op == 0x63:
        result.branch = True
        result.taken = branch_taken(entry.ins.funct3, entry.value)
        result.next_pc = entry.dest + (entry.ins.imm if result.taken else ac.u32(4))
        result.taken = result.next_pc != entry.dest + 4
        result.flush = result.next_pc != entry.predicted
    elif op == 0x67:
        result.flush = True
        result.next_pc = entry.value
    else:
        result.write = True
        if op == 0x6f and (entry.ins.word & 127) != 0x67:
            result.next_pc = entry.pc + entry.ins.imm
    return result
