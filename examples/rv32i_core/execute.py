# ruff: noqa: F823, F841, N802 -- captured hardware source.
import pycircuit as ac
from rv32i_core.types import (
    Execute,
    Packet,
    Machine,
    Pins,
    WritebackControl,
    MemoryControl,
    ExecuteControl,
)
from rv32i_core.arithmetic import compute_Arithmetic


@ac.struct
class ResultExecuteLogic:
    state: Machine
    io: Pins
    control: ExecuteControl


def compute_ExecuteLogic(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    wb_control: WritebackControl,
    mem: Packet,
    mem_control: MemoryControl,
) -> ResultExecuteLogic:
    next_state: Machine = prior
    io: Pins = original_io
    ex: Execute = old.execute_stage
    wb: Packet = old.writeback_stage
    wb_fault: ac.u1 = wb_control.fault
    mem_fault: ac.u1 = mem_control.fault
    mem_space: ac.u1 = mem_control.space
    ex_move: ac.u1 = ex.valid and mem_space and (not mem_fault)
    redirected: ac.u1 = 0
    kill_front: ac.u1 = mem_fault
    if ex_move:
        first: ac.u32 = ex.first
        second: ac.u32 = ex.second
        if wb.valid and wb.writes and (wb.rd != 0) and (not wb_fault):
            if ex.control.use1 and ex.control.rs1 == wb.rd:
                first = wb.result
            if ex.control.use2 and ex.control.rs2 == wb.rd:
                second = wb.result
        if (
            mem.valid
            and mem.writes
            and (mem.rd != 0)
            and (not mem.trap)
            and (not mem.load or mem.done)
        ):
            if ex.control.use1 and ex.control.rs1 == mem.rd:
                first = mem.result
            if ex.control.use2 and ex.control.rs2 == mem.rd:
                second = mem.result
        packet = compute_Arithmetic(ex.control, ex.pc, ex.instruction, first, second)
        next_state.memory_stage = packet
        next_state.execute_stage.valid = 0
        redirected = packet.redirect
        kill_front = packet.redirect or packet.trap
        if packet.trap:
            next_state.stopping = 1
        if packet.redirect:
            next_state.fetch_pc = packet.next_pc
        if kill_front:
            next_state.decode_stage.valid = 0
            next_state.fetch_buffer.valid = 0
            next_state.fetch_killed = old.fetch_busy
    return ResultExecuteLogic(
        state=next_state,
        io=io,
        control=ExecuteControl(
            moved=ex_move,
            kill=kill_front,
            redirected=redirected,
            memory_fault=mem_fault,
        ),
    )


@ac.module
def ExecuteLogic(
    old: Machine,
    prior: Machine,
    original_io: Pins,
    wb_control: WritebackControl,
    mem: Packet,
    mem_control: MemoryControl,
) -> tuple[Machine, Pins, ExecuteControl]:
    result = compute_ExecuteLogic(old, prior, original_io, wb_control, mem, mem_control)
    return (result.state, result.io, result.control)
