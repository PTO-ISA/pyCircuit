# ruff: noqa: F823, F841, N802 -- captured hardware declarations and state.
import pycircuit as ac


# ndf: implements=RV32-SPEC-BUS-001
@ac.struct
class BusInputs:
    imem_ready: ac.u1
    imem_data: ac.u32
    imem_fault: ac.u1
    dmem_ready: ac.u1
    dmem_data: ac.u32
    dmem_fault: ac.u1


@ac.struct
class Decoded:
    rs1: ac.u5
    rs2: ac.u5
    rd: ac.u5
    use1: ac.u1
    use2: ac.u1
    writes: ac.u1
    operation: ac.u4
    immediate: ac.u32
    immediate_rhs: ac.u1
    load: ac.u1
    store: ac.u1
    size: ac.u2
    signed_load: ac.u1
    branch: ac.u1
    condition: ac.u3
    jump: ac.u2
    trap: ac.u1 = 1
    cause: ac.u4 = 2
    tval: ac.u32


@ac.struct
class Fetch:
    valid: ac.u1
    pc: ac.u32
    instruction: ac.u32
    fault: ac.u1


@ac.struct
class Execute:
    valid: ac.u1
    pc: ac.u32
    instruction: ac.u32
    control: Decoded
    first: ac.u32
    second: ac.u32


@ac.struct
class Packet:
    valid: ac.u1
    pc: ac.u32
    instruction: ac.u32
    next_pc: ac.u32
    rd: ac.u5
    writes: ac.u1
    result: ac.u32
    load: ac.u1
    store: ac.u1
    address: ac.u32
    store_data: ac.u32
    load_data: ac.u32
    size: ac.u2
    signed_load: ac.u1
    done: ac.u1
    redirect: ac.u1
    trap: ac.u1
    cause: ac.u4
    tval: ac.u32


# ndf: implements=RV32-SPEC-STATE-001
@ac.struct
class Machine:
    registers: ac.table[32, ac.u32]
    fetch_pc: ac.u32 = 2147483648
    arch_pc: ac.u32 = 2147483648
    fetch_busy: ac.u1
    fetch_request_pc: ac.u32
    fetch_killed: ac.u1
    fetch_buffer: Fetch
    decode_stage: Fetch
    execute_stage: Execute
    memory_stage: Packet
    writeback_stage: Packet
    stopping: ac.u1
    halted: ac.u1
    order: ac.u64


# ndf: implements=RV32-SPEC-BUS-001,RV32-SPEC-COMMIT-001
@ac.struct
class Pins:
    imem_valid: ac.u1
    imem_address: ac.u32
    dmem_valid: ac.u1
    dmem_write: ac.u1
    dmem_address: ac.u32
    dmem_data: ac.u32
    dmem_mask: ac.u4
    retire_valid: ac.u1
    retire_order: ac.u64
    retire_pc: ac.u32
    retire_instruction: ac.u32
    retire_next_pc: ac.u32
    retire_rd_write: ac.u1
    retire_rd: ac.u5
    retire_value: ac.u32
    retire_load: ac.u1
    retire_store: ac.u1
    retire_address: ac.u32
    retire_mask: ac.u4
    retire_mem_data: ac.u32
    retire_trap: ac.u1
    retire_cause: ac.u4
    retire_tval: ac.u32
    stage_if: ac.u1
    stage_id: ac.u1
    stage_ex: ac.u1
    stage_mem: ac.u1
    stage_wb: ac.u1
    load_use: ac.u1
    memory_wait: ac.u1
    redirected: ac.u1
    halted: ac.u1


@ac.struct
class WritebackControl:
    store: ac.u1
    fault: ac.u1
    done: ac.u1
    space: ac.u1
    terminal: ac.u1


@ac.struct
class MemoryControl:
    fault: ac.u1
    space: ac.u1


@ac.struct
class ExecuteControl:
    moved: ac.u1
    kill: ac.u1
    redirected: ac.u1
    memory_fault: ac.u1


def byte_mask(size: ac.u2, address: ac.u32) -> ac.u4:
    base: ac.u4 = 15 if size == 2 else 3 if size == 1 else 1
    return base << address[0:2]


# ndf: implements=RV32-SPEC-ISA-001,RV32-SPEC-BUS-001
def read_load(data: ac.u32, address: ac.u32, size: ac.u2, signed: ac.u1) -> ac.u32:
    low: ac.u3 = 0
    shift: ac.u5 = ac.concat(address[0:2], low)
    value: ac.u32 = data >> shift
    if size == 0:
        value = value & 255
        if signed and value[7:8]:
            value = value | 4294967040
    elif size == 1:
        value = value & 65535
        if signed and value[15:16]:
            value = value | 4294901760
    return value


def make_decoded() -> Decoded:
    return Decoded()


def make_packet() -> Packet:
    return Packet()


def make_pins() -> Pins:
    return Pins()


def make_writeback_control() -> WritebackControl:
    return WritebackControl()


def initial_machine() -> Machine:
    return Machine()
