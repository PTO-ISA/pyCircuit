"""IFU — Instruction Fetch Unit for XiangShan-pyc.

3-stage pipeline bridging FTQ/ICache and IBuffer.

Pipeline:
  s0  Accept fetch target from FTQ, send request to ICache
  s1  Receive ICache response, gate with pipeline valid
  s2  Pre-decode (RVC detection, branch type), output to IBuffer

Reference: XiangShan/src/main/scala/xiangshan/frontend/ifu/

Key features:
  F-IFU-001  3-stage fetch pipeline (s0 / s1 / s2)
  F-IFU-002  FTQ→ICache request forwarding
  F-IFU-003  Per-slot RVC detection and 32-bit instruction assembly
  F-IFU-004  Pre-decode: branch / jal / jalr type detection
  F-IFU-005  Pipeline flush on redirect
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
    wire_of,
)
from top.parameters import (
    CACHE_LINE_SIZE,
    FETCH_BLOCK_INST_NUM,
    PC_WIDTH,
)

FETCH_WIDTH = FETCH_BLOCK_INST_NUM  # 32 half-word (16-bit) slots
CACHE_DATA_WIDTH = CACHE_LINE_SIZE  # 512 bits
PARCEL_WIDTH = 16
INST_WIDTH = 32


def ifu(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ifu",
    fetch_width: int = FETCH_WIDTH,
    pc_width: int = PC_WIDTH,
    cache_data_width: int = CACHE_DATA_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """IFU: 3-stage instruction fetch pipeline (FTQ → ICache → IBuffer)."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    num_w = max(1, (fetch_width).bit_length())

    # ── All inputs (declared at cycle 0) ──────────────────────────────

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    # FTQ → IFU
    ftq_valid = (
        _in["ftq_to_ifu_valid"]
        if "ftq_to_ifu_valid" in _in
        else cas(domain, m.input(f"{prefix}_ftq_to_ifu_valid", width=1), cycle=0)
    )
    ftq_pc = (
        _in["ftq_to_ifu_pc"]
        if "ftq_to_ifu_pc" in _in
        else cas(domain, m.input(f"{prefix}_ftq_to_ifu_pc", width=pc_width), cycle=0)
    )
    ftq_target = (
        _in["ftq_to_ifu_target"]
        if "ftq_to_ifu_target" in _in
        else cas(
            domain, m.input(f"{prefix}_ftq_to_ifu_target", width=pc_width), cycle=0
        )
    )

    # ICache → IFU (response)
    icache_resp_valid = (
        _in["icache_resp_valid"]
        if "icache_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_icache_resp_valid", width=1), cycle=0)
    )
    icache_resp_data = (
        _in["icache_resp_data"]
        if "icache_resp_data" in _in
        else cas(
            domain,
            m.input(f"{prefix}_icache_resp_data", width=cache_data_width),
            cycle=0,
        )
    )
    icache_resp_hit = (
        _in["icache_resp_hit"]
        if "icache_resp_hit" in _in
        else cas(domain, m.input(f"{prefix}_icache_resp_hit", width=1), cycle=0)
    )

    # ================================================================
    # s0 — Accept FTQ fetch target, send ICache request
    # ================================================================

    s0_valid = ftq_valid & (~flush)

    m.output(f"{prefix}_icache_req_valid", wire_of(s0_valid))
    _out["icache_req_valid"] = s0_valid
    m.output(f"{prefix}_icache_req_vaddr", wire_of(ftq_pc))
    _out["icache_req_vaddr"] = ftq_pc
    m.output(
        f"{prefix}_ftq_to_ifu_ready", wire_of(cas(domain, m.const(1, width=1), cycle=0))
    )

    # Pipeline registers s0 → s1
    s1_v_r = domain.cycle(wire_of(s0_valid), name=f"{prefix}_s1_v")
    s1_pc_r = domain.cycle(wire_of(ftq_pc), name=f"{prefix}_s1_pc")
    s1_tgt_r = domain.cycle(wire_of(ftq_target), name=f"{prefix}_s1_tgt")

    domain.next()  # ──────────────── s0 → s1 ────────────────

    # ================================================================
    # s1 — ICache response arrives; gate with pipeline valid
    # ================================================================

    s1_fire = (
        s1_v_r
        & wire_of(icache_resp_valid)
        & wire_of(icache_resp_hit)
        & (~wire_of(flush))
    )

    # Pipeline registers s1 → s2
    s2_v_r = domain.cycle(s1_fire, name=f"{prefix}_s2_v")
    s2_pc_r = domain.cycle(s1_pc_r, name=f"{prefix}_s2_pc")
    domain.cycle(s1_tgt_r, name=f"{prefix}_s2_tgt")
    s2_data_r = domain.cycle(wire_of(icache_resp_data), name=f"{prefix}_s2_data")

    domain.next()  # ──────────────── s1 → s2 ────────────────

    # ================================================================
    # s2 — Pre-decode: RVC detection, instruction assembly, output
    # ================================================================

    out_valid = s2_v_r & (~wire_of(flush))
    m.output(f"{prefix}_ifu_to_ibuf_valid", out_valid)
    _out["ifu_to_ibuf_valid"] = cas(domain, out_valid, cycle=domain.cycle_index)

    for i in range(fetch_width):
        lo = i * PARCEL_WIDTH
        hi = lo + PARCEL_WIDTH

        parcel = s2_data_r[lo:hi]

        # RVC detection: bits[1:0] != 0b11 → compressed instruction
        is_rvc = ~(parcel[0:2] == m.const(3, width=2))

        # Slot PC = base_pc + slot_index * 2 bytes
        slot_pc = (s2_pc_r + m.const(i * 2, width=pc_width))[0:pc_width]

        # Assemble 32-bit instruction from adjacent 16-bit parcels
        if i < fetch_width - 1:
            nxt_lo = (i + 1) * PARCEL_WIDTH
            nxt_hi = nxt_lo + PARCEL_WIDTH
            nxt_parcel = s2_data_r[nxt_lo:nxt_hi]
        else:
            nxt_parcel = m.const(0, width=PARCEL_WIDTH)

        rvi_inst = m.cat(nxt_parcel, parcel)
        rvc_inst = m.cat(m.const(0, width=PARCEL_WIDTH), parcel)
        inst = is_rvc.select(rvc_inst, rvi_inst)

        # Pre-decode: detect branch type from opcode field
        opcode = rvi_inst[0:7]
        is_br = opcode == m.const(0x63, width=7)
        is_jal = opcode == m.const(0x6F, width=7)
        is_jalr = opcode == m.const(0x67, width=7)
        pred_is_br = is_br | is_jal | is_jalr

        m.output(f"{prefix}_out_valid_{i}", out_valid)
        m.output(f"{prefix}_out_inst_{i}", inst)
        m.output(f"{prefix}_out_pc_{i}", slot_pc)
        m.output(f"{prefix}_out_is_rvc_{i}", is_rvc)
        m.output(f"{prefix}_out_is_br_{i}", pred_is_br)

    out_num = out_valid.select(
        m.const(fetch_width, width=num_w),
        m.const(0, width=num_w),
    )
    m.output(f"{prefix}_out_num", out_num)
    _out["out_num"] = cas(domain, out_num, cycle=domain.cycle_index)
    return _out


ifu.__pycircuit_name__ = "ifu"


if __name__ == "__main__":
    pass
