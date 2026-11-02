# ruff: noqa: F823, F841, N802 -- captured hardware source.
import pycircuit as ac
from rv32i_core.types import (
    BusInputs,
    Packet,
    Machine,
    Pins,
    WritebackControl,
    MemoryControl,
    byte_mask,
    read_load,
)


@ac.struct
class ResultMemory:
    state: Machine
    io: Pins
    memory: Packet
    control: MemoryControl


# ndf: implements=RV32-SPEC-BUS-001,RV32-SPEC-EEI-001
def compute_Memory(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    wb_control: WritebackControl,
    bus: BusInputs,
) -> ResultMemory:
    next_state: Machine = prior
    io: Pins = original_io
    mem: Packet = old.memory_stage
    wb_store: ac.u1 = wb_control.store
    wb_space: ac.u1 = wb_control.space
    load_request: ac.u1 = (
        mem.valid and mem.load and (not mem.trap) and (not mem.done) and (not wb_store)
    )
    if load_request:
        io.dmem_valid = 1
        io.dmem_address = mem.address & 4294967292
        io.dmem_mask = byte_mask(mem.size, mem.address)
    if load_request and bus.dmem_ready:
        mem.done = 1
        mem.result = read_load(bus.dmem_data, mem.address, mem.size, mem.signed_load)
        raw_mask: ac.u32 = (
            4294967295 if mem.size == 2 else 65535 if mem.size == 1 else 255
        )
        offset: ac.u5 = mem.address[0:2]
        shift: ac.u5 = offset << 3
        mem.load_data = bus.dmem_data >> shift & raw_mask
        if bus.dmem_fault:
            mem.trap = 1
            mem.cause = 5
            mem.tval = mem.address
            mem.writes = 0
            mem.next_pc = mem.pc
        next_state.memory_stage = mem
    mem_fault: ac.u1 = mem.valid and mem.trap
    mem_move: ac.u1 = mem.valid and wb_space and (not mem.load or mem.done or mem.trap)
    if mem_move:
        next_state.writeback_stage = mem
        next_state.memory_stage.valid = 0
    mem_space: ac.u1 = not mem.valid or mem_move
    if mem_fault:
        next_state.stopping = 1
        next_state.execute_stage.valid = 0
        next_state.decode_stage.valid = 0
        next_state.fetch_buffer.valid = 0
        next_state.fetch_killed = old.fetch_busy
    return ResultMemory(
        state=next_state,
        io=io,
        memory=mem,
        control=MemoryControl(fault=mem_fault, space=mem_space),
    )


@ac.module
def Memory(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    wb_control: WritebackControl,
    bus: BusInputs,
) -> tuple[Machine, Pins, Packet, MemoryControl]:
    result = compute_Memory(old, prior, original_io, wb_control, bus)
    return (result.state, result.io, result.memory, result.control)
