from __future__ import annotations

from pycircuit import Circuit, Wire, module, mux, u, zext

from .decode import decode_window
from .isa import BK_FALL, OP_INVALID
from .pipeline import CoreState, MemWbRegs
from .stages.wb_stage import build_wb_stage


@module
def build_icall_contract(m: Circuit, clk: Wire, rst: Wire, cycle: Wire) -> None:
    """Exercise the released SETC.TGT/BSTART.ICALL/BSTOP production path."""
    zero64 = zext(cycle, width=64) & u(64, 0)
    window = zero64 | u(64, 0x7F)
    pc = zero64
    wb_value = zero64
    window = mux(cycle == 0, u(64, 0x001C), window)
    wb_value = mux(cycle == 0, u(64, 0x8800), wb_value)
    window = mux(cycle == 1, u(64, 0x50166001), window)
    pc = mux(cycle == 1, u(64, 0x4000), pc)
    window = mux(cycle == 3, u(64, 0x0000), window)

    window_wire = m.named_wire("window", width=64)
    m.assign(window_wire, window)
    decoded = decode_window(m, window_wire)
    retired_compressed = decode_window(m, zero64 | u(64, 0x3000))

    state = CoreState(
        pc=m.out("state_pc", clk=clk, rst=rst, width=64),
        br_kind=m.out("br_kind", clk=clk, rst=rst, width=3, init=BK_FALL),
        br_base_pc=m.out("br_base_pc", clk=clk, rst=rst, width=64),
        br_off=m.out("br_off", clk=clk, rst=rst, width=64),
        commit_cond=m.out("commit_cond", clk=clk, rst=rst, width=1),
        commit_tgt=m.out("commit_tgt", clk=clk, rst=rst, width=64),
        icall_tgt=m.out("icall_tgt", clk=clk, rst=rst, width=64),
        dec_hdr_active=m.out("dec_hdr_active", clk=clk, rst=rst, width=1),
        in_body=m.out("in_body", clk=clk, rst=rst, width=1),
        body_tpc=m.out("body_tpc", clk=clk, rst=rst, width=64),
        return_pc=m.out("return_pc", clk=clk, rst=rst, width=64),
        exit_code=m.out("exit_code", clk=clk, rst=rst, width=32),
        cycles=m.out("cycles", clk=clk, rst=rst, width=64),
        halted=m.out("halted", clk=clk, rst=rst, width=1),
    )
    memwb = MemWbRegs(
        valid=m.out("memwb_valid", clk=clk, rst=rst, width=1),
        pc=m.out("memwb_pc", clk=clk, rst=rst, width=64),
        window=m.out("memwb_window", clk=clk, rst=rst, width=64),
        pred_next_pc=m.out("memwb_pred_next_pc", clk=clk, rst=rst, width=64),
        op=m.out("memwb_op", clk=clk, rst=rst, width=12),
        len_bytes=m.out("memwb_len_bytes", clk=clk, rst=rst, width=3),
        regdst=m.out("memwb_regdst", clk=clk, rst=rst, width=6),
        srcl=m.out("memwb_srcl", clk=clk, rst=rst, width=6),
        srcr=m.out("memwb_srcr", clk=clk, rst=rst, width=6),
        imm=m.out("memwb_imm", clk=clk, rst=rst, width=64),
        value=m.out("memwb_value", clk=clk, rst=rst, width=64),
        is_load=m.out("memwb_is_load", clk=clk, rst=rst, width=1),
        is_store=m.out("memwb_is_store", clk=clk, rst=rst, width=1),
        size=m.out("memwb_size", clk=clk, rst=rst, width=3),
        addr=m.out("memwb_addr", clk=clk, rst=rst, width=64),
        wdata=m.out("memwb_wdata", clk=clk, rst=rst, width=64),
    )

    control = build_wb_stage(m, do_wb=memwb.valid.out(), state=state, memwb=memwb)

    memwb.valid.set(1)
    memwb.pc.set(pc)
    memwb.window.set(window)
    memwb.pred_next_pc.set(0)
    memwb.op.set(decoded.op)
    memwb.len_bytes.set(decoded.len_bytes)
    memwb.regdst.set(decoded.regdst)
    memwb.srcl.set(decoded.srcl)
    memwb.srcr.set(decoded.srcr)
    memwb.imm.set(decoded.imm)
    wb_value = mux(cycle != 0, decoded.imm, wb_value)
    memwb.value.set(wb_value)
    memwb.is_load.set(0)
    memwb.is_store.set(0)
    memwb.size.set(0)
    memwb.addr.set(0)
    memwb.wdata.set(0)

    for unused in (
        state.pc,
        state.dec_hdr_active,
        state.in_body,
        state.body_tpc,
        state.return_pc,
        state.exit_code,
        state.cycles,
        state.halted,
    ):
        unused.set(unused.out())

    observed_valid = m.out("observed_valid", clk=clk, rst=rst, width=1)
    observed_target = m.out("observed_target", clk=clk, rst=rst, width=64)
    observed_ra = m.out("observed_ra", clk=clk, rst=rst, width=64)
    observed_valid.set(1, when=control.ra_write_valid)
    observed_target.set(control.target_pc, when=control.ra_write_valid)
    observed_ra.set(control.ra_write_value, when=control.ra_write_valid)

    m.output("icall_contract_valid", observed_valid)
    m.output("icall_contract_target", observed_target)
    m.output("icall_contract_ra", observed_ra)
    m.output("icall_contract_raw_3000_invalid", retired_compressed.op == OP_INVALID)
