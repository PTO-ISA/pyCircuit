"""Five-stage, in-order RV32I subset with a conservative RAW/WAW scoreboard.

FIFO stage boundaries consume only old queued data. Cell updates become visible
next cycle. Decode and writeback deliberately serialize scoreboard writes.
"""
from pycircuit import ac
from isa import ADD, SUB, LOAD, STORE, Fetched, Decoded, Operands, Executed, Retired, decode

@ac.module
class RegisterFile:

    def __init__() -> None:
        registers = ac.cell[ac.array[ac.u32, 32]](initial=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
        pending = ac.cell[ac.array[bool, 32]](initial=[False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False, False])

    @ac.rule
    def ready(inst: Decoded) -> bool:
        return not pending[inst.rs1] and (not pending[inst.rs2]) and (not inst.writes or not pending[inst.rd])

    @ac.rule
    def read(index: ac.u32) -> ac.u32:
        if index == 0:
            return 0
        return registers[index]

    @ac.rule
    def reserve(inst: Decoded) -> None:
        if inst.writes and inst.rd != 0:
            pending[inst.rd] = True

    @ac.rule
    def commit(inst: Retired) -> None:
        if inst.writes and inst.rd != 0:
            registers[inst.rd] = inst.value
            pending[inst.rd] = False

@ac.module
class DataMemory:

    def __init__() -> None:
        words = ac.cell[ac.array[ac.u32, 16]](initial=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])

    @ac.rule
    def load(address: ac.u32) -> ac.u32:
        assert address & 3 == 0 and address < 64
        return words[address >> 2]

    @ac.rule
    def store(address: ac.u32, value: ac.u32) -> None:
        assert address & 3 == 0 and address < 64
        words[address >> 2] = value

@ac.module
class Fetch:

    def __init__() -> None:
        rom = ac.cell[ac.array[ac.u32, 6]](initial=[7340179, 5243155, 2130355, 3153955, 8707, 1075970739])
        pc = ac.cell[ac.u32](initial=0)
        out = ac.queue[Fetched]()

    @ac.rule
    def fetch(current_pc: ac.u32) -> ac.queue[Fetched]:
        word = rom[current_pc >> 2]
        pc = current_pc + 4
        return Fetched(current_pc, word)

    def __call__() -> ac.queue[Fetched]:
        current_pc = pc
        if current_pc < 24:
            out = fetch(current_pc)
        return out

@ac.module
class DecodeStage:

    def __init__() -> None:
        out = ac.queue[Operands]()

    @ac.rule
    def read_operands(message: ac.queue[Fetched], decoded: Decoded) -> ac.queue[Operands]:
        consumed = message
        lhs = rf.read(decoded.rs1)
        store_value = rf.read(decoded.rs2)
        rhs = decoded.immediate if decoded.immediate_rhs else store_value
        rf.reserve(decoded)
        return Operands(decoded.pc, decoded.word, decoded.kind, decoded.rd, lhs, rhs, store_value, decoded.writes)

    def __call__(source: ac.queue[Fetched], rf: RegisterFile, retirement: ac.queue[Retired]) -> ac.queue[Operands]:
        inst = decode(source)
        enabled = retirement.empty() and rf.ready(inst)
        if enabled:
            out = read_operands(source, inst)
        return out

@ac.module
class Execute:

    def __init__() -> None:
        out = ac.queue[Executed]()

    @ac.rule
    def execute(message: ac.queue[Operands]) -> ac.queue[Executed]:
        inst = message
        result = inst.lhs - inst.rhs if inst.kind == SUB else inst.lhs + inst.rhs
        return Executed(inst.pc, inst.word, inst.kind, inst.rd, result, inst.store_value, inst.writes)

    def __call__(source: ac.queue[Operands]) -> ac.queue[Executed]:
        out = execute(source)
        return out

@ac.module
class MemoryStage:

    def __init__() -> None:
        pass

    @ac.rule
    def access(message: ac.queue[Executed]) -> ac.queue[Retired]:
        inst = message
        result = inst.value
        if inst.kind == LOAD:
            result = memory.load(inst.value)
        elif inst.kind == STORE:
            memory.store(inst.value, inst.store_value)
        return Retired(inst.pc, inst.word, inst.rd, result, inst.writes)

    def __call__(source: ac.queue[Executed], memory: DataMemory, out: ac.queue[Retired]) -> ac.queue[Retired]:
        out = access(source)
        return out

@ac.module
class Writeback:

    def __init__() -> None:
        retired = ac.cell[ac.u32](initial=0)
        last_pc = ac.cell[ac.u32](initial=0)

    @ac.rule
    def commit(message: ac.queue[Retired]) -> None:
        inst = message
        rf.commit(inst)
        retired = retired + 1
        last_pc = inst.pc

    def __call__(source: ac.queue[Retired], rf: RegisterFile) -> tuple[ac.cell[ac.u32], ac.cell[ac.u32]]:
        commit(source)
        return (retired, last_pc)

@ac.module
class Pipeline:

    def __init__() -> None:
        rf = RegisterFile()
        memory = DataMemory()
        retirement = ac.queue[Retired]()
        fetch = Fetch()
        decode_stage = DecodeStage()
        execute = Execute()
        memory_stage = MemoryStage()
        writeback = Writeback()

    def __call__() -> tuple[ac.cell[ac.u32], ac.cell[ac.u32]]:
        fetch()
        decode_stage(fetch.outputs, rf, retirement)
        execute(decode_stage.outputs)
        memory_stage(execute.outputs, memory, retirement)
        writeback(retirement, rf)
        return writeback.outputs
