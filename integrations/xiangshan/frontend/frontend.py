"""Frontend — Top-level frontend integration for XiangShan-pyc.

Wires together BPU → FTQ → IFU + ICache → IBuffer → Decode in a simplified
pass-through pipeline.  The full sub-module build functions are intended to be
called from here once all leaf modules are complete; this version exposes the
key inter-module interfaces as a multi-stage pipeline.

Reference: XiangShan/src/main/scala/xiangshan/frontend/Frontend.scala

Pipeline (simplified):
  Cycle 0 — BPU: prediction / redirect handling, generate fetch PC
  Cycle 1 — ICache: send fetch request, receive response
  Cycle 2 — IFU: instruction extraction, IBuffer enqueue
  Cycle 3 — Decode: produce decoded uops for backend

Key features:
  F-FE-001  Redirect from backend overrides fetch PC
  F-FE-002  BPU prediction drives FTQ / ICache fetch address
  F-FE-003  ICache hit path → IFU → IBuffer → Decode
  F-FE-004  ICache miss → L2 refill request
  F-FE-005  Backpressure: IBuffer full stalls IFU
  F-FE-006  Decoded uops output to backend dispatch
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    wire_of,
)
from top.parameters import (
    DECODE_WIDTH,
    FETCH_BLOCK_SIZE,
    ICACHE_BLOCK_BYTES,
    INST_BYTES,
    PC_WIDTH,
)

INST_WIDTH = 32
FETCH_WIDTH = FETCH_BLOCK_SIZE // INST_BYTES
BLOCK_BITS = ICACHE_BLOCK_BYTES * 8

from frontend.bpu.bpu import bpu
from frontend.decode.decode import decode
from frontend.ftq.ftq import ftq
from frontend.ibuffer.ibuffer import ibuffer
from frontend.icache.icache import icache
from frontend.ifu.ifu import ifu


def frontend(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "fe",
    decode_width: int = DECODE_WIDTH,
    pc_width: int = PC_WIDTH,
    fetch_width: int = FETCH_WIDTH,
    inst_width: int = INST_WIDTH,
    block_bits: int = BLOCK_BITS,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Frontend: simplified BPU → ICache → IBuffer → Decode pipeline."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    # ── Sub-module calls ──
    domain.call(bpu, inputs={}, prefix=f"{prefix}_s_bpu", pc_width=pc_width)

    domain.call(ftq, inputs={}, prefix=f"{prefix}_s_ftq", pc_width=pc_width)

    domain.call(icache, inputs={}, prefix=f"{prefix}_s_ic", pc_width=pc_width)

    domain.call(ifu, inputs={}, prefix=f"{prefix}_s_ifu", pc_width=pc_width)

    domain.call(ibuffer, inputs={}, prefix=f"{prefix}_s_ibuf", deq_width=decode_width)

    domain.call(
        decode,
        inputs={},
        prefix=f"{prefix}_s_dec",
        decode_width=decode_width,
        pc_width=pc_width,
    )

    pred_block_bytes = FETCH_BLOCK_SIZE

    # ================================================================
    # Cycle 0 — BPU prediction / redirect handling
    # ================================================================

    # Backend redirect (highest priority)
    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_target = (
        _in["redirect_target"]
        if "redirect_target" in _in
        else cas(domain, m.input(f"{prefix}_redirect_target", width=pc_width), cycle=0)
    )

    # Backpressure from IBuffer / decode
    ibuf_ready = (
        _in["ibuf_ready"]
        if "ibuf_ready" in _in
        else cas(domain, m.input(f"{prefix}_ibuf_ready", width=1), cycle=0)
    )

    # ICache refill interface (from L2)
    refill_valid = (
        _in["refill_valid"]
        if "refill_valid" in _in
        else cas(domain, m.input(f"{prefix}_refill_valid", width=1), cycle=0)
    )
    refill_data = (
        _in["refill_data"]
        if "refill_data" in _in
        else cas(domain, m.input(f"{prefix}_refill_data", width=block_bits), cycle=0)
    )

    cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)
    cas(domain, m.const(0, width=pc_width), cycle=0)
    FALLTHROUGH = cas(domain, m.const(pred_block_bytes, width=pc_width), cycle=0)

    # BPU state: fetch PC register
    fetch_pc = domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_fetch_pc")
    bpu_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_bpu_valid")

    # BPU simplified prediction: fallthrough unless redirected
    bpu_pred_target = cas(
        domain, (wire_of(fetch_pc) + wire_of(FALLTHROUGH))[0:pc_width], cycle=0
    )

    # Flush pipeline on redirect
    flush = redirect_valid

    # s0 fire: prediction valid and downstream ready
    s0_fire = bpu_valid & ibuf_ready & (~flush)

    # Next PC selection (priority: redirect > prediction > hold)
    next_pc = fetch_pc
    next_pc = mux(s0_fire, bpu_pred_target, next_pc)
    next_pc = mux(redirect_valid, redirect_target, next_pc)

    m.output(f"{prefix}_bpu_pred_pc", wire_of(fetch_pc))
    _out["bpu_pred_pc"] = fetch_pc
    m.output(f"{prefix}_bpu_pred_target", wire_of(bpu_pred_target))
    _out["bpu_pred_target"] = bpu_pred_target
    m.output(f"{prefix}_bpu_pred_valid", wire_of(s0_fire))
    _out["bpu_pred_valid"] = s0_fire

    # ── Pipeline registers: cycle 0 → cycle 1 ─────────────────────
    s1_valid_w = domain.cycle(wire_of(s0_fire), name=f"{prefix}_s1_v")
    s1_pc_w = domain.cycle(wire_of(fetch_pc), name=f"{prefix}_s1_pc")

    domain.next()

    # ================================================================
    # Cycle 1 — ICache fetch (simplified: single-cycle hit model)
    # ================================================================

    s1_valid = s1_valid_w & (~wire_of(redirect_valid))

    # ICache hit model: refill_valid acts as data-ready signal
    s1_resp_valid = s1_valid & wire_of(refill_valid)
    s1_miss = s1_valid & (~wire_of(refill_valid))

    m.output(f"{prefix}_icache_miss_valid", s1_miss)
    _out["icache_miss_valid"] = cas(domain, s1_miss, cycle=domain.cycle_index)
    m.output(f"{prefix}_icache_miss_addr", s1_pc_w)
    _out["icache_miss_addr"] = cas(domain, s1_pc_w, cycle=domain.cycle_index)

    # ── Pipeline registers: cycle 1 → cycle 2 ─────────────────────
    s2_valid_w = domain.cycle(s1_resp_valid, name=f"{prefix}_s2_v")
    s2_pc_w = domain.cycle(s1_pc_w, name=f"{prefix}_s2_pc")
    s2_data_w = domain.cycle(wire_of(refill_data), name=f"{prefix}_s2_data")

    domain.next()

    # ================================================================
    # Cycle 2 — IFU: instruction extraction + IBuffer enqueue
    # ================================================================

    s2_valid = s2_valid_w & (~wire_of(redirect_valid))

    # Simplified instruction extraction: slice block into inst_width chunks
    ifu_insts = []
    for i in range(decode_width):
        lo = i * inst_width
        hi = lo + inst_width
        if hi <= block_bits:
            ifu_insts.append(s2_data_w[lo:hi])
        else:
            ifu_insts.append(m.const(0, width=inst_width))

    # ── Pipeline registers: cycle 2 → cycle 3 ─────────────────────
    s3_valid_w = domain.cycle(s2_valid, name=f"{prefix}_s3_v")
    s3_pc_w = domain.cycle(s2_pc_w, name=f"{prefix}_s3_pc")
    s3_insts_w = [
        domain.cycle(ifu_insts[i], name=f"{prefix}_s3_inst_{i}")
        for i in range(decode_width)
    ]

    domain.next()

    # ================================================================
    # Cycle 3 — Decode outputs to backend
    # ================================================================

    s3_valid = s3_valid_w & (~wire_of(redirect_valid))

    for i in range(decode_width):
        inst_pc = (s3_pc_w + m.const(i * INST_BYTES, width=pc_width))[0:pc_width]
        m.output(f"{prefix}_dec_valid_{i}", s3_valid)
        m.output(f"{prefix}_dec_inst_{i}", s3_insts_w[i])
        m.output(f"{prefix}_dec_pc_{i}", inst_pc)

    m.output(f"{prefix}_frontend_stall", ~wire_of(ibuf_ready))

    # ================================================================
    # State updates (after last domain.next)
    # ================================================================

    domain.next()

    bpu_valid <<= ONE_1
    fetch_pc <<= next_pc
    return _out


frontend.__pycircuit_name__ = "frontend"


if __name__ == "__main__":
    pass
