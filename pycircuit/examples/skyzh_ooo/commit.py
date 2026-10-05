"""Retirement and redirect decisions from the old ROB head."""
from .types import ac, Retirement
from .logic import branch_taken


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


@ac.module
def CommitControl(rob, head):
    retirement = retire(rob, head)
    return retirement
