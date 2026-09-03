"""FPU — Floating-Point Unit for XiangShan-pyc backend.

Simplified FP execution pipeline supporting FADD, FSUB, FMUL (3-cycle
pipelined latency) and FDIV (variable latency, FSM-based like the integer
divider).

Reference: XiangShan/src/main/scala/xiangshan/backend/fu/fpu/

Operations (2-bit fpu_op encoding):
  00  FADD     floating-point add
  01  FSUB     floating-point subtract
  10  FMUL     floating-point multiply
  11  FDIV     floating-point divide (variable latency)

Key features:
  B-FPU-001  3-cycle pipelined latency for FADD/FSUB/FMUL
  B-FPU-002  Variable-latency FDIV via FSM (IDLE→BUSY→DONE)
  B-FPU-003  Valid/ready handshake
  B-FPU-004  Flush support to cancel in-flight operations
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    u,
    wire_of,
)
from top.parameters import XLEN

FPU_OP_WIDTH = 2

OP_FADD = 0b00
OP_FSUB = 0b01
OP_FMUL = 0b10
OP_FDIV = 0b11

PIPE_LATENCY = 3
FDIV_LATENCY = 12

ST_IDLE = 0
ST_BUSY = 1
ST_DONE = 2
STATE_WIDTH = 2


def fpu(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "fpu",
    data_width: int = XLEN,
    pipe_latency: int = PIPE_LATENCY,
    fdiv_latency: int = FDIV_LATENCY,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """FPU: floating-point unit with pipelined add/sub/mul and FSM-based div."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    op_w = FPU_OP_WIDTH
    cnt_w = max(1, fdiv_latency.bit_length())
    double_w = data_width * 2

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    in_valid = (
        _in["in_valid"]
        if "in_valid" in _in
        else cas(domain, m.input(f"{prefix}_in_valid", width=1), cycle=0)
    )
    src1 = (
        _in["src1"]
        if "src1" in _in
        else cas(domain, m.input(f"{prefix}_src1", width=data_width), cycle=0)
    )
    src2 = (
        _in["src2"]
        if "src2" in _in
        else cas(domain, m.input(f"{prefix}_src2", width=data_width), cycle=0)
    )
    fpu_op = (
        _in["fpu_op"]
        if "fpu_op" in _in
        else cas(domain, m.input(f"{prefix}_fpu_op", width=op_w), cycle=0)
    )
    out_ready = (
        _in["out_ready"]
        if "out_ready" in _in
        else cas(domain, m.input(f"{prefix}_out_ready", width=1), cycle=0)
    )
    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    def _const(val, w=data_width):
        return cas(domain, m.const(val, width=w), cycle=0)

    def _op(val):
        return cas(domain, m.const(val, width=op_w), cycle=0)

    cas(domain, m.const(0, width=1), cycle=0)
    cas(domain, m.const(1, width=1), cycle=0)

    is_fdiv = fpu_op == _op(OP_FDIV)
    is_pipe = ~is_fdiv

    # ── Pipelined path (FADD/FSUB/FMUL): combinational result ───
    add_result = cas(domain, (wire_of(src1) + wire_of(src2))[0:data_width], cycle=0)
    sub_result = cas(domain, (wire_of(src1) - wire_of(src2))[0:data_width], cycle=0)

    src1_wide = cas(domain, (wire_of(src1) + u(double_w, 0))[0:double_w], cycle=0)
    src2_wide = cas(domain, (wire_of(src2) + u(double_w, 0))[0:double_w], cycle=0)
    mul_full = cas(
        domain, (wire_of(src1_wide) * wire_of(src2_wide))[0:double_w], cycle=0
    )
    mul_result = mul_full[0:data_width]

    pipe_result = add_result
    pipe_result = mux(fpu_op == _op(OP_FSUB), sub_result, pipe_result)
    pipe_result = mux(fpu_op == _op(OP_FMUL), mul_result, pipe_result)

    pipe_fire = in_valid & is_pipe & (~flush)

    # Pipeline stages for pipelined path
    pipe_v = pipe_fire
    pipe_r = pipe_result
    for stage in range(pipe_latency):
        pipe_v_w = domain.cycle(wire_of(pipe_v), name=f"{prefix}_fpipe_v_{stage}")
        pipe_r_w = domain.cycle(wire_of(pipe_r), name=f"{prefix}_fpipe_r_{stage}")
        pipe_v = cas(domain, pipe_v_w, cycle=0)
        pipe_r = cas(domain, pipe_r_w, cycle=0)

    # ── FDIV FSM path ────────────────────────────────────────────
    cur_state = domain.signal(
        width=STATE_WIDTH, reset_value=ST_IDLE, name=f"{prefix}_fdiv_fsm"
    )
    cur_cnt = domain.signal(width=cnt_w, reset_value=0, name=f"{prefix}_fdiv_cnt")
    cur_ds1 = domain.signal(width=data_width, reset_value=0, name=f"{prefix}_fdiv_s1")
    cur_ds2 = domain.signal(width=data_width, reset_value=0, name=f"{prefix}_fdiv_s2")
    cur_dres = domain.signal(width=data_width, reset_value=0, name=f"{prefix}_fdiv_res")

    is_idle = cur_state == cas(domain, m.const(ST_IDLE, width=STATE_WIDTH), cycle=0)
    is_busy = cur_state == cas(domain, m.const(ST_BUSY, width=STATE_WIDTH), cycle=0)
    is_done = cur_state == cas(domain, m.const(ST_DONE, width=STATE_WIDTH), cycle=0)

    cnt_zero = cur_cnt == cas(domain, m.const(0, width=cnt_w), cycle=0)

    # Simplified division: dividend shifted right (placeholder for real FP div)
    divisor_zero = cur_ds2 == _const(0)
    all_ones = _const((1 << data_width) - 1)
    div_quot = cas(
        domain, wire_of(cur_ds1).lshr(amount=m.const(0, width=1))[0:data_width], cycle=0
    )
    div_safe = mux(divisor_zero, all_ones, div_quot)

    # ── Outputs ──────────────────────────────────────────────────
    # Pipe output valid after pipeline stages, FDIV output valid when done
    pipe_out_valid = pipe_v & (~flush)
    div_out_valid = is_done & (~flush)

    out_valid = pipe_out_valid | div_out_valid
    result = mux(div_out_valid, cur_dres, pipe_r)
    in_ready = (is_pipe | (is_fdiv & is_idle)) & (~flush)

    m.output(f"{prefix}_out_valid", wire_of(out_valid))
    _out["out_valid"] = out_valid
    m.output(f"{prefix}_in_ready", wire_of(in_ready))
    _out["in_ready"] = in_ready
    m.output(f"{prefix}_result", wire_of(result))
    _out["result"] = result

    # ── Cycle 1: State updates ───────────────────────────────────
    domain.next()

    LAT_CONST = cas(domain, m.const(fdiv_latency - 1, width=cnt_w), cycle=0)
    CNT_DEC = cas(
        domain, (wire_of(cur_cnt) - m.const(1, width=cnt_w))[0:cnt_w], cycle=0
    )

    # FDIV: IDLE → BUSY on valid fdiv input
    div_start = is_idle & in_valid & is_fdiv & (~flush)
    cur_state.assign(
        cas(domain, m.const(ST_BUSY, width=STATE_WIDTH), cycle=0), when=div_start
    )
    cur_cnt.assign(LAT_CONST, when=div_start)
    cur_ds1.assign(src1, when=div_start)
    cur_ds2.assign(src2, when=div_start)

    # BUSY: decrement counter; → DONE when counter reaches zero
    busy_tick = is_busy & (~flush)
    cur_cnt.assign(CNT_DEC, when=busy_tick)
    busy_to_done = is_busy & cnt_zero & (~flush)
    cur_state.assign(
        cas(domain, m.const(ST_DONE, width=STATE_WIDTH), cycle=0), when=busy_to_done
    )
    cur_dres.assign(div_safe, when=busy_to_done)

    # DONE → IDLE on downstream accept
    done_ack = is_done & out_ready & (~flush)
    cur_state.assign(
        cas(domain, m.const(ST_IDLE, width=STATE_WIDTH), cycle=0), when=done_ack
    )

    # Flush: return to IDLE
    cur_state.assign(
        cas(domain, m.const(ST_IDLE, width=STATE_WIDTH), cycle=0), when=flush
    )
    cur_cnt.assign(cas(domain, m.const(0, width=cnt_w), cycle=0), when=flush)
    return _out


fpu.__pycircuit_name__ = "fpu"


if __name__ == "__main__":
    pass
