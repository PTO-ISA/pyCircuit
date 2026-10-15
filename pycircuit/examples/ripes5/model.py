"""Ripes RV5S stage timing expressed with Modules, Rules and Cells.

The top declares stage registers and binds Stage return values to these Cells.
Stage inputs observe old state; accepted outputs update the fixed destination
at the clock edge. Explicit bypasses model combinational forwarding.
"""
from pycircuit import ac
from isa import Program, Registers, DataWords, Instruction, Fetched, Decoded, Executed, Completed, Control, ForwardedOperands, ExecutionResult, Retirement, StoreEvent
from isa import decode, alu, branch_taken, forwarding, forwarded


@ac.module
class RegisterFile:
    def __init__(initial: Registers) -> None:
        registers: ac.array(ac.cell[ac.u32], shape=(32,)) = [
            ac.cell[ac.u32](initial=initial[i]) for i in range(32)
        ]

    @ac.rule
    def read(index: ac.u32, writeback: Completed) -> ac.u32:
        if index == 0:
            return 0
        if writeback.writes and writeback.rd == index:
            return writeback.value
        return registers[index]

    @ac.rule
    def write(index: ac.u32, value: ac.u32) -> None:
        if index != 0:
            registers[index] = value


@ac.module
class Fetch:
    def __init__(program: Program) -> None:
        rom = ac.cell[Program](initial=program)
        pc = ac.cell[ac.u32](initial=0)

    @ac.rule
    def advance(control: Control) -> ac.cell[Fetched]:
        if control.stall:
            return None
        if control.redirect:
            pc = control.target
            return Fetched()
        assert pc & 3 == 0
        packet = Fetched(True, pc, rom[pc >> 2])
        pc = pc + 4
        return packet

    def __call__(control: Control) -> ac.cell[Fetched]:
        return advance(control)


@ac.module
class Decode:
    def __init__() -> None:
        instruction = ac.signal[Instruction]()

    @ac.rule
    def decode_instruction(source: ac.cell[Fetched]) -> None:
        instruction = decode(source.word)

    @ac.rule
    def inspect() -> Instruction:
        return instruction

    @ac.rule
    def advance(source: ac.cell[Fetched], control: Control,
                writeback: ac.cell[Completed]) -> ac.cell[Decoded]:
        if control.stall or control.redirect:
            bubble = Decoded()
            bubble.stalled = control.stall
            return bubble
        packet = source
        completed = writeback
        decoded = inspect()
        lhs = rf.read(decoded.rs1, completed)
        rhs = rf.read(decoded.rs2, completed)
        return Decoded(packet.valid, packet.pc, packet.word, decoded, lhs, rhs, False)

    def __call__(source: ac.cell[Fetched], control: Control,
                 rf: RegisterFile, writeback: ac.cell[Completed]) -> ac.cell[Decoded]:
        decode_instruction(source)
        return advance(source, control, writeback)


@ac.module
class HazardUnit:
    def __init__() -> None:
        pass

    @ac.rule
    def load_use(source: ac.cell[Decoded], instruction: Instruction) -> bool:
        executing = source.instruction
        return executing.load and executing.rd != 0 and (
            executing.rd == instruction.rs1 or executing.rd == instruction.rs2)


@ac.module
class ForwardingUnit:
    def __init__() -> None:
        pass

    @ac.rule
    def resolve(source: ac.cell[Decoded], older: ac.cell[Executed],
                retiring: ac.cell[Completed]) -> ForwardedOperands:
        packet = source
        memory = older
        writeback = retiring
        source_a = forwarding(packet.instruction.rs1, memory, writeback)
        source_b = forwarding(packet.instruction.rs2, memory, writeback)
        lhs = forwarded(packet.lhs, source_a, memory, writeback)
        rhs = forwarded(packet.rhs, source_b, memory, writeback)
        return ForwardedOperands(lhs, rhs, source_a, source_b)


@ac.module
class Execute:
    def __init__() -> None:
        pass

    @ac.rule
    def resolve(source: ac.cell[Decoded], operands: ForwardedOperands) -> ExecutionResult:
        packet = source
        executing = packet.instruction
        lhs = operands.lhs
        rhs = operands.rhs
        result = alu(executing, packet.pc, lhs, rhs)
        jump = executing.opcode == 111 or executing.opcode == 103
        redirect = jump or branch_taken(executing, lhs, rhs)
        value = packet.pc + 4 if jump else result
        next_packet = Executed(packet.valid, packet.pc, packet.word, executing.rd, executing.writes,
                               executing.load, executing.store, result, rhs, value, packet.stalled)
        return ExecutionResult(next_packet, redirect, result if redirect else 0)

    @ac.rule
    def advance(packet: Executed) -> ac.cell[Executed]:
        return packet

    def __call__(packet: Executed) -> ac.cell[Executed]:
        return advance(packet)


@ac.module
class Memory:
    def __init__(initial: DataWords, base: ac.u32) -> None:
        words: ac.array(ac.cell[ac.u32], shape=(64,)) = [
            ac.cell[ac.u32](initial=initial[i]) for i in range(64)
        ]
        store_event = ac.cell[StoreEvent](initial=StoreEvent())

    @ac.rule
    def advance(source: ac.cell[Executed]) -> ac.cell[Completed]:
        packet = source
        value = packet.value
        event = StoreEvent()
        if packet.load or packet.store:
            assert packet.alu & 3 == 0 and packet.alu >= base and packet.alu - base < 256
            index = (packet.alu - base) >> 2
            if packet.load:
                value = words[index]
            if packet.store:
                words[index] = packet.store_value
                event = StoreEvent(True, packet.alu, packet.store_value)
        store_event = event
        return Completed(packet.valid, packet.pc, packet.word, packet.rd, packet.writes, value, packet.stalled)

    def __call__(source: ac.cell[Executed]) -> ac.cell[Completed]:
        return advance(source)


@ac.module
class Writeback:
    def __init__() -> None:
        retired = ac.cell[ac.u32](initial=0)
        retirement = ac.cell[Retirement](initial=Retirement())

    @ac.rule
    def advance(source: ac.cell[Completed], code_size: ac.u32) -> None:
        packet = source
        event = Retirement()
        if packet.writes:
            rf.write(packet.rd, packet.value)
        if packet.valid and packet.pc < code_size:
            retired = retired + 1
            event = Retirement(True, packet.pc, packet.word, packet.rd, packet.writes and packet.rd != 0, packet.value)
        retirement = event

    def __call__(source: ac.cell[Completed], rf: RegisterFile, code_size: ac.u32) -> None:
        advance(source, code_size)


@ac.module
class Ripes5:
    def __init__(program: Program, initial_registers: Registers, initial_data: DataWords,
                 data_base: ac.u32, code_size: ac.u32) -> None:
        rf = RegisterFile(initial_registers)
        if_id = ac.cell[Fetched](initial=Fetched())
        id_ex = ac.cell[Decoded](initial=Decoded())
        ex_mem = ac.cell[Executed](initial=Executed())
        mem_wb = ac.cell[Completed](initial=Completed())
        fetch = Fetch(program)
        decode_stage = Decode()
        hazard_unit = HazardUnit()
        forwarding_unit = ForwardingUnit()
        execute = Execute()
        memory = Memory(initial_data, data_base)
        writeback = Writeback()
        last_control = ac.cell[Control](initial=Control())

    @ac.rule
    def observe(control: Control) -> None:
        last_control = control

    def __call__() -> None:
        # Independent combinational units observe the stage registers' old values.
        instruction = decode_stage.inspect()
        stall = hazard_unit.load_use(id_ex, instruction)
        operands = forwarding_unit.resolve(id_ex, ex_mem, mem_wb)
        result = execute.resolve(id_ex, operands)
        control = Control(stall, result.redirect, result.target, operands.source_a, operands.source_b)

        # These bindings update existing Cells at the edge; inputs still read old values.
        if_id = fetch(control)
        id_ex = decode_stage(if_id, control, rf, mem_wb)
        ex_mem = execute(result.packet)
        mem_wb = memory(ex_mem)
        writeback(mem_wb, rf, code_size)
        observe(control)
