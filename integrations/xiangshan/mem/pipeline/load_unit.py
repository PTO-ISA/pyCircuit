"""Load Unit — 3-stage load pipeline for XiangShan-pyc MemBlock.

Pipeline:
  s0  Address generation: compute virtual address, issue TLB lookup
  s1  DCache access: use translated physical address to read DCache
  s2  Data return: select data (cache hit / forwarded), writeback result

Reference: XiangShan/src/main/scala/xiangshan/mem/pipeline/LoadUnit.scala

Key features:
  M-LU-001  3-stage pipelined load execution
  M-LU-002  TLB lookup in s0, DCache access in s1
  M-LU-003  Store-to-load forwarding check in s2
  M-LU-004  Pipeline kill on redirect (branch misprediction)
  M-LU-005  Writeback to register file with rob_idx tracking
"""

from __future__ import annotations

import math
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


def load_unit(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ldu",
    data_width: int = XLEN,
    addr_width: int = PC_WIDTH,
    rob_idx_width: int = ROB_IDX_WIDTH,
    lq_idx_width: int = LQ_IDX_WIDTH,
    ppn_width: int = 24,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Load Unit: 3-stage pipeline (addr-gen → TLB/DCache → writeback)."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    int(math.log2(CACHE_LINE_BYTES))

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
    issue_rob_idx = (
        _in["issue_rob_idx"]
        if "issue_rob_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_issue_rob_idx", width=rob_idx_width), cycle=0
        )
    )
    issue_lq_idx = (
        _in["issue_lq_idx"]
        if "issue_lq_idx" in _in
        else cas(domain, m.input(f"{prefix}_issue_lq_idx", width=lq_idx_width), cycle=0)
    )

    # Store buffer forwarding interface (checked in s2)
    fwd_valid = (
        _in["fwd_valid"]
        if "fwd_valid" in _in
        else cas(domain, m.input(f"{prefix}_fwd_valid", width=1), cycle=0)
    )
    fwd_data = (
        _in["fwd_data"]
        if "fwd_data" in _in
        else cas(domain, m.input(f"{prefix}_fwd_data", width=data_width), cycle=0)
    )

    # TLB response (arrives in s1 for s0's request)
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

    # DCache response (arrives in s2)
    dcache_resp_valid = (
        _in["dcache_resp_valid"]
        if "dcache_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_dcache_resp_valid", width=1), cycle=0)
    )
    dcache_resp_data = (
        _in["dcache_resp_data"]
        if "dcache_resp_data" in _in
        else cas(
            domain, m.input(f"{prefix}_dcache_resp_data", width=data_width), cycle=0
        )
    )

    s0_fire = issue_valid & (~flush)
    s0_vpn = issue_addr[12 : 12 + (addr_width - 12)]

    # TLB request
    m.output(f"{prefix}_tlb_req_valid", wire_of(s0_fire))
    _out["tlb_req_valid"] = s0_fire
    m.output(f"{prefix}_tlb_req_vpn", wire_of(s0_vpn))
    _out["tlb_req_vpn"] = s0_vpn

    # ── Pipeline registers s0 → s1 ───────────────────────────────

    s1_valid_w = domain.cycle(wire_of(s0_fire), name=f"{prefix}_s1_v")
    s1_addr_w = domain.cycle(wire_of(issue_addr), name=f"{prefix}_s1_addr")
    s1_rob_idx_w = domain.cycle(wire_of(issue_rob_idx), name=f"{prefix}_s1_rob")
    s1_lq_idx_w = domain.cycle(wire_of(issue_lq_idx), name=f"{prefix}_s1_lq")

    domain.next()  # ─────────────── s0 → s1 boundary ──────────────

    # ================================================================
    # s1 — TLB result + DCache access
    # ================================================================

    s1_kill = wire_of(flush)
    s1_alive = s1_valid_w & (~s1_kill)
    s1_tlb_miss = s1_alive & (~wire_of(tlb_resp_hit))

    paddr_lo = s1_addr_w[0:12]
    s1_paddr = m.cat(wire_of(tlb_resp_ppn), paddr_lo)
    ppn_width + 12

    s1_dcache_fire = s1_alive & wire_of(tlb_resp_hit)

    m.output(f"{prefix}_dcache_req_valid", s1_dcache_fire)
    _out["dcache_req_valid"] = cas(domain, s1_dcache_fire, cycle=domain.cycle_index)
    m.output(f"{prefix}_dcache_req_addr", s1_paddr)
    _out["dcache_req_addr"] = cas(domain, s1_paddr, cycle=domain.cycle_index)

    # Load queue update with translated address
    m.output(f"{prefix}_lq_update_valid", s1_alive)
    _out["lq_update_valid"] = cas(domain, s1_alive, cycle=domain.cycle_index)
    m.output(f"{prefix}_lq_update_idx", s1_lq_idx_w)
    _out["lq_update_idx"] = cas(domain, s1_lq_idx_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_lq_update_addr", s1_paddr)
    _out["lq_update_addr"] = cas(domain, s1_paddr, cycle=domain.cycle_index)

    # ── Pipeline registers s1 → s2 ───────────────────────────────

    s2_valid_w = domain.cycle(s1_dcache_fire, name=f"{prefix}_s2_v")
    s2_rob_idx_w = domain.cycle(s1_rob_idx_w, name=f"{prefix}_s2_rob")
    s2_lq_idx_w = domain.cycle(s1_lq_idx_w, name=f"{prefix}_s2_lq")
    domain.cycle(s1_paddr, name=f"{prefix}_s2_pa")

    domain.next()  # ─────────────── s1 → s2 boundary ──────────────

    # ================================================================
    # s2 — Data return + forwarding check + writeback
    # ================================================================

    s2_kill = wire_of(flush)
    s2_alive = s2_valid_w & (~s2_kill)

    s2_data = wire_of(fwd_valid).select(wire_of(fwd_data), wire_of(dcache_resp_data))
    s2_hit = wire_of(dcache_resp_valid) | wire_of(fwd_valid)

    wb_valid = s2_alive & s2_hit
    wb_data = s2_data

    domain.next()  # ─────────────── s2 → output boundary ─────────

    # ================================================================
    # Output ports
    # ================================================================

    m.output(f"{prefix}_wb_valid", wb_valid)
    _out["wb_valid"] = cas(domain, wb_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_wb_data", wb_data)
    _out["wb_data"] = cas(domain, wb_data, cycle=domain.cycle_index)
    m.output(f"{prefix}_wb_rob_idx", s2_rob_idx_w)
    _out["wb_rob_idx"] = cas(domain, s2_rob_idx_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_wb_lq_idx", s2_lq_idx_w)
    _out["wb_lq_idx"] = cas(domain, s2_lq_idx_w, cycle=domain.cycle_index)

    m.output(f"{prefix}_tlb_miss", s1_tlb_miss)
    _out["tlb_miss"] = cas(domain, s1_tlb_miss, cycle=domain.cycle_index)
    return _out


load_unit.__pycircuit_name__ = "load_unit"


if __name__ == "__main__":
    pass
