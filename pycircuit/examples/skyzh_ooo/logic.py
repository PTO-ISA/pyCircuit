"""RV32I decode and execution helpers, compiled as ordinary ACPy functions."""
from pycircuit import ac

INVALID = 0
ALU = 1
LOAD = 2
STORE = 3
BRANCH = 4
JAL = 5
JALR = 6
LUI = 7
AUIPC = 8
HALT = 9


class Instruction:
    kind: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0
    immediate: ac.u32 = 0
    function: ac.u32 = 0
    immediate_operand: bool = False
    subtract: bool = False
    arithmetic_shift: bool = False
    width: ac.u32 = 0
    unsigned_load: bool = False


class Tag:
    epoch: ac.u64 = 0
    sequence: ac.u64 = 0


class Control:
    epoch: ac.u64 = 1
    stopped: bool = False


class Request:
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    left: ac.u32 = 0
    right: ac.u32 = 0
    predicted_next: ac.u32 = 0
    fault: ac.u32 = 0


class Completion:
    tag: Tag
    value: ac.u32 = 0
    address: ac.u32 = 0
    store_value: ac.u32 = 0
    target: ac.u32 = 0
    fault: ac.u32 = 0
    redirect: bool = False


def sign_extend(value: ac.u32, width: ac.u32) -> ac.u32:
    sign = 1 << (width - 1)
    return (value ^ sign) - sign


def decode(word: ac.u32) -> Instruction:
    opcode = word & 127
    f3 = (word >> 12) & 7
    f7 = word >> 25
    ins = Instruction(rd=(word >> 7) & 31, rs1=(word >> 15) & 31,
                      rs2=(word >> 20) & 31, function=f3)
    if opcode == 0x33:
        if f7 == 0 or (f7 == 32 and f3 in (0, 5)):
            ins.kind = ALU
            ins.subtract = f7 == 32 and f3 == 0
            ins.arithmetic_shift = f7 == 32 and f3 == 5
    elif opcode == 0x13:
        if f3 not in (1, 5) or f7 == 0 or (f7 == 32 and f3 == 5):
            ins.kind = ALU
            ins.immediate_operand = True
            ins.immediate = sign_extend(word >> 20, 12)
            ins.arithmetic_shift = f7 == 32 and f3 == 5
            ins.rs2 = 0
    elif opcode == 3 and f3 in (0, 1, 2, 4, 5):
        ins.kind = LOAD
        ins.immediate = sign_extend(word >> 20, 12)
        ins.width = 1 << (f3 & 3)
        ins.unsigned_load = f3 >= 4
        ins.rs2 = 0
    elif opcode == 0x23 and f3 in (0, 1, 2):
        ins.kind = STORE
        ins.immediate = sign_extend(((word >> 25) << 5) | ((word >> 7) & 31), 12)
        ins.width = 1 << f3
        ins.rd = 0
    elif opcode == 0x63 and f3 in (0, 1, 4, 5, 6, 7):
        bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11)
        bits |= (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
        ins.kind = BRANCH
        ins.immediate = sign_extend(bits, 13)
        ins.rd = 0
    elif opcode in (0x37, 0x17):
        ins.kind = LUI if opcode == 0x37 else AUIPC
        ins.immediate = word & 0xfffff000
        ins.rs1 = 0
        ins.rs2 = 0
    elif opcode == 0x6f:
        bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12)
        bits |= (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
        ins.kind = JAL
        ins.immediate = sign_extend(bits, 21)
        ins.rs1 = 0
        ins.rs2 = 0
    elif opcode == 0x67 and f3 == 0:
        ins.kind = JALR
        ins.immediate = sign_extend(word >> 20, 12)
        ins.rs2 = 0
    elif word == 0x00100073:
        ins = Instruction(kind=HALT)
    return ins


def calculate(request: Request) -> Completion:
    ins = decode(request.word)
    a = request.left
    b = ins.immediate if ins.immediate_operand else request.right
    result = Completion(tag=request.tag, target=request.pc + 4, fault=request.fault)
    if ins.kind == INVALID and result.fault == 0:
        result.fault = 2
    elif ins.kind == ALU:
        f = ins.function
        if f == 0:
            result.value = a - b if ins.subtract else a + b
        elif f == 1:
            result.value = a << (b & 31)
        elif f == 2:
            result.value = ac.u32(ac.i32(a) < ac.i32(b))
        elif f == 3:
            result.value = ac.u32(a < b)
        elif f == 4:
            result.value = a ^ b
        elif f == 5:
            result.value = ac.u32(ac.i32(a) >> ac.i32(b & 31)) if ins.arithmetic_shift else a >> (b & 31)
        elif f == 6:
            result.value = a | b
        else:
            result.value = a & b
    elif ins.kind in (LUI, AUIPC):
        result.value = ins.immediate + (request.pc if ins.kind == AUIPC else 0)
    elif ins.kind in (LOAD, STORE):
        result.address = a + ins.immediate
        result.store_value = request.right
        if result.fault == 0 and result.address % ins.width != 0:
            result.fault = 3
    elif ins.kind == BRANCH:
        f = ins.function
        taken = (f == 0 and a == b) or (f == 1 and a != b)
        taken = taken or (f == 4 and ac.i32(a) < ac.i32(b)) or (f == 5 and ac.i32(a) >= ac.i32(b))
        taken = taken or (f == 6 and a < b) or (f == 7 and a >= b)
        if taken:
            result.target = request.pc + ins.immediate
    elif ins.kind in (JAL, JALR):
        result.value = request.pc + 4
        result.target = request.pc + ins.immediate if ins.kind == JAL else (a + ins.immediate) & 0xfffffffe
    result.redirect = result.target != request.predicted_next
    return result


def load_value(word: ac.u32, address: ac.u32, width: ac.u32, is_unsigned: bool) -> ac.u32:
    value = word >> ((address & 3) * 8)
    if width == 1:
        value = value & 255
        return value if is_unsigned else sign_extend(value, 8)
    if width == 2:
        value = value & 65535
        return value if is_unsigned else sign_extend(value, 16)
    return value


def store_value(word: ac.u32, address: ac.u32, width: ac.u32, value: ac.u32) -> ac.u32:
    if width == 4:
        return value
    shift = (address & 3) * 8
    mask = (255 if width == 1 else 65535) << shift
    return (word & ~mask) | ((value << shift) & mask)
