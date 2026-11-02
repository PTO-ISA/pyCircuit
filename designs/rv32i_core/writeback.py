# ruff: noqa: F823, F841, N802 -- captured hardware source.
from rv32i_core.types import make_pins, make_writeback_control
import pycircuit as ac
from rv32i_core.types import (
    BusInputs,
    Packet,
    Machine,
    Pins,
    WritebackControl,
    byte_mask,
)


@ac.struct
class ResultWriteback:
    state: Machine
    io: Pins
    control: WritebackControl


# ndf: implements=RV32-SPEC-COMMIT-001,RV32-SPEC-HAZARD-001
def compute_Writeback(old: Machine, bus: BusInputs) -> ResultWriteback:
    next_state: Machine = old
    io = make_pins()
    io.stage_id = old.decode_stage.valid
    io.stage_ex = old.execute_stage.valid
    io.stage_mem = old.memory_stage.valid
    io.stage_wb = old.writeback_stage.valid
    io.halted = old.halted
    flags = make_writeback_control()
    if not old.halted:
        wb: Packet = old.writeback_stage
        wb_store: ac.u1 = wb.valid and wb.store and (not wb.trap)
        wb_fault: ac.u1 = wb.trap or (wb_store and bus.dmem_ready and bus.dmem_fault)
        wb_done: ac.u1 = wb.valid and (not wb_store or bus.dmem_ready)
        wb_space: ac.u1 = not wb.valid or wb_done
        terminal: ac.u1 = wb_done and wb_fault
        if wb_store:
            io.dmem_valid = 1
            io.dmem_write = 1
            io.dmem_address = wb.address & 4294967292
            io.dmem_mask = byte_mask(wb.size, wb.address)
            offset: ac.u5 = wb.address[0:2]
            shift: ac.u5 = offset << 3
            io.dmem_data = wb.store_data << shift
        if wb_done:
            io.retire_valid = 1
            io.retire_order = old.order
            io.retire_pc = wb.pc
            io.retire_instruction = wb.instruction
            io.retire_next_pc = wb.pc if wb_fault else wb.next_pc
            io.retire_trap = wb_fault
            io.retire_cause = (
                (7 if wb_store and bus.dmem_fault else wb.cause) if wb_fault else 0
            )
            io.retire_tval = (
                (wb.address if wb_store and bus.dmem_fault else wb.tval)
                if wb_fault
                else 0
            )
            io.retire_rd_write = wb.writes and wb.rd != 0 and (not wb_fault)
            io.retire_rd = wb.rd if io.retire_rd_write else 0
            io.retire_value = wb.result if io.retire_rd_write else 0
            io.retire_load = wb.load and (not wb_fault)
            io.retire_store = wb.store and (not wb_fault)
            io.retire_address = (
                wb.address if (wb.load or wb.store) and (not wb_fault) else 0
            )
            io.retire_mask = (
                byte_mask(wb.size, wb.address)
                if (wb.load or wb.store) and (not wb_fault)
                else 0
            )
            raw_mask: ac.u32 = (
                4294967295 if wb.size == 2 else 65535 if wb.size == 1 else 255
            )
            io.retire_mem_data = (
                wb.store_data & raw_mask
                if wb.store and (not wb_fault)
                else wb.load_data if wb.load and (not wb_fault) else 0
            )
            next_state.order = old.order + 1
            next_state.arch_pc = wb.pc if wb_fault else wb.next_pc
            next_state.writeback_stage.valid = 0
            if io.retire_rd_write:
                next_state.registers[wb.rd] = wb.result
        if terminal:
            next_state.halted = 1
            next_state.stopping = 1
            next_state.decode_stage.valid = 0
            next_state.execute_stage.valid = 0
            next_state.memory_stage.valid = 0
            next_state.fetch_buffer.valid = 0
            next_state.fetch_busy = 0
        flags = make_writeback_control()
        flags.store = wb_store
        flags.fault = wb_fault
        flags.done = wb_done
        flags.space = wb_space
        flags.terminal = terminal
    return ResultWriteback(state=next_state, io=io, control=flags)


@ac.module
def Writeback(old: Machine, bus: BusInputs) -> tuple[Machine, Pins, WritebackControl]:
    result = compute_Writeback(old, bus)
    return (result.state, result.io, result.control)
