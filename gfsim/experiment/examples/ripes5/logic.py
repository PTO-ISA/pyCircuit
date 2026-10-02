"""Pure combinational logic. Inputs are values, never Queues or proposals."""
from typing import NamedTuple
from ..riscv.isa import decode
from ..riscv.records import (ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW,
                             SW, BEQ, BNE, JAL, JALR, INVALID, u32, signed)

WRITES = (ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, JAL, JALR)


class Slot(NamedTuple):
    valid: bool = False
    pc: int = 0
    word: int = 0
    left: int = 0
    right: int = 0
    result: int = 0
    value: int = 0
    stalled: bool = False


class Event(NamedTuple):
    sequence: int = 0
    pc: int = 0
    word: int = 0
    rd: int = 0
    value: int = 0


class Store(NamedTuple):
    sequence: int = 0
    address: int = 0
    value: int = 0


def writer(slot):
    ins = decode(slot.word)
    return ins.rd if ins.op in WRITES else 0


def forward(index, original, mem, wb):
    # Ripes forwards the MEM ALU output, WB's selected writeback value.
    if index and index == writer(mem):
        return mem.result, 1
    if index and index == writer(wb):
        return wb.value, 2
    return original, 0


def execute(ex, mem, wb):
    ins = decode(ex.word)
    a, fa = forward(ins.rs1, ex.left, mem, wb)
    b, fb = forward(ins.rs2, ex.right, mem, wb)
    op, imm = ins.op, ins.immediate
    redirect = op in (JAL, JALR) or (op == BEQ and a == b) or (op == BNE and a != b)
    if op == ADD:
        result = a + b
    elif op in (ADDI, LW, SW, JALR):
        # The pinned Ripes uses ADD for JALR (no bit-zero masking).
        result = a + imm
    elif op == SUB:
        result = a - b
    elif op == AND:
        result = a & b
    elif op == OR:
        result = a | b
    elif op == XOR:
        result = a ^ b
    elif op == SLT:
        result = int(signed(a) < signed(b))
    elif op == LUI:
        result = imm
    elif op in (BEQ, BNE, JAL):
        result = ex.pc + imm
    elif op == INVALID:
        result = 0
    else:
        raise ValueError(f'unsupported instruction {ex.word:#x}')
    return u32(result), b, bool(redirect), fa, fb


def control(id_slot, ex, mem, wb):
    dec, ins = decode(id_slot.word), decode(ex.word)
    stall = ins.op == LW and ins.rd != 0 and ins.rd in (dec.rs1, dec.rs2)
    target, _, redirect, fa, fb = execute(ex, mem, wb)
    return dict(stall=stall, flush_ifid=redirect, flush_idex=redirect or stall,
                pc_enable=not stall, idex_enable=True, exmem_clear=False,
                target=target if redirect else None, forward_a=fa, forward_b=fb)
