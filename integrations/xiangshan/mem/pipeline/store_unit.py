"""Store Unit — 2-stage store pipeline for XiangShan-pyc MemBlock.

Pipeline:
  s0  Address generation: compute virtual address, issue TLB lookup
  s1  TLB result: write translated address + data to store queue

Reference: XiangShan/src/main/scala/xiangshan/mem/pipeline/StoreUnit.scala

Key features:
  M-SU-001  2-stage pipelined store execution
  M-SU-002  TLB lookup in s0, store queue write in s1
  M-SU-003  Pipeline kill on redirect
  M-SU-004  Writeback acknowledgement with rob_idx
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
from top.parameters import *


def store_unit(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "stu",
    data_width: int = XLEN,
    addr_width: int = PC_WIDTH,
    rob_idx_width: int = ROB_IDX_WIDTH,
    sq_idx_width: int = SQ_IDX_WIDTH,
    ppn_width: int = 24,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Store Unit: 2-stage pipeline (addr-gen/TLB → store queue write)."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    # ================================================================
    # s0 — Address Generation + TLB request
    # ================================================================

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    issue_valid = (
        _in["issue_valid"]
        if "issue_valid" in _in
        else cas(domain, m.input(f"{prefix}_issue_valid", width=1), cycle=0)
    )
    issue_addr = (
        _in["issue_addr"]
        if "issue_addr" in _in
        else cas(domain, m.input(f"{prefix}_issue_addr", width=addr_width), cycle=0)
    )
    issue_data = (
        _in["issue_data"]
        if "issue_data" in _in
        else cas(domain, m.input(f"{prefix}_issue_data", width=data_width), cycle=0)
    )
    issue_rob_idx = (
        _in["issue_rob_idx"]
        if "issue_rob_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_issue_rob_idx", width=rob_idx_width), cycle=0
        )
    )
    issue_sq_idx = (
        _in["issue_sq_idx"]
        if "issue_sq_idx" in _in
        else cas(domain, m.input(f"{prefix}_issue_sq_idx", width=sq_idx_width), cycle=0)
    )

    # TLB response (arrives in s1)
    tlb_resp_hit = (
        _in["tlb_resp_hit"]
        if "tlb_resp_hit" in _in
        else cas(domain, m.input(f"{prefix}_tlb_resp_hit", width=1), cycle=0)
    )
    tlb_resp_ppn = (
        _in["tlb_resp_ppn"]
        if "tlb_resp_ppn" in _in
        else cas(domain, m.input(f"{prefix}_tlb_resp_ppn", width=ppn_width), cycle=0)
    )

    s0_fire = issue_valid & (~flush)
    s0_vpn = issue_addr[12 : 12 + (addr_width - 12)]

    m.output(f"{prefix}_tlb_req_valid", wire_of(s0_fire))
    _out["tlb_req_valid"] = s0_fire
    m.output(f"{prefix}_tlb_req_vpn", wire_of(s0_vpn))
    _out["tlb_req_vpn"] = s0_vpn

    # ── Pipeline registers s0 → s1 ───────────────────────────────

    s1_valid_w = domain.cycle(wire_of(s0_fire), name=f"{prefix}_s1_v")
    s1_addr_w = domain.cycle(wire_of(issue_addr), name=f"{prefix}_s1_addr")
    s1_data_w = domain.cycle(wire_of(issue_data), name=f"{prefix}_s1_data")
    s1_rob_idx_w = domain.cycle(wire_of(issue_rob_idx), name=f"{prefix}_s1_rob")
    s1_sq_idx_w = domain.cycle(wire_of(issue_sq_idx), name=f"{prefix}_s1_sq")

    domain.next()  # ─────────────── s0 → s1 boundary ──────────────

    # ================================================================
    # s1 — TLB result + Store Queue write
    # ================================================================

    s1_kill = wire_of(flush)
    s1_alive = s1_valid_w & (~s1_kill)
    s1_tlb_miss = s1_alive & (~wire_of(tlb_resp_hit))

    paddr_lo = s1_addr_w[0:12]
    s1_paddr = m.cat(wire_of(tlb_resp_ppn), paddr_lo)

    s1_sq_fire = s1_alive & wire_of(tlb_resp_hit)

    domain.next()  # ─────────────── s1 → output boundary ─────────

    # ================================================================
    # Output ports
    # ================================================================

    m.output(f"{prefix}_sq_write_valid", s1_sq_fire)
    _out["sq_write_valid"] = cas(domain, s1_sq_fire, cycle=domain.cycle_index)
    m.output(f"{prefix}_sq_write_idx", s1_sq_idx_w)
    _out["sq_write_idx"] = cas(domain, s1_sq_idx_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_sq_write_addr", s1_paddr)
    _out["sq_write_addr"] = cas(domain, s1_paddr, cycle=domain.cycle_index)
    m.output(f"{prefix}_sq_write_data", s1_data_w)
    _out["sq_write_data"] = cas(domain, s1_data_w, cycle=domain.cycle_index)

    m.output(f"{prefix}_wb_valid", s1_sq_fire)
    _out["wb_valid"] = cas(domain, s1_sq_fire, cycle=domain.cycle_index)
    m.output(f"{prefix}_wb_rob_idx", s1_rob_idx_w)
    _out["wb_rob_idx"] = cas(domain, s1_rob_idx_w, cycle=domain.cycle_index)

    m.output(f"{prefix}_tlb_miss", s1_tlb_miss)
    _out["tlb_miss"] = cas(domain, s1_tlb_miss, cycle=domain.cycle_index)
    return _out


store_unit.__pycircuit_name__ = "store_unit"


if __name__ == "__main__":
    pass
