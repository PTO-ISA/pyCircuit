"""SoC Peripheral stubs — PLIC and CLINT for XiangShan-pyc.

Simplified placeholder models for Platform-Level Interrupt Controller (PLIC)
and Core Local Interruptor (CLINT).  These provide the minimum port interface
for SoC integration without full protocol logic.

Reference:
  - RISC-V PLIC Specification v1.0
  - RISC-V Privileged Architecture (CLINT memory-mapped registers)

Key features:
  SOC-PLIC-001  Interrupt gateway: N source inputs → priority arbitration → target output
  SOC-PLIC-002  Claim/complete handshake (simplified to single-cycle)
  SOC-CLINT-001 Machine timer interrupt (mtip) based on mtime >= mtimecmp
  SOC-CLINT-002 Machine software interrupt (msip) via memory-mapped write
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

PLIC_NUM_SOURCES = 64
PLIC_NUM_TARGETS = 2
PLIC_PRIO_WIDTH = 3

CLINT_TIMER_WIDTH = 64


# ═══════════════════════════════════════════════════════════════════
#  PLIC — Platform-Level Interrupt Controller (stub)
# ═══════════════════════════════════════════════════════════════════


def plic(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "plic",
    num_sources: int = PLIC_NUM_SOURCES,
    num_targets: int = PLIC_NUM_TARGETS,
    prio_width: int = PLIC_PRIO_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """PLIC stub: priority-based interrupt routing from sources to targets."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    src_id_w = max(1, (num_sources - 1).bit_length())

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    irq_pending = (
        _in["irq_pending"]
        if "irq_pending" in _in
        else cas(domain, m.input(f"{prefix}_irq_pending", width=num_sources), cycle=0)
    )
    irq_enable = (
        _in["irq_enable"]
        if "irq_enable" in _in
        else cas(domain, m.input(f"{prefix}_irq_enable", width=num_sources), cycle=0)
    )

    claim_valid = (
        _in["claim_valid"]
        if "claim_valid" in _in
        else cas(domain, m.input(f"{prefix}_claim_valid", width=1), cycle=0)
    )
    complete_valid = (
        _in["complete_valid"]
        if "complete_valid" in _in
        else cas(domain, m.input(f"{prefix}_complete_valid", width=1), cycle=0)
    )
    complete_id = (
        _in["complete_id"]
        if "complete_id" in _in
        else cas(domain, m.input(f"{prefix}_complete_id", width=src_id_w), cycle=0)
    )

    threshold = (
        _in["threshold"]
        if "threshold" in _in
        else cas(domain, m.input(f"{prefix}_threshold", width=prio_width), cycle=0)
    )

    # ── State ────────────────────────────────────────────────────
    claimed = domain.signal(width=num_sources, reset_value=0, name=f"{prefix}_claimed")

    # Per-source priority (simplified: stored as state, writable)
    src_prio = [
        domain.signal(width=prio_width, reset_value=0, name=f"{prefix}_prio_{i}")
        for i in range(min(num_sources, 8))
    ]

    # ── Cycle 0: Combinational — find highest-priority pending ───
    def _const(val, w):
        return cas(domain, m.const(val, width=w), cycle=0)

    ZERO_1 = _const(0, 1)
    ONE_1 = _const(1, 1)

    effective_pending = irq_pending & irq_enable & (~claimed)

    # Simple priority scan (lowest index with pending wins — stub)
    best_id = _const(0, src_id_w)
    any_irq = ZERO_1
    for i in range(min(num_sources, 8)):
        bit_i = effective_pending[i : i + 1]
        prio_above = src_prio[i] if i < len(src_prio) else _const(1, prio_width)
        above_thresh = prio_above > threshold if i < len(src_prio) else ONE_1
        take = bit_i & above_thresh & (~any_irq)
        best_id = mux(take, _const(i, src_id_w), best_id)
        any_irq = any_irq | take

    # ── Outputs ──────────────────────────────────────────────────
    m.output(f"{prefix}_irq_out", wire_of(any_irq))
    _out["irq_out"] = any_irq
    m.output(f"{prefix}_irq_id", wire_of(best_id))
    _out["irq_id"] = best_id

    # ── Cycle 1: State updates ───────────────────────────────────
    domain.next()

    # On claim: set claimed bit for best_id
    claim_mask = _const(0, num_sources)
    for i in range(min(num_sources, 8)):
        hit = best_id == _const(i, src_id_w)
        bit = mux(
            hit & claim_valid & any_irq,
            _const(1 << i, num_sources),
            _const(0, num_sources),
        )
        claim_mask = claim_mask | bit

    # On complete: clear claimed bit for complete_id
    complete_mask = _const(0, num_sources)
    for i in range(min(num_sources, 8)):
        hit = complete_id == _const(i, src_id_w)
        bit = mux(
            hit & complete_valid, _const(1 << i, num_sources), _const(0, num_sources)
        )
        complete_mask = complete_mask | bit

    new_claimed = (claimed | claim_mask) & (~complete_mask)
    claimed <<= new_claimed
    return _out


plic.__pycircuit_name__ = "plic"


# ═══════════════════════════════════════════════════════════════════
#  CLINT — Core Local Interruptor (stub)
# ═══════════════════════════════════════════════════════════════════


def clint(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "clint",
    timer_width: int = CLINT_TIMER_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """CLINT stub: timer interrupt (mtip) and software interrupt (msip)."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    # ── Cycle 0: Inputs ──────────────────────────────────────────

    # Software interrupt write
    msip_write_valid = (
        _in["msip_write_valid"]
        if "msip_write_valid" in _in
        else cas(domain, m.input(f"{prefix}_msip_write_valid", width=1), cycle=0)
    )
    msip_write_data = (
        _in["msip_write_data"]
        if "msip_write_data" in _in
        else cas(domain, m.input(f"{prefix}_msip_write_data", width=1), cycle=0)
    )

    # Timer compare register write
    mtimecmp_write_valid = (
        _in["mtimecmp_write_valid"]
        if "mtimecmp_write_valid" in _in
        else cas(domain, m.input(f"{prefix}_mtimecmp_write_valid", width=1), cycle=0)
    )
    mtimecmp_write_data = (
        _in["mtimecmp_write_data"]
        if "mtimecmp_write_data" in _in
        else cas(
            domain, m.input(f"{prefix}_mtimecmp_write_data", width=timer_width), cycle=0
        )
    )

    # ── State ────────────────────────────────────────────────────

    mtime = domain.signal(width=timer_width, reset_value=0, name=f"{prefix}_mtime")
    mtimecmp = domain.signal(
        width=timer_width, reset_value=0, name=f"{prefix}_mtimecmp"
    )
    msip_reg = domain.signal(width=1, reset_value=0, name=f"{prefix}_msip")

    # ── Cycle 0: Combinational ───────────────────────────────────

    def _const(val, w):
        return cas(domain, m.const(val, width=w), cycle=0)

    # Timer interrupt: mtime >= mtimecmp
    # Unsigned comparison: mtip = ~(mtime < mtimecmp)
    time_lt_cmp = mtime < mtimecmp
    mtip = ~time_lt_cmp

    # ── Outputs ──────────────────────────────────────────────────
    m.output(f"{prefix}_mtip", wire_of(mtip))
    _out["mtip"] = mtip
    m.output(f"{prefix}_msip", wire_of(msip_reg))
    _out["msip"] = msip_reg
    m.output(f"{prefix}_mtime_out", wire_of(mtime))
    _out["mtime_out"] = mtime

    # ── Cycle 1: State updates ───────────────────────────────────
    domain.next()

    one = _const(1, timer_width)

    # mtime increments every cycle (free-running counter)
    next_mtime = cas(domain, (wire_of(mtime) + wire_of(one))[0:timer_width], cycle=0)
    mtime <<= next_mtime

    # mtimecmp updated on write
    mtimecmp <<= mux(mtimecmp_write_valid, mtimecmp_write_data, mtimecmp)

    # msip updated on write
    msip_reg <<= mux(msip_write_valid, msip_write_data, msip_reg)
    return _out


clint.__pycircuit_name__ = "clint"


# ═══════════════════════════════════════════════════════════════════
#  Standalone emission
# ═══════════════════════════════════════════════════════════════════

if __name__ == "__main__":

    pass
