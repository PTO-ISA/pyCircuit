"""RV32I subset: ADDI, ADD, SUB, LW and SW; immediates are u32 bit patterns."""
from pycircuit import ac
ADD = 0
SUB = 1
LOAD = 2
STORE = 3

class Fetched:
    pc: ac.u32 = 0
    word: ac.u32 = 0

class Decoded:
    pc: ac.u32 = 0
    word: ac.u32 = 0
    kind: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0
    immediate: ac.u32 = 0
    immediate_rhs: bool = False
    writes: bool = False

class Operands:
    pc: ac.u32 = 0
    word: ac.u32 = 0
    kind: ac.u32 = 0
    rd: ac.u32 = 0
    lhs: ac.u32 = 0
    rhs: ac.u32 = 0
    store_value: ac.u32 = 0
    writes: bool = False

class Executed:
    pc: ac.u32 = 0
    word: ac.u32 = 0
    kind: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0
    store_value: ac.u32 = 0
    writes: bool = False

class Retired:
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0
    writes: bool = False

def sext12(value: ac.u32) -> ac.u32:
    return (value ^ 2048) - 2048

def decode(packet: Fetched) -> Decoded:
    word = packet.word
    opcode = word & 127
    funct3 = word >> 12 & 7
    funct7 = word >> 25
    rd = word >> 7 & 31
    rs1 = word >> 15 & 31
    rs2 = word >> 20 & 31
    kind = ADD
    immediate = 0
    immediate_rhs = False
    writes = True
    if opcode == 19:
        assert funct3 == 0
        rs2 = 0
        immediate = sext12(word >> 20)
        immediate_rhs = True
    elif opcode == 51:
        assert funct3 == 0 and (funct7 == 0 or funct7 == 32)
        if funct7 == 32:
            kind = SUB
    elif opcode == 3:
        assert funct3 == 2
        kind = LOAD
        rs2 = 0
        immediate = sext12(word >> 20)
        immediate_rhs = True
    else:
        assert opcode == 35 and funct3 == 2
        kind = STORE
        rd = 0
        immediate = sext12(word >> 25 << 5 | word >> 7 & 31)
        immediate_rhs = True
        writes = False
    return Decoded(packet.pc, word, kind, rd, rs1, rs2, immediate, immediate_rhs, writes)
