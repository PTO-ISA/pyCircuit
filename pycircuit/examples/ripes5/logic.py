from pycircuit import ac

INVALID = 0
ADD = 1
ADDI = 2
SUB = 3
AND = 4
OR = 5
XOR = 6
SLT = 7
LUI = 8
LW = 9
SW = 10
BEQ = 11
BNE = 12
JAL = 13
JALR = 14
HALT = 15

class Instruction:
    op: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0
    immediate: ac.u32 = 0

class Slot:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    left: ac.u32 = 0
    right: ac.u32 = 0
    result: ac.u32 = 0
    value: ac.u32 = 0
    stalled: bool = False

class ExResult:
    next_slot: Slot
    redirect: bool = False
    forward_a: ac.u32 = 0
    forward_b: ac.u32 = 0

class Event:
    sequence: ac.u64 = 0
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0

class Store:
    sequence: ac.u64 = 0
    address: ac.u32 = 0
    value: ac.u32 = 0

class Forward:
    value: ac.u32 = 0
    source: ac.u32 = 0


def sext(value: ac.u32, width: ac.u32) -> ac.u32:
    sign = 1 << (width - 1)
    return (value ^ sign) - sign


def decode(word: ac.u32) -> Instruction:
    opcode = word & 127
    f3 = (word >> 12) & 7
    f7 = word >> 25
    i = Instruction(rd=(word >> 7) & 31, rs1=(word >> 15) & 31, rs2=(word >> 20) & 31)
    if opcode == 0x33:
        if f3 == 0 and f7 == 0:
            i.op = ADD
        if f3 == 0 and f7 == 32:
            i.op = SUB
        if f3 == 7 and f7 == 0:
            i.op = AND
        if f3 == 6 and f7 == 0:
            i.op = OR
        if f3 == 4 and f7 == 0:
            i.op = XOR
        if f3 == 2 and f7 == 0:
            i.op = SLT
    elif opcode in (0x13, 0x03, 0x67):
        if opcode == 0x13 and f3 == 0:
            i.op = ADDI
        if opcode == 0x03 and f3 == 2:
            i.op = LW
        if opcode == 0x67 and f3 == 0:
            i.op = JALR
        i.immediate = sext(word >> 20, 12)
        i.rs2 = 0
    elif opcode == 0x37:
        i.op = LUI
        i.immediate = word & 0xfffff000
        i.rs1 = 0
        i.rs2 = 0
    elif opcode == 0x23 and f3 == 2:
        i.op = SW
        i.immediate = sext(((word >> 25) << 5) | ((word >> 7) & 31), 12)
        i.rd = 0
    elif opcode == 0x63 and f3 in (0, 1):
        bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11)
        bits |= (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
        i.op = BEQ if f3 == 0 else BNE
        i.immediate = sext(bits, 13)
        i.rd = 0
    elif opcode == 0x6f:
        bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12)
        bits |= (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
        i.op = JAL
        i.immediate = sext(bits, 21)
        i.rs1 = 0
        i.rs2 = 0
    elif word == 0x00100073:
        i.op = HALT
        i.rd = 0
        i.rs1 = 0
        i.rs2 = 0
    return i


def writer(slot: Slot) -> ac.u32:
    ins = decode(slot.word)
    if ins.op in (ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, JAL, JALR):
        return ins.rd
    return 0


def forward(index: ac.u32, original: ac.u32, mem: Slot, wb: Slot) -> Forward:
    if index and index == writer(mem):
        return Forward(mem.result, 1)
    if index and index == writer(wb):
        return Forward(wb.value, 2)
    return Forward(original, 0)


def execute(ex: Slot, mem: Slot, wb: Slot) -> ExResult:
    ins = decode(ex.word)
    fa = forward(ins.rs1, ex.left, mem, wb)
    fb = forward(ins.rs2, ex.right, mem, wb)
    a = fa.value
    b = fb.value
    op = ins.op
    redirect = op in (JAL, JALR) or (op == BEQ and a == b) or (op == BNE and a != b)
    result = ac.u32(0)
    if op == ADD:
        result = a + b
    elif op in (ADDI, LW, SW, JALR):
        # Pinned native Ripes uses ADD without masking JALR bit zero.
        result = a + ins.immediate
    elif op == SUB:
        result = a - b
    elif op == AND:
        result = a & b
    elif op == OR:
        result = a | b
    elif op == XOR:
        result = a ^ b
    elif op == SLT:
        result = ac.u32(ac.i32(a) < ac.i32(b))
    elif op == LUI:
        result = ins.immediate
    elif op in (BEQ, BNE, JAL):
        result = ex.pc + ins.immediate
    ex.result = result
    ex.right = b
    return ExResult(ex, redirect, fa.source, fb.source)


def loadUseStall(id_slot: Slot, ex: Slot) -> bool:
    dec = decode(id_slot.word)
    ins = decode(ex.word)
    return ins.op == LW and ins.rd != 0 and ins.rd in (dec.rs1, dec.rs2)
