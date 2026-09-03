"""Dispatch — Dispatch Unit for XiangShan-pyc backend.

Takes renamed micro-ops and routes them to the appropriate issue queues
(Integer, Floating-point, Memory) based on functional unit type.  Applies
backpressure when a target issue queue is full.

Homogeneous per-slot buses are Vector ports (shape=[dispatch_width]);
scalar control (flush, IQ ready, stall, counts) stays scalar.

Reference: XiangShan/src/main/scala/xiangshan/backend/dispatch/

Pipeline:
  Cycle 0 — Receive renamed uops, classify by fu_type, check IQ availability
  Cycle 1 — Write accepted uops into target issue queues, emit ROB enqueue

Key features:
  B-DP-001  FU-type based routing: int / fp / mem issue queues
  B-DP-002  Backpressure: stall rename if any target IQ is full
  B-DP-003  Per-slot dispatch valid: slot fires only if its target IQ has room
  B-DP-004  ROB enqueue output for each dispatched uop
  B-DP-005  Flush on redirect: cancel in-flight dispatches
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
    compile_cycle_aware,
    mux,
    u,
    wire_of,
)

from top.parameters import (
    PC_WIDTH,
    PTAG_WIDTH_INT,
    RENAME_WIDTH,
    ROB_IDX_WIDTH,
)

# FU type encoding (3 bits)
FU_TYPE_WIDTH = 3
FU_ALU = 0  # Integer ALU
FU_MUL = 1  # Integer multiply
FU_DIV = 2  # Integer divide
FU_BRU = 3  # Branch unit
FU_FPU = 4  # Floating-point
FU_FMISC = 5  # FP misc (fmv, fcvt)
FU_LDU = 6  # Load unit
FU_STU = 7  # Store unit

# IQ class encoding (2 bits)
IQ_CLASS_WIDTH = 2
IQ_INT = 0
IQ_FP = 1
IQ_MEM = 2


def _vin(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    _in: dict[str, CycleAwareSignal],
    key: str,
    *,
    prefix: str,
    width: int,
    shape: list[int],
) -> CycleAwareSignal:
    """Resolve an injected Vector CAS or declare a shaped input port."""
    if key in _in:
        return _in[key]
    return cas(
        domain,
        m.input(f"{prefix}_{key}", width=width, shape=shape),
        cycle=0,
    )


def _sin(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    _in: dict[str, CycleAwareSignal],
    key: str,
    *,
    prefix: str,
) -> CycleAwareSignal:
    if key in _in:
        return _in[key]
    return cas(domain, m.input(f"{prefix}_{key}", width=1), cycle=0)


def dispatch(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "dp",
    dispatch_width: int = RENAME_WIDTH,
    fu_type_width: int = FU_TYPE_WIDTH,
    ptag_w: int = PTAG_WIDTH_INT,
    pc_width: int = PC_WIDTH,
    rob_idx_w: int = ROB_IDX_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Dispatch: route renamed uops to int / fp / mem issue queues."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}
    shape = [dispatch_width]
    dp_cnt_w = max(1, dispatch_width.bit_length())

    # ================================================================
    # Cycle 0 — Receive renamed uops, classify, check availability
    # ================================================================

    flush = _sin(m, domain, _in, "flush", prefix=prefix)

    in_valid = _vin(m, domain, _in, "in_valid", prefix=prefix, width=1, shape=shape)
    in_pdest = _vin(
        m, domain, _in, "in_pdest", prefix=prefix, width=ptag_w, shape=shape
    )
    in_psrc1 = _vin(
        m, domain, _in, "in_psrc1", prefix=prefix, width=ptag_w, shape=shape
    )
    in_psrc2 = _vin(
        m, domain, _in, "in_psrc2", prefix=prefix, width=ptag_w, shape=shape
    )
    in_old_pdest = _vin(
        m, domain, _in, "in_old_pdest", prefix=prefix, width=ptag_w, shape=shape
    )
    in_fu_type = _vin(
        m, domain, _in, "in_fu_type", prefix=prefix, width=fu_type_width, shape=shape
    )
    in_rob_idx = _vin(
        m, domain, _in, "in_rob_idx", prefix=prefix, width=rob_idx_w, shape=shape
    )
    in_pc = _vin(m, domain, _in, "in_pc", prefix=prefix, width=pc_width, shape=shape)

    iq_int_ready = _sin(m, domain, _in, "iq_int_ready", prefix=prefix)
    iq_fp_ready = _sin(m, domain, _in, "iq_fp_ready", prefix=prefix)
    iq_mem_ready = _sin(m, domain, _in, "iq_mem_ready", prefix=prefix)

    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)

    FU_FPU_C = cas(domain, m.const(FU_FPU, width=fu_type_width), cycle=0)
    FU_FMISC_C = cas(domain, m.const(FU_FMISC, width=fu_type_width), cycle=0)
    FU_LDU_C = cas(domain, m.const(FU_LDU, width=fu_type_width), cycle=0)
    FU_STU_C = cas(domain, m.const(FU_STU, width=fu_type_width), cycle=0)

    # ── FU-type classification (element-wise Vector) ─────────────
    is_fp = (in_fu_type == FU_FPU_C) | (in_fu_type == FU_FMISC_C)
    is_mem = (in_fu_type == FU_LDU_C) | (in_fu_type == FU_STU_C)
    is_int = (~is_fp) & (~is_mem)

    # ── Check target IQ availability per slot ────────────────────
    slot_iq_ready = mux(
        is_int,
        iq_int_ready,
        mux(is_fp, iq_fp_ready, mux(is_mem, iq_mem_ready, ZERO_1)),
    )

    # ── Dispatch fire: any blocked valid slot stalls the group ───
    any_blocked = (in_valid & (~slot_iq_ready)).reduce_or()
    dispatch_fire = (~any_blocked) & (~flush)

    m.output(f"{prefix}_stall", wire_of(any_blocked))
    _out["stall"] = any_blocked

    slot_fire = in_valid & dispatch_fire
    int_fire = slot_fire & is_int
    fp_fire = slot_fire & is_fp
    mem_fire = slot_fire & is_mem

    # ── Vector dispatch / ROB outputs ────────────────────────────
    m.output(f"{prefix}_iq_int_valid", wire_of(int_fire))
    _out["iq_int_valid"] = int_fire
    m.output(f"{prefix}_iq_fp_valid", wire_of(fp_fire))
    _out["iq_fp_valid"] = fp_fire
    m.output(f"{prefix}_iq_mem_valid", wire_of(mem_fire))
    _out["iq_mem_valid"] = mem_fire

    m.output(f"{prefix}_out_pdest", wire_of(in_pdest))
    _out["out_pdest"] = in_pdest
    m.output(f"{prefix}_out_psrc1", wire_of(in_psrc1))
    _out["out_psrc1"] = in_psrc1
    m.output(f"{prefix}_out_psrc2", wire_of(in_psrc2))
    _out["out_psrc2"] = in_psrc2
    m.output(f"{prefix}_out_fu_type", wire_of(in_fu_type))
    _out["out_fu_type"] = in_fu_type
    m.output(f"{prefix}_out_rob_idx", wire_of(in_rob_idx))
    _out["out_rob_idx"] = in_rob_idx
    m.output(f"{prefix}_out_pc", wire_of(in_pc))
    _out["out_pc"] = in_pc

    m.output(f"{prefix}_rob_enq_valid", wire_of(slot_fire))
    _out["rob_enq_valid"] = slot_fire
    m.output(f"{prefix}_rob_enq_pdest", wire_of(in_pdest))
    _out["rob_enq_pdest"] = in_pdest
    m.output(f"{prefix}_rob_enq_old_pdest", wire_of(in_old_pdest))
    _out["rob_enq_old_pdest"] = in_old_pdest
    m.output(f"{prefix}_rob_enq_pc", wire_of(in_pc))
    _out["rob_enq_pc"] = in_pc

    # ── Count dispatched uops per IQ class (widen before reduce) ─
    int_cnt = int_fire.zext(dp_cnt_w).reduce_sum()
    fp_cnt = fp_fire.zext(dp_cnt_w).reduce_sum()
    mem_cnt = mem_fire.zext(dp_cnt_w).reduce_sum()

    m.output(f"{prefix}_int_dispatch_count", wire_of(int_cnt))
    _out["int_dispatch_count"] = int_cnt
    m.output(f"{prefix}_fp_dispatch_count", wire_of(fp_cnt))
    _out["fp_dispatch_count"] = fp_cnt
    m.output(f"{prefix}_mem_dispatch_count", wire_of(mem_cnt))
    _out["mem_dispatch_count"] = mem_cnt

    # ── Cycle 1: pipeline registers for downstream latching ──────
    domain.next()
    domain.cycle(wire_of(slot_fire), name=f"{prefix}_dp1_v")
    domain.cycle(wire_of(in_pdest), name=f"{prefix}_dp1_pdest")
    domain.cycle(wire_of(in_psrc1), name=f"{prefix}_dp1_psrc1")
    domain.cycle(wire_of(in_psrc2), name=f"{prefix}_dp1_psrc2")
    domain.cycle(wire_of(in_fu_type), name=f"{prefix}_dp1_fu")
    domain.cycle(wire_of(in_rob_idx), name=f"{prefix}_dp1_rob")
    return _out


dispatch.__pycircuit_name__ = "dispatch"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            dispatch,
            name="dispatch",
            eager=True,
            dispatch_width=2,
            fu_type_width=3,
            ptag_w=4,
            pc_width=16,
            rob_idx_w=4,
        ).emit_mlir()
    )
