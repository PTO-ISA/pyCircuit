"""State and combinational ports for the pinned skyzh microarchitecture."""
from pycircuit import ac

ROB_SLOTS = 8
STATIONS = 10
MEMORY_PAGES = 16384
ADD = 233
SUB = 234
SLT = 235
SLTU = 236
XOR = 237
OR = 238
AND = 239
SLL = 240
SRL = 241
SRA = 242


class MemoryPage:
    words: ac.array[ac.u32, 64]


class PredictorPage:
    history: ac.array[ac.u8, 256]
    counters: ac.array[ac.u8, 256]


class Instruction:
    word: ac.u32 = 0
    opcode: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0
    funct3: ac.u32 = 0
    funct7: ac.u32 = 0
    imm: ac.u32 = 0


class Operand:
    tag: ac.u32 = 0
    value: ac.u32 = 0


class OperandView:
    registers: ac.array[Operand, 32]


class Station:
    op: ac.u32 = 0
    left: Operand
    right: Operand
    address: ac.u32 = 0
    rob: ac.u32 = 0
    pc: ac.u32 = 0


class ROBEntry:
    ins: Instruction
    pc: ac.u32 = 0
    dest: ac.u32 = 0
    value: ac.u32 = 0
    predicted: ac.u32 = 0
    ready: bool = False


class RenameEntry:
    tag: ac.u32 = 0
    busy: bool = False


class LSUState:
    phase: ac.u32 = 0
    buffer: ac.u32 = 0


class Allocation:
    valid: bool = False
    station: ac.u32 = 0
    rob: ac.u32 = 0
    slot: Station
    entry: ROBEntry


class Dispatch:
    count: ac.u32 = 0
    allocations: ac.array[Allocation, 2]
    next_pc: ac.u32 = 0
    next_tail: ac.u32 = 1
    rename: bool = False
    rd: ac.u32 = 0
    rename_tag: ac.u32 = 0


class CompletionLane:
    complete: bool = False
    rob: ac.u32 = 0
    value: ac.u32 = 0
    address_valid: bool = False
    address: ac.u32 = 0


class Completion:
    lanes: ac.array[CompletionLane, 10]


class Retirement:
    valid: bool = False
    rob: ac.u32 = 0
    entry: ROBEntry
    write: bool = False
    store: bool = False
    branch: bool = False
    taken: bool = False
    flush: bool = False
    next_pc: ac.u32 = 0
