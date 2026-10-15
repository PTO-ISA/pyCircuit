"""RV32I datapath values and pure combinational operations.

This example implements integer operations, branches, jumps, LW and SW.
ECALL/CSR, compressed instructions and byte/halfword memory are outside its ISA.
All integer values use u32 bit patterns; no Python runtime executes the model.
"""
from pycircuit import ac

Program = ac.array(ac.u32, shape=(256,))
Registers = ac.array(ac.u32, shape=(32,))
DataWords = ac.array(ac.u32, shape=(64,))


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


class Fetched:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0


class Decoded:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    instruction: Instruction
    lhs: ac.u32 = 0
    rhs: ac.u32 = 0
    stalled: bool = False


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


class Completed:
    valid: bool = False
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    writes: bool = False
    value: ac.u32 = 0
    stalled: bool = False


class Control:
    stall: bool = False
    redirect: bool = False
    target: ac.u32 = 0
    forward_a: ac.u32 = 0
    forward_b: ac.u32 = 0


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


def sign_extend(value: ac.u32, bits: ac.u32) -> ac.u32:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def decode(word: ac.u32) -> Instruction:
    instruction = Instruction()
    if word != 0:
        opcode = word & 127
        instruction.opcode = opcode
        instruction.funct3 = word >> 12 & 7
        instruction.funct7 = word >> 25
        instruction.rd = word >> 7 & 31
        instruction.rs1 = word >> 15 & 31
        if opcode == 51 or opcode == 35 or opcode == 99:
            instruction.rs2 = word >> 20 & 31
        instruction.writes = opcode == 51 or opcode == 19 or opcode == 3 or opcode == 55 or opcode == 23 or opcode == 111 or opcode == 103
        instruction.load = opcode == 3
        instruction.store = opcode == 35
        if opcode == 55 or opcode == 23:
            instruction.rs1 = 0
            instruction.immediate = word & 4294963200
        elif opcode == 111:
            instruction.rs1 = 0
            immediate = (word >> 31 << 20) | ((word >> 12 & 255) << 12) | ((word >> 20 & 1) << 11) | ((word >> 21 & 1023) << 1)
            instruction.immediate = sign_extend(immediate, 21)
        elif opcode == 99:
            immediate = (word >> 31 << 12) | ((word >> 7 & 1) << 11) | ((word >> 25 & 63) << 5) | ((word >> 8 & 15) << 1)
            instruction.immediate = sign_extend(immediate, 13)
        elif opcode == 35:
            instruction.immediate = sign_extend((word >> 25 << 5) | (word >> 7 & 31), 12)
        elif opcode == 19 or opcode == 3 or opcode == 103:
            instruction.immediate = sign_extend(word >> 20, 12)
        else:
            assert opcode == 51
        if instruction.load or instruction.store:
            assert instruction.funct3 == 2
        if opcode == 51:
            assert instruction.funct7 == 0 or (instruction.funct7 == 32 and (instruction.funct3 == 0 or instruction.funct3 == 5))
        if opcode == 103:
            assert instruction.funct3 == 0
        if opcode == 19 and instruction.funct3 == 1:
            assert instruction.funct7 == 0
        if opcode == 19 and instruction.funct3 == 5:
            assert instruction.funct7 == 0 or instruction.funct7 == 32
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
    if opcode == 3 or opcode == 35 or opcode == 103:
        return lhs + instruction.immediate
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
        if memory.writes and memory.rd == index:
            return 1
        if writeback.writes and writeback.rd == index:
            return 2
    return 0


def forwarded(original: ac.u32, source: ac.u32, memory: Executed, writeback: Completed) -> ac.u32:
    if source == 1:
        return memory.alu
    if source == 2:
        return writeback.value
    return original
