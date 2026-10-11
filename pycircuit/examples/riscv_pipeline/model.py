"""Five-stage, in-order RV32I subset with a conservative RAW/WAW scoreboard.

FIFO stage boundaries consume only old queued data. Cell updates become visible
next cycle. Decode and writeback deliberately serialize scoreboard writes.
"""
from pycircuit import ac
from isa import ADD, SUB, LOAD, STORE, Fetched, Decoded, Operands, Executed, Retired, decode


@ac.module
def RegisterFile():
    registers = ac.cell[ac.array[ac.u32, 32]](initial=[
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ])
    pending = ac.cell[ac.array[bool, 32]](initial=[
        False, False, False, False, False, False, False, False,
        False, False, False, False, False, False, False, False,
        False, False, False, False, False, False, False, False,
        False, False, False, False, False, False, False, False,
    ])

    @ac.method
    def ready(inst: Decoded) -> bool:
        return (not pending.value[inst.rs1]
                and not pending.value[inst.rs2]
                and (not inst.writes or not pending.value[inst.rd]))

    @ac.method
    def read(index: ac.u32) -> ac.u32:
        if index == 0:
            return 0
        return registers.value[index]

    @ac.method
    def reserve(inst: Decoded):
        if inst.writes and inst.rd != 0:
            pending.value[inst.rd] = True

    @ac.method
    def commit(inst: Retired):
        if inst.writes and inst.rd != 0:
            registers.value[inst.rd] = inst.value
            pending.value[inst.rd] = False

    return registers


@ac.module
def DataMemory():
    words = ac.cell[ac.array[ac.u32, 16]](initial=[
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ])

    @ac.method
    def load(address: ac.u32) -> ac.u32:
        assert (address & 3) == 0 and address < 64
        return words.value[address >> 2]

    @ac.method
    def store(address: ac.u32, value: ac.u32):
        assert (address & 3) == 0 and address < 64
        words.value[address >> 2] = value

    return words


@ac.module
def Fetch():
    # addi x1,x0,7; addi x2,x0,5; add x3,x1,x2;
    # sw x3,0(x0); lw x4,0(x0); sub x5,x4,x2.
    rom = ac.cell[ac.array[ac.u32, 6]](initial=[
        0x00700093, 0x00500113, 0x002081b3,
        0x00302023, 0x00002203, 0x402202b3,
    ])
    pc = ac.cell[ac.u32](initial=0)
    out = ac.queue[Fetched]()

    @ac.rule
    def fetch(current_pc: ac.u32) -> ac.queue[Fetched]:
        word = rom.value[current_pc >> 2]
        pc.value = current_pc + 4
        return Fetched(current_pc, word)

    current_pc = pc.value
    if current_pc < 24:
        out = fetch(current_pc)
    return out


@ac.module
def DecodeStage(source: ac.queue[Fetched], rf, retirement: ac.queue[Retired]):
    out = ac.queue[Operands]()

    @ac.rule
    def read_operands(message: ac.queue[Fetched], decoded: Decoded) -> ac.queue[Operands]:
        consumed = message.value
        lhs = rf.read(decoded.rs1)
        store_value = rf.read(decoded.rs2)
        rhs = decoded.immediate if decoded.immediate_rhs else store_value
        rf.reserve(decoded)
        return Operands(decoded.pc, decoded.word, decoded.kind, decoded.rd,
                        lhs, rhs, store_value, decoded.writes)

    # Preparation read readiness also requires old input data to be present.
    # The shared decode observes source; the Rule still consumes its input.
    inst = decode(source.value)
    # Old retirement occupancy separates reserve/commit scoreboard writes.
    enabled = retirement.empty() and rf.ready(inst)
    if enabled:
        out = read_operands(source, inst)
    return out


@ac.module
def Execute(source: ac.queue[Operands]):
    out = ac.queue[Executed]()

    @ac.rule
    def execute(message: ac.queue[Operands]) -> ac.queue[Executed]:
        inst = message.value
        result = inst.lhs - inst.rhs if inst.kind == SUB else inst.lhs + inst.rhs
        return Executed(inst.pc, inst.word, inst.kind, inst.rd,
                        result, inst.store_value, inst.writes)

    out = execute(source)
    return out


@ac.module
def MemoryStage(source: ac.queue[Executed], memory, out: ac.queue[Retired]):

    @ac.rule
    def access(message: ac.queue[Executed]) -> ac.queue[Retired]:
        inst = message.value
        result = inst.value
        if inst.kind == LOAD:
            result = memory.load(inst.value)
        elif inst.kind == STORE:
            memory.store(inst.value, inst.store_value)
        return Retired(inst.pc, inst.word, inst.rd, result, inst.writes)

    out = access(source)
    return out


@ac.module
def Writeback(source: ac.queue[Retired], rf):
    retired = ac.cell[ac.u32](initial=0)
    last_pc = ac.cell[ac.u32](initial=0)

    @ac.rule
    def commit(message: ac.queue[Retired]):
        inst = message.value
        rf.commit(inst)
        retired.value = retired.value + 1
        last_pc.value = inst.pc

    commit(source)
    return retired, last_pc


@ac.module
def Pipeline():
    rf = RegisterFile()
    memory = DataMemory()
    retirement = ac.queue[Retired]()  # Fixed shared output connection, declared first.
    fetch = Fetch()
    decode_stage = DecodeStage(fetch.outputs, rf, retirement)
    execute = Execute(decode_stage.outputs)
    memory_stage = MemoryStage(execute.outputs, memory, retirement)
    writeback = Writeback(retirement, rf)
    return rf.outputs, memory.outputs, writeback.outputs
