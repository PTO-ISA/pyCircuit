"""Fixed immutable records: ordinary value structs in generated C++."""

from typing import NamedTuple


INVALID, ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, SW, BEQ, BNE, JAL, JALR, HALT = range(16)
NO_MEMORY, LOAD, STORE = range(3)


class ModelError(RuntimeError):
    pass


class Instruction(NamedTuple):
    word: int = 0
    op: int = INVALID
    rd: int = 0
    rs1: int = 0
    rs2: int = 0
    immediate: int = 0


class FrontControl(NamedTuple):
    epoch: int = 0
    stopped: bool = False


class Redirect(NamedTuple):
    pc: int
    epoch: int


class Fetched(NamedTuple):
    pc: int
    word: int
    epoch: int


class Decoded(NamedTuple):
    pc: int
    instruction: Instruction
    epoch: int
    left: int
    right: int


class Executed(NamedTuple):
    pc: int = 0
    word: int = 0
    rd: int = 0
    result: int = 0
    memory: int = NO_MEMORY
    address: int = 0
    store_data: int = 0
    halt: bool = False


class Busy(NamedTuple):
    valid: bool = False
    due: int = 0
    instruction: Executed = Executed()


class Completed(NamedTuple):
    pc: int = 0
    word: int = 0
    rd: int = 0
    value: int = 0
    store: bool = False
    address: int = 0
    data: int = 0
    halt: bool = False


class Retired(NamedTuple):
    sequence: int = 0
    instruction: Completed = Completed()


def u32(value):
    return value & 0xffffffff


def signed(value):
    return value - (1 << 32) if value & (1 << 31) else value
