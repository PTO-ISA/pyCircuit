"""TLB — Translation Lookaside Buffer for XiangShan-pyc.

Fully-associative TLB for virtual-to-physical address translation.
2-cycle pipeline: cycle 0 (CAM lookup), cycle 1 (result output).

Reference: XiangShan/src/main/scala/xiangshan/cache/mmu/TLB.scala

Pipeline:
  c0  Accept lookup request, compare VPN+ASID against all entries (CAM)
  c1  Output PPN on hit; raise miss signal for PTW request

Key parameters (from XiangShan KunMingHu defaults):
  nWays=48, vpnWidth=27 (Sv39), ppnWidth=24, asidWidth=16

Simplified vs full XiangShan:
  - 4KB pages only (no superpage / 2MB / 1GB support)
  - No separate normal-page / super-page sections
  - Round-robin replacement (no pseudo-LRU)
  - Single lookup port (XiangShan has per-pipeline TLBs)
  - No A/D bit management (handled by page-fault exceptions)
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
    wire_of,
)
from top.parameters import *


def tlb(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "tlb",
    n_ways: int = ITLB_WAYS,
    vpn_width: int = 27,
    ppn_width: int = 24,
    asid_width: int = ASID_LENGTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """TLB: fully-associative translation lookaside buffer with 2-cycle pipeline."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    way_bits = max(1, (n_ways - 1).bit_length())

    # ================================================================
    # Cycle 0 — CAM Lookup: compare VPN+ASID against all entries
    # ================================================================

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )
    flush_asid_valid = (
        _in["flush_asid_valid"]
        if "flush_asid_valid" in _in
        else cas(domain, m.input(f"{prefix}_flush_asid_valid", width=1), cycle=0)
    )
    flush_asid = (
        _in["flush_asid"]
        if "flush_asid" in _in
        else cas(domain, m.input(f"{prefix}_flush_asid", width=asid_width), cycle=0)
    )

    lookup_valid = (
        _in["lookup_valid"]
        if "lookup_valid" in _in
        else cas(domain, m.input(f"{prefix}_lookup_valid", width=1), cycle=0)
    )
    lookup_vpn = (
        _in["lookup_vpn"]
        if "lookup_vpn" in _in
        else cas(domain, m.input(f"{prefix}_lookup_vpn", width=vpn_width), cycle=0)
    )
    lookup_asid = (
        _in["lookup_asid"]
        if "lookup_asid" in _in
        else cas(domain, m.input(f"{prefix}_lookup_asid", width=asid_width), cycle=0)
    )

    refill_valid = (
        _in["refill_valid"]
        if "refill_valid" in _in
        else cas(domain, m.input(f"{prefix}_refill_valid", width=1), cycle=0)
    )
    refill_vpn = (
        _in["refill_vpn"]
        if "refill_vpn" in _in
        else cas(domain, m.input(f"{prefix}_refill_vpn", width=vpn_width), cycle=0)
    )
    refill_ppn = (
        _in["refill_ppn"]
        if "refill_ppn" in _in
        else cas(domain, m.input(f"{prefix}_refill_ppn", width=ppn_width), cycle=0)
    )
    refill_asid = (
        _in["refill_asid"]
        if "refill_asid" in _in
        else cas(domain, m.input(f"{prefix}_refill_asid", width=asid_width), cycle=0)
    )

    # ── Entry storage (fully-associative CAM) ─────────────────────

    entry_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_e{i}_v")
        for i in range(n_ways)
    ]
    entry_vpn = [
        domain.signal(width=vpn_width, reset_value=0, name=f"{prefix}_e{i}_vpn")
        for i in range(n_ways)
    ]
    entry_ppn = [
        domain.signal(width=ppn_width, reset_value=0, name=f"{prefix}_e{i}_ppn")
        for i in range(n_ways)
    ]
    entry_asid = [
        domain.signal(width=asid_width, reset_value=0, name=f"{prefix}_e{i}_asid")
        for i in range(n_ways)
    ]

    replace_ptr = domain.signal(
        width=way_bits, reset_value=0, name=f"{prefix}_repl_ptr"
    )

    # ── Read entries as CAS signals at cycle 0 ────────────────────

    ev = [entry_valid[i] for i in range(n_ways)]
    evpn = [entry_vpn[i] for i in range(n_ways)]
    eppn = [entry_ppn[i] for i in range(n_ways)]
    easid = [entry_asid[i] for i in range(n_ways)]

    # ── Per-entry CAM match ───────────────────────────────────────

    way_hit = []
    for i in range(n_ways):
        vpn_eq = evpn[i] == lookup_vpn
        asid_eq = easid[i] == lookup_asid
        way_hit.append(ev[i] & vpn_eq & asid_eq)

    any_hit = way_hit[0]
    for i in range(1, n_ways):
        any_hit = any_hit | way_hit[i]

    # ── Select PPN from matching entry ────────────────────────────

    hit_ppn = eppn[0]
    for i in range(1, n_ways):
        hit_ppn = mux(way_hit[i], eppn[i], hit_ppn)

    miss = lookup_valid & (~any_hit)

    # ── Pipeline registers c0 → c1 ───────────────────────────────

    s1_valid_w = domain.cycle(wire_of(lookup_valid), name=f"{prefix}_s1_v")
    s1_hit_w = domain.cycle(wire_of(any_hit), name=f"{prefix}_s1_hit")
    s1_miss_w = domain.cycle(wire_of(miss), name=f"{prefix}_s1_miss")
    s1_ppn_w = domain.cycle(wire_of(hit_ppn), name=f"{prefix}_s1_ppn")
    s1_vpn_w = domain.cycle(wire_of(lookup_vpn), name=f"{prefix}_s1_vpn")

    domain.next()  # ─────────────── c0 → c1 boundary ──────────────

    # ================================================================
    # Cycle 1 — Output (results already in pipeline registers)
    # ================================================================

    domain.next()  # ─────────────── c1 → state-update boundary ────

    # ================================================================
    # State Updates
    # ================================================================

    rptr = wire_of(replace_ptr)
    zero1 = m.const(0, width=1)
    one1 = m.const(1, width=1)

    for i in range(n_ways):
        i_const = m.const(i, width=way_bits)
        is_victim = rptr == i_const
        do_write = wire_of(refill_valid) & is_victim

        asid_match = wire_of(entry_asid[i]) == wire_of(flush_asid)
        selective_flush = wire_of(flush_asid_valid) & asid_match
        invalidate = wire_of(flush) | selective_flush

        new_valid = do_write.select(one1, wire_of(entry_valid[i]))
        new_valid = invalidate.select(zero1, new_valid)
        entry_valid[i] <<= new_valid

        entry_vpn[i] <<= do_write.select(wire_of(refill_vpn), wire_of(entry_vpn[i]))
        entry_ppn[i] <<= do_write.select(wire_of(refill_ppn), wire_of(entry_ppn[i]))
        entry_asid[i] <<= do_write.select(wire_of(refill_asid), wire_of(entry_asid[i]))

    # Advance replace pointer on refill (wrap at n_ways)
    at_limit = rptr == m.const(n_ways - 1, width=way_bits)
    next_ptr = at_limit.select(
        m.const(0, width=way_bits),
        (rptr + m.const(1, width=way_bits))[0:way_bits],
    )
    replace_ptr <<= wire_of(refill_valid).select(next_ptr, rptr)

    # ================================================================
    # Output ports
    # ================================================================

    m.output(f"{prefix}_resp_valid", s1_valid_w)
    _out["resp_valid"] = cas(domain, s1_valid_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_resp_hit", s1_hit_w)
    _out["resp_hit"] = cas(domain, s1_hit_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_resp_miss", s1_miss_w)
    _out["resp_miss"] = cas(domain, s1_miss_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_resp_ppn", s1_ppn_w)
    _out["resp_ppn"] = cas(domain, s1_ppn_w, cycle=domain.cycle_index)

    m.output(f"{prefix}_ptw_req_valid", s1_miss_w)
    _out["ptw_req_valid"] = cas(domain, s1_miss_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_ptw_req_vpn", s1_vpn_w)
    _out["ptw_req_vpn"] = cas(domain, s1_vpn_w, cycle=domain.cycle_index)
    return _out


tlb.__pycircuit_name__ = "tlb"


if __name__ == "__main__":
    pass
