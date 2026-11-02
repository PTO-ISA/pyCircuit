# ruff: noqa: F823, F841, N802 -- captured hardware source.
import pycircuit as ac
from rv32i_core.types import (
    BusInputs,
    Fetch,
    Execute,
    Packet,
    Machine,
    Pins,
    ExecuteControl,
)
from rv32i_core.decoder import compute_Decoder


@ac.struct
class ResultFrontend:
    state: Machine
    io: Pins


def compute_Frontend(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    ex_control: ExecuteControl,
    bus: BusInputs,
) -> ResultFrontend:
    next_state: Machine = prior
    io: Pins = original_io
    ident: Fetch = old.decode_stage
    ex: Execute = old.execute_stage
    wb: Packet = old.writeback_stage
    ex_move: ac.u1 = ex_control.moved
    kill_front: ac.u1 = ex_control.kill
    redirected: ac.u1 = ex_control.redirected
    mem_fault: ac.u1 = ex_control.memory_fault
    control = compute_Decoder(ident.instruction)
    if ident.fault:
        control.trap = 1
        control.cause = 1
        control.tval = ident.pc
        control.writes = 0
    dependency: ac.u1 = (
        ex.valid
        and ex.control.load
        and (ex.control.rd != 0)
        and (
            control.use1
            and control.rs1 == ex.control.rd
            or (control.use2 and control.rs2 == ex.control.rd)
        )
    )
    ex_space: ac.u1 = not ex.valid or ex_move
    id_move: ac.u1 = (
        ident.valid
        and ex_space
        and (not dependency)
        and (not kill_front)
        and (not next_state.stopping)
    )
    io.load_use = ident.valid and dependency and ex_move and (not kill_front)
    if id_move:
        first: ac.u32 = old.registers[control.rs1] if control.rs1 != 0 else 0
        second: ac.u32 = old.registers[control.rs2] if control.rs2 != 0 else 0
        if io.retire_rd_write:
            if control.rs1 == wb.rd:
                first = wb.result
            if control.rs2 == wb.rd:
                second = wb.result
        next_state.execute_stage = Execute(
            valid=1,
            pc=ident.pc,
            instruction=ident.instruction,
            control=control,
            first=first,
            second=second,
        )
        next_state.decode_stage.valid = 0
    id_space: ac.u1 = not ident.valid or id_move
    if (
        old.fetch_buffer.valid
        and id_space
        and (not kill_front)
        and (not next_state.stopping)
    ):
        next_state.decode_stage = old.fetch_buffer
        next_state.fetch_buffer.valid = 0
    launch: ac.u1 = (
        not old.fetch_busy
        and (not old.fetch_buffer.valid)
        and id_space
        and (not kill_front)
        and (not next_state.stopping)
    )
    io.imem_valid = old.fetch_busy or launch
    io.imem_address = old.fetch_request_pc if old.fetch_busy else old.fetch_pc
    if io.imem_valid:
        if bus.imem_ready:
            next_state.fetch_busy = 0
            next_state.fetch_killed = 0
            if not old.fetch_killed and (not kill_front) and (not next_state.stopping):
                fetched = Fetch(
                    valid=1,
                    pc=io.imem_address,
                    instruction=bus.imem_data,
                    fault=bus.imem_fault,
                )
                next_state.fetch_pc = io.imem_address + 4
                if id_space:
                    next_state.decode_stage = fetched
                else:
                    next_state.fetch_buffer = fetched
        else:
            next_state.fetch_busy = 1
            next_state.fetch_request_pc = io.imem_address
    io.redirected = redirected or mem_fault
    io.memory_wait = io.dmem_valid and (not bus.dmem_ready)
    return ResultFrontend(state=next_state, io=io)


@ac.module
def Frontend(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    ex_control: ExecuteControl,
    bus: BusInputs,
) -> tuple[Machine, Pins]:
    result = compute_Frontend(old, prior, original_io, ex_control, bus)
    return (result.state, result.io)


def compute_Finalize(
    old: Machine,
    stopped: Machine,
    stopped_io: Pins,
    active: Machine,
    active_io: Pins,
    terminal: ac.u1,
) -> ResultFrontend:
    result = ResultFrontend(state=active, io=active_io)
    if old.halted or terminal:
        result = ResultFrontend(state=stopped, io=stopped_io)
    result.io.stage_if = result.io.imem_valid or old.fetch_buffer.valid
    return result


@ac.module
def Finalize(
    old: Machine,
    stopped: Machine,
    stopped_io: Pins,
    active: Machine,
    active_io: Pins,
    terminal: ac.u1,
) -> tuple[Machine, Pins]:
    result = compute_Finalize(old, stopped, stopped_io, active, active_io, terminal)
    return result.state, result.io
