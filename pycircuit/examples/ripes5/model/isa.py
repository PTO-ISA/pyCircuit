"""RV32IMC payloads and combinational integer datapath (bit-pattern values)."""
from pycircuit import ac

Program = ac.array(ac.u32, shape=(1024,))
Registers = ac.array(ac.u32, shape=(32,))
DataWords = ac.array(ac.u32, shape=(1024,))


class Instruction:
    opcode: ac.u32 = 0
    funct3: ac.u32 = 0
    funct7: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0
    immediate: ac.u32 = 0
    writes: bool = False
    load: bool = False
    store: bool = False
    illegal: bool = False
    system: ac.u32 = 0


class Fetched:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    expanded: ac.u32 = 0
    next_pc: ac.u32 = 0
    fetch_fault: bool = False


class Decoded:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    instruction: Instruction
    lhs: ac.u32 = 0
    rhs: ac.u32 = 0
    stalled: bool = False
    next_pc: ac.u32 = 0
    expanded: ac.u32 = 0
    fetch_fault: bool = False


class Executed:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    writes: bool = False
    load: bool = False
    store: bool = False
    alu: ac.u32 = 0
    store_value: ac.u32 = 0
    value: ac.u32 = 0
    stalled: bool = False
    funct3: ac.u32 = 0
    illegal: bool = False
    system: ac.u32 = 0
    expanded: ac.u32 = 0
    next_pc: ac.u32 = 0


class Completed:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    writes: bool = False
    value: ac.u32 = 0
    stalled: bool = False
    illegal: bool = False
    system: ac.u32 = 0
    expanded: ac.u32 = 0
    next_pc: ac.u32 = 0


class Control:
    stall: bool = False
    redirect: bool = False
    target: ac.u32 = 0
    forward_a: ac.u32 = 0
    forward_b: ac.u32 = 0
    ecall_stall: bool = False
    stop: bool = False


class ForwardedOperands:
    lhs: ac.u32 = 0
    rhs: ac.u32 = 0
    source_a: ac.u32 = 0
    source_b: ac.u32 = 0


class ExecutionResult:
    packet: Executed
    redirect: bool = False
    target: ac.u32 = 0


class Retirement:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    writes: bool = False
    value: ac.u32 = 0


class StoreEvent:
    valid: bool = False
    address: ac.u32 = 0
    value: ac.u32 = 0
    size: ac.u32 = 0


class SystemEvent:
    valid: bool = False
    pc: ac.u32 = 0
    kind: ac.u32 = 0
    number: ac.u32 = 0
    argument: ac.u32 = 0


class Expansion:
    word: ac.u32 = 0
    length: ac.u32 = 4



def sign_extend(value: ac.u32, bits: ac.u32) -> ac.u32:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def decode(word: ac.u32, multiply: bool) -> Instruction:
    instruction = Instruction()
    opcode = word & 127
    function = word >> 12 & 7
    upper = word >> 25
    instruction.opcode = opcode
    instruction.funct3 = function
    instruction.funct7 = upper
    instruction.rd = word >> 7 & 31
    instruction.rs1 = word >> 15 & 31
    if opcode == 51 or opcode == 35 or opcode == 99:
        instruction.rs2 = word >> 20 & 31
    legal = False
    if opcode == 51:
        legal = upper == 0 or (upper == 32 and (function == 0 or function == 5)) or (upper == 1 and multiply)
    elif opcode == 19:
        legal = (function != 1 and function != 5) or (function == 1 and upper == 0) or (function == 5 and (upper == 0 or upper == 32))
        instruction.immediate = sign_extend(word >> 20, 12)
    elif opcode == 3 or opcode == 103:
        legal = (function == 0 or function == 1 or function == 2 or function == 4 or function == 5) if opcode == 3 else function == 0
        instruction.immediate = sign_extend(word >> 20, 12)
    elif opcode == 35:
        legal = function <= 2
        instruction.immediate = sign_extend((word >> 25 << 5) | (word >> 7 & 31), 12)
    elif opcode == 99:
        legal = function == 0 or function == 1 or function >= 4
        immediate = (word >> 31 << 12) | ((word >> 7 & 1) << 11) | ((word >> 25 & 63) << 5) | ((word >> 8 & 15) << 1)
        instruction.immediate = sign_extend(immediate, 13)
    elif opcode == 55 or opcode == 23:
        legal = True
        instruction.rs1 = 0
        instruction.immediate = word & 4294963200
    elif opcode == 111:
        legal = True
        instruction.rs1 = 0
        immediate = (word >> 31 << 20) | ((word >> 12 & 255) << 12) | ((word >> 20 & 1) << 11) | ((word >> 21 & 1023) << 1)
        instruction.immediate = sign_extend(immediate, 21)
    elif opcode == 15:
        # FENCE rd/rs1 and unused mode bits are reserved for forward compatibility.
        legal = function == 0
        instruction.rs1 = 0
    elif opcode == 115:
        legal = word == 115 or word == 1048691
        instruction.rs1 = 0
        instruction.system = 1 if word == 115 else 2
    instruction.illegal = not legal
    instruction.writes = legal and (opcode == 51 or opcode == 19 or opcode == 3 or opcode == 55 or opcode == 23 or opcode == 111 or opcode == 103)
    instruction.load = legal and opcode == 3
    instruction.store = legal and opcode == 35
    return instruction


def less_signed(lhs: ac.u32, rhs: ac.u32) -> bool:
    return (lhs ^ 2147483648) < (rhs ^ 2147483648)


def arithmetic_shift(value: ac.u32, amount: ac.u32) -> ac.u32:
    result = value >> amount
    if amount != 0 and (value & 2147483648) != 0:
        result = result | (4294967295 << (32 - amount))
    return result


def alu(instruction: Instruction, pc: ac.u32, lhs: ac.u32, rhs: ac.u32) -> ac.u32:
    opcode = instruction.opcode
    if opcode == 55:
        return instruction.immediate
    if opcode == 23 or opcode == 111 or opcode == 99:
        return pc + instruction.immediate
    if opcode == 103:
        return (lhs + instruction.immediate) & 4294967294
    if opcode == 3 or opcode == 35:
        return lhs + instruction.immediate
    if opcode == 51 and instruction.funct7 == 1:
        return multiply_divide(instruction.funct3, lhs, rhs)
    if opcode == 51 or opcode == 19:
        operand = instruction.immediate if opcode == 19 else rhs
        function = instruction.funct3
        if function == 0:
            return lhs - operand if opcode == 51 and instruction.funct7 == 32 else lhs + operand
        if function == 1:
            return lhs << (operand & 31)
        if function == 2:
            return 1 if less_signed(lhs, operand) else 0
        if function == 3:
            return 1 if lhs < operand else 0
        if function == 4:
            return lhs ^ operand
        if function == 5:
            return arithmetic_shift(lhs, operand & 31) if instruction.funct7 == 32 else lhs >> (operand & 31)
        if function == 6:
            return lhs | operand
        return lhs & operand
    return 0


def branch_taken(instruction: Instruction, lhs: ac.u32, rhs: ac.u32) -> bool:
    if instruction.opcode != 99:
        return False
    function = instruction.funct3
    if function == 0:
        return lhs == rhs
    if function == 1:
        return lhs != rhs
    if function == 4:
        return less_signed(lhs, rhs)
    if function == 5:
        return not less_signed(lhs, rhs)
    if function == 6:
        return lhs < rhs
    assert function == 7
    return lhs >= rhs


def forwarding(index: ac.u32, memory: Executed, writeback: Completed) -> ac.u32:
    if index != 0:
        if memory.valid and memory.writes and memory.rd == index:
            return 1
        if writeback.valid and writeback.writes and writeback.rd == index:
            return 2
    return 0


def forwarded(original: ac.u32, source: ac.u32, memory: Executed, writeback: Completed) -> ac.u32:
    if source == 1:
        return memory.alu
    if source == 2:
        return writeback.value
    return original


def multiply_divide(function: ac.u32, lhs: ac.u32, rhs: ac.u32) -> ac.u32:
    if function <= 3:
        wide = ac.u64(lhs) * ac.u64(rhs)
        high = ac.u32(wide >> 32)
        if function == 0:
            return ac.u32(wide)
        if (function == 1 or function == 2) and (lhs & 2147483648) != 0:
            high = high - rhs
        if function == 1 and (rhs & 2147483648) != 0:
            high = high - lhs
        return high
    if rhs == 0:
        return 4294967295 if function == 4 or function == 5 else lhs
    if function == 5:
        return lhs // rhs
    if function == 7:
        return lhs % rhs
    # Unsigned magnitudes implement truncation toward zero, including MIN/-1,
    # without ever evaluating an overflowing signed host divide.
    negative_a = (lhs & 2147483648) != 0
    negative_b = (rhs & 2147483648) != 0
    a = 0 - lhs if negative_a else lhs
    b = 0 - rhs if negative_b else rhs
    if function == 4:
        quotient = a // b
        return 0 - quotient if negative_a != negative_b else quotient
    remainder = a % b
    return 0 - remainder if negative_a else remainder


def encode_i(op: ac.u32, rd: ac.u32, f: ac.u32, rs: ac.u32, imm: ac.u32) -> ac.u32:
    return ((imm & 4095) << 20) | (rs << 15) | (f << 12) | (rd << 7) | op


def encode_r(rd: ac.u32, f: ac.u32, rs: ac.u32, rt: ac.u32, upper: ac.u32) -> ac.u32:
    return (upper << 25) | (rt << 20) | (rs << 15) | (f << 12) | (rd << 7) | 51


def encode_s(rs: ac.u32, rt: ac.u32, imm: ac.u32) -> ac.u32:
    return ((imm >> 5 & 127) << 25) | (rt << 20) | (rs << 15) | 8192 | ((imm & 31) << 7) | 35


def encode_j(rd: ac.u32, imm: ac.u32) -> ac.u32:
    return ((imm >> 20 & 1) << 31) | ((imm >> 1 & 1023) << 21) | ((imm >> 11 & 1) << 20) | (imm & 1044480) | (rd << 7) | 111


def encode_b(rs: ac.u32, f: ac.u32, imm: ac.u32) -> ac.u32:
    return ((imm >> 12 & 1) << 31) | ((imm >> 5 & 63) << 25) | (rs << 15) | (f << 12) | ((imm >> 1 & 15) << 8) | ((imm >> 11 & 1) << 7) | 99


def decompress(raw: ac.u32, compressed: bool) -> Expansion:
    if (raw & 3) == 3 or not compressed:
        return Expansion(raw, 4)
    h = raw & 65535
    quadrant = h & 3
    function = h >> 13
    rd = h >> 7 & 31
    rt = h >> 2 & 31
    rs_prime = 8 + (h >> 7 & 7)
    rt_prime = 8 + (h >> 2 & 7)
    immediate = sign_extend(((h >> 12 & 1) << 5) | (h >> 2 & 31), 6)
    word: ac.u32 = 0
    if quadrant == 0:
        if function == 0:
            imm = ((h >> 7 & 15) << 6) | ((h >> 11 & 3) << 4) | ((h >> 5 & 1) << 3) | ((h >> 6 & 1) << 2)
            if imm != 0:
                word = encode_i(19, rt_prime, 0, 2, imm)
        elif function == 2 or function == 6:
            imm = ((h >> 5 & 1) << 6) | ((h >> 10 & 7) << 3) | ((h >> 6 & 1) << 2)
            word = encode_i(3, rt_prime, 2, rs_prime, imm) if function == 2 else encode_s(rs_prime, rt_prime, imm)
    elif quadrant == 1:
        if function == 0:
            word = encode_i(19, rd, 0, rd, immediate)
        elif function == 1 or function == 5:
            imm = ((h >> 12 & 1) << 11) | ((h >> 11 & 1) << 4) | ((h >> 9 & 3) << 8) | ((h >> 8 & 1) << 10) | ((h >> 7 & 1) << 6) | ((h >> 6 & 1) << 7) | ((h >> 3 & 7) << 1) | ((h >> 2 & 1) << 5)
            word = encode_j(1 if function == 1 else 0, sign_extend(imm, 12))
        elif function == 2:
            word = encode_i(19, rd, 0, 0, immediate)
        elif function == 3:
            if rd == 2:
                imm = ((h >> 12 & 1) << 9) | ((h >> 6 & 1) << 4) | ((h >> 5 & 1) << 6) | ((h >> 3 & 3) << 7) | ((h >> 2 & 1) << 5)
                if imm != 0:
                    word = encode_i(19, 2, 0, 2, sign_extend(imm, 10))
            elif immediate != 0:
                word = (immediate << 12) | (rd << 7) | 55
        elif function == 4:
            operation = h >> 10 & 3
            if operation == 0 or operation == 1:
                if (h & 4096) == 0:
                    word = encode_i(19, rs_prime, 5, rs_prime, (h >> 2 & 31) | (1024 if operation == 1 else 0))
            elif operation == 2:
                word = encode_i(19, rs_prime, 7, rs_prime, immediate)
            elif (h & 4096) == 0:
                sub = h >> 5 & 3
                f = 0 if sub == 0 else 4 if sub == 1 else 6 if sub == 2 else 7
                word = encode_r(rs_prime, f, rs_prime, rt_prime, 32 if sub == 0 else 0)
        elif function == 6 or function == 7:
            imm = ((h >> 12 & 1) << 8) | ((h >> 10 & 3) << 3) | ((h >> 5 & 3) << 6) | ((h >> 3 & 3) << 1) | ((h >> 2 & 1) << 5)
            word = encode_b(rs_prime, 0 if function == 6 else 1, sign_extend(imm, 9))
    elif quadrant == 2:
        if function == 0:
            if (h & 4096) == 0:
                word = encode_i(19, rd, 1, rd, h >> 2 & 31)
        elif function == 2:
            if rd != 0:
                imm = ((h >> 12 & 1) << 5) | ((h >> 4 & 7) << 2) | ((h >> 2 & 3) << 6)
                word = encode_i(3, rd, 2, 2, imm)
        elif function == 4:
            if (h & 4096) == 0:
                if rt == 0:
                    if rd != 0:
                        word = encode_i(103, 0, 0, rd, 0)
                else:
                    word = encode_r(rd, 0, 0, rt, 0)
            elif rt == 0:
                word = 1048691 if rd == 0 else encode_i(103, 1, 0, rd, 0)
            else:
                word = encode_r(rd, 0, rd, rt, 0)
        elif function == 6:
            imm = ((h >> 9 & 15) << 2) | ((h >> 7 & 3) << 6)
            word = encode_s(2, rt, imm)
    return Expansion(word, 2)


def supported_ecall(number: ac.u32) -> bool:
    return number == 1 or number == 4 or number == 11 or number == 10 or number == 93
