"""Ripes RV5S stage timing expressed with Modules, Rules and Cells.

The top declares stage registers and binds Stage return values to these Cells.
Stage inputs observe old state; accepted outputs update the fixed destination
at the clock edge. Explicit bypasses model combinational forwarding.
"""
from pycircuit import ac
from isa import Program, Registers, DataWords, Instruction, Fetched, Decoded, Executed, Completed, Control, ForwardedOperands, ExecutionResult, Retirement, StoreEvent, SystemEvent
from isa import decode, alu, branch_taken, forwarding, forwarded, decompress, sign_extend, supported_ecall


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
    def __init__(program: Program, entry: ac.u32, compressed: bool) -> None:
        rom = ac.cell[Program](initial=program)
        pc = ac.cell[ac.u32](initial=entry)

    @ac.rule
    def advance(control: Control) -> ac.cell[Fetched]:
        if control.stop:
            # Preserve RV5S's inactive PC observation during exit drain. Once
            # outside ROM, a halted machine remains idle without indexing it.
            if pc < 4096:
                raw = rom[pc >> 2] >> ((pc & 3) * 8)
                if (pc & 2) != 0 and pc + 3 < 4096:
                    raw = raw | (rom[(pc >> 2) + 1] << 16)
                length = 4 if not compressed or raw == 0 or (raw & 3) == 3 else 2
                pc = pc + length
            return Fetched()
        if control.stall or control.ecall_stall:
            return None
        if control.redirect:
            pc = control.target
            return Fetched()
        if pc >= 4096 or (pc & (1 if compressed else 3)) != 0:
            # A younger fetch can run past ROM before an older branch resolves.
            # Carry the fault to EX so redirect/stop can discard it first.
            packet = Fetched(True, pc, 0, 0, pc + 4, True)
            pc = pc + 4
            return packet
        raw = rom[pc >> 2] >> ((pc & 3) * 8)
        if (pc & 2) != 0 and (raw & 3) == 3:
            if pc + 3 >= 4096:
                packet = Fetched(True, pc, raw & 65535, raw & 65535, pc + 4, True)
                pc = pc + 4
                return packet
            raw = raw | (rom[(pc >> 2) + 1] << 16)
        expanded = decompress(raw, compressed)
        packet = Fetched(True, pc, raw & 65535 if expanded.length == 2 else raw, expanded.word, pc + expanded.length)
        pc = pc + expanded.length
        return packet

    def __call__(control: Control) -> ac.cell[Fetched]:
        return advance(control)


@ac.module
class Decode:
    def __init__(multiply: bool) -> None:
        instruction = ac.signal[Instruction]()

    @ac.rule
    def decode_instruction(source: ac.cell[Fetched]) -> None:
        instruction = decode(source.expanded, multiply)

    @ac.rule
    def inspect() -> Instruction:
        return instruction

    @ac.rule
    def advance(source: ac.cell[Fetched], control: Control,
                writeback: ac.cell[Completed]) -> ac.cell[Decoded]:
        if control.ecall_stall:
            return None
        if control.stall or control.redirect or control.stop:
            bubble = Decoded()
            bubble.stalled = control.stall
            return bubble
        packet = source
        completed = writeback
        decoded = inspect()
        lhs = rf.read(decoded.rs1, completed)
        rhs = rf.read(decoded.rs2, completed)
        return Decoded(packet.valid, packet.pc, packet.word, decoded, lhs, rhs, False, packet.next_pc, packet.expanded, packet.fetch_fault)

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
        return source.valid and executing.load and executing.rd != 0 and (
            executing.rd == instruction.rs1 or executing.rd == instruction.rs2)


    @ac.rule
    def ecall_wait(source: ac.cell[Decoded], memory: ac.cell[Executed], writeback: ac.cell[Completed]) -> bool:
        return source.valid and source.instruction.system == 1 and (memory.writes or writeback.writes)


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
        if packet.valid and packet.fetch_fault:
            assert False
        executing = packet.instruction
        lhs = operands.lhs
        rhs = operands.rhs
        result = alu(executing, packet.pc, lhs, rhs)
        jump = not executing.illegal and (executing.opcode == 111 or executing.opcode == 103)
        redirect = packet.valid and not executing.illegal and (jump or branch_taken(executing, lhs, rhs))
        value = packet.next_pc if jump else result
        next_packet = Executed(packet.valid, packet.pc, packet.word, executing.rd, executing.writes,
                               executing.load, executing.store, result, rhs, value, packet.stalled, executing.funct3, executing.illegal, executing.system, packet.expanded, packet.next_pc)
        return ExecutionResult(next_packet, redirect, result if redirect else 0)

    @ac.rule
    def advance(packet: Executed, control: Control) -> ac.cell[Executed]:
        if control.ecall_stall:
            bubble = Executed()
            bubble.stalled = True
            return bubble
        return packet

    def __call__(packet: Executed, control: Control) -> ac.cell[Executed]:
        return advance(packet, control)


@ac.module
class Memory:
    def __init__(initial: DataWords, base: ac.u32, program: Program) -> None:
        words = ac.cell[DataWords](initial=initial)
        text = ac.cell[Program](initial=program)
        store_event = ac.cell[StoreEvent](initial=StoreEvent())
        load_event = ac.cell[StoreEvent](initial=StoreEvent())

    @ac.rule
    def read_word(address: ac.u32) -> ac.u32:
        if address < 4096:
            return text[address >> 2]
        if address >= base and address - base < 4096:
            return words[(address - base) >> 2]
        assert False
        return 0

    @ac.rule
    def read(address: ac.u32, size: ac.u32) -> ac.u32:
        if (address < 4096 and size <= 4096 - address) or (address >= base and address - base < 4096 and size <= 4096 - (address - base)):
            shift = (address & 3) * 8
            value = read_word(address) >> shift
            if (address & 3) + size > 4:
                value = value | (read_word((address & 4294967292) + 4) << (32 - shift))
            return value & (255 if size == 1 else 65535 if size == 2 else 4294967295)
        assert False
        return 0

    @ac.rule
    def advance(source: ac.cell[Executed]) -> ac.cell[Completed]:
        packet = source
        value = packet.value
        event = StoreEvent()
        loaded = StoreEvent()
        if packet.valid and (packet.load or packet.store):
            size = 1 << (packet.funct3 & 3)
            address = packet.alu
            if packet.load:
                value = read(address, size)
                loaded = StoreEvent(True, address, value, size)
                if packet.funct3 == 0 or packet.funct3 == 1:
                    value = sign_extend(value, size * 8)
            if packet.store:
                if address >= base and address - base < 4096 and size <= 4096 - (address - base):
                    offset = address - base
                    index = offset >> 2
                    shift = (offset & 3) * 8
                    mask = 255 if size == 1 else 65535 if size == 2 else 4294967295
                    updated = words
                    updated[index] = (words[index] & (4294967295 ^ (mask << shift))) | ((packet.store_value & mask) << shift)
                    if (offset & 3) + size > 4:
                        high_shift = 32 - shift
                        updated[index + 1] = (words[index + 1] & (4294967295 ^ (mask >> high_shift))) | ((packet.store_value & mask) >> high_shift)
                    words = updated
                    event = StoreEvent(True, address, packet.store_value & mask, size)
                else:
                    assert False
        store_event = event
        load_event = loaded
        return Completed(packet.valid, packet.pc, packet.word, packet.rd, packet.writes, value, packet.stalled, packet.illegal, packet.system, packet.expanded, packet.next_pc)

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
        if packet.valid and packet.writes:
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
                 data_base: ac.u32, code_size: ac.u32, entry: ac.u32 = 0, multiply: bool = True, compressed: bool = True) -> None:
        rf = RegisterFile(initial_registers)
        if_id = ac.cell[Fetched](initial=Fetched())
        id_ex = ac.cell[Decoded](initial=Decoded())
        ex_mem = ac.cell[Executed](initial=Executed())
        mem_wb = ac.cell[Completed](initial=Completed())
        fetch = Fetch(program, entry, compressed)
        decode_stage = Decode(multiply)
        hazard_unit = HazardUnit()
        forwarding_unit = ForwardingUnit()
        execute = Execute()
        memory = Memory(initial_data, data_base, program)
        writeback = Writeback()
        last_control = ac.cell[Control](initial=Control())
        halted = ac.cell[bool](initial=False)
        system_event = ac.cell[SystemEvent](initial=SystemEvent())
        exit_status = ac.cell[ac.u32](initial=0)

    @ac.rule
    def observe(control: Control) -> None:
        last_control = control

    @ac.rule
    def environment(wait: bool) -> None:
        event = SystemEvent()
        if id_ex.valid and not wait and not halted:
            instruction = id_ex.instruction
            if instruction.system != 0 or instruction.illegal:
                number = rf.read(17, mem_wb)
                argument = rf.read(10, mem_wb)
                event = SystemEvent(True, id_ex.pc, 3 if instruction.illegal else 4 if instruction.system == 1 and not supported_ecall(number) else instruction.system, number, argument)
                if instruction.illegal or instruction.system == 2 or number == 10 or number == 93 or not supported_ecall(number):
                    halted = True
                    exit_status = argument if number == 93 and instruction.system == 1 else 0
        system_event = event

    def __call__() -> None:
        # Independent combinational units observe the stage registers' old values.
        instruction = decode_stage.inspect()
        stall = hazard_unit.load_use(id_ex, instruction)
        operands = forwarding_unit.resolve(id_ex, ex_mem, mem_wb)
        result = execute.resolve(id_ex, operands)
        wait = hazard_unit.ecall_wait(id_ex, ex_mem, mem_wb)
        instruction_ex = id_ex.instruction
        stopping = halted or (id_ex.valid and (instruction_ex.illegal or instruction_ex.system == 2 or (instruction_ex.system == 1 and not wait and (rf.read(17, mem_wb) == 10 or rf.read(17, mem_wb) == 93 or not supported_ecall(rf.read(17, mem_wb))))))
        control = Control(stall, result.redirect, result.target, operands.source_a, operands.source_b, wait, stopping)

        # These bindings update existing Cells at the edge; inputs still read old values.
        if_id = fetch(control)
        id_ex = decode_stage(if_id, control, rf, mem_wb)
        ex_mem = execute(result.packet, control)
        mem_wb = memory(ex_mem)
        writeback(mem_wb, rf, code_size)
        observe(control)
        environment(wait)
