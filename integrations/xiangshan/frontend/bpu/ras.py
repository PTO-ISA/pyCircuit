"""RAS — Return Address Stack for XiangShan-pyc.

Speculative RAS with a commit stack for misprediction recovery.
Push on call instructions, pop on return instructions.
Each entry stores an address and a call-count for recursive calls
(successive pushes of the same address increment the counter instead
of advancing the stack pointer).

Reference: XiangShan/src/main/scala/xiangshan/frontend/bpu/RAS.scala
           XiangShan-doc/docs/frontend/bp.md  §RAS

Key features:
  F-RA-001  Speculative push/pop with stack pointer
  F-RA-002  Commit stack for recovery on misprediction
  F-RA-003  Recursive-call counter (same-address compression)
  F-RA-004  Overflow/underflow pointer wrapping
  F-RA-005  Snapshot save/restore via external FTQ
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
    mux,
    u,
    wire_of,
)
from top.parameters import PC_WIDTH, RAS_COMMIT_STACK_SIZE, RAS_SPEC_QUEUE_SIZE


def ras(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ras",
    commit_size: int = RAS_COMMIT_STACK_SIZE,
    spec_size: int = RAS_SPEC_QUEUE_SIZE,
    pc_width: int = PC_WIDTH,
    ctr_width: int = 3,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """RAS: speculative return address stack with commit-level recovery."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    spec_ptr_w = max(1, math.ceil(math.log2(spec_size)))
    commit_ptr_w = max(1, math.ceil(math.log2(commit_size)))
    ctr_max = (1 << ctr_width) - 1

    # ── Cycle 0: Inputs ──────────────────────────────────────────────
    s0_fire = (
        _in["s0_fire"]
        if "s0_fire" in _in
        else cas(domain, m.input(f"{prefix}_s0_fire", width=1), cycle=0)
    )
    do_push = (
        _in["do_push"]
        if "do_push" in _in
        else cas(domain, m.input(f"{prefix}_do_push", width=1), cycle=0)
    )
    do_pop = (
        _in["do_pop"]
        if "do_pop" in _in
        else cas(domain, m.input(f"{prefix}_do_pop", width=1), cycle=0)
    )
    push_addr = (
        _in["push_addr"]
        if "push_addr" in _in
        else cas(domain, m.input(f"{prefix}_push_addr", width=pc_width), cycle=0)
    )

    commit_push = (
        _in["commit_push"]
        if "commit_push" in _in
        else cas(domain, m.input(f"{prefix}_commit_push", width=1), cycle=0)
    )
    commit_pop = (
        _in["commit_pop"]
        if "commit_pop" in _in
        else cas(domain, m.input(f"{prefix}_commit_pop", width=1), cycle=0)
    )
    commit_push_addr = (
        _in["commit_push_addr"]
        if "commit_push_addr" in _in
        else cas(domain, m.input(f"{prefix}_commit_push_addr", width=pc_width), cycle=0)
    )

    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_sp = (
        _in["redirect_sp"]
        if "redirect_sp" in _in
        else cas(domain, m.input(f"{prefix}_redirect_sp", width=spec_ptr_w), cycle=0)
    )
    redirect_top_addr = (
        _in["redirect_top_addr"]
        if "redirect_top_addr" in _in
        else cas(
            domain, m.input(f"{prefix}_redirect_top_addr", width=pc_width), cycle=0
        )
    )
    redirect_top_ctr = (
        _in["redirect_top_ctr"]
        if "redirect_top_ctr" in _in
        else cas(
            domain, m.input(f"{prefix}_redirect_top_ctr", width=ctr_width), cycle=0
        )
    )

    cas(domain, m.const(0, width=1), cycle=0)
    cas(domain, m.const(1, width=1), cycle=0)
    zero_pc = cas(domain, m.const(0, width=pc_width), cycle=0)

    # ── Speculative stack storage ────────────────────────────────────
    spec_addr = [
        domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_sa_{i}")
        for i in range(spec_size)
    ]
    spec_ctr = [
        domain.signal(width=ctr_width, reset_value=0, name=f"{prefix}_sc_{i}")
        for i in range(spec_size)
    ]
    sp = domain.signal(width=spec_ptr_w, reset_value=0, name=f"{prefix}_spec_sp")

    sp_m1 = cas(domain, (wire_of(sp) - u(spec_ptr_w, 1))[0:spec_ptr_w], cycle=0)
    sp_p1 = cas(domain, (wire_of(sp) + u(spec_ptr_w, 1))[0:spec_ptr_w], cycle=0)

    # Read top-of-stack
    tos_addr = zero_pc
    tos_ctr = cas(domain, m.const(0, width=ctr_width), cycle=0)
    for j in range(spec_size):
        hit = sp == cas(domain, m.const(j, width=spec_ptr_w), cycle=0)
        tos_addr = mux(hit, spec_addr[j], tos_addr)
        tos_ctr = mux(hit, spec_ctr[j], tos_ctr)

    # Read TOS-1 for pop with ctr==0
    tos_m1_addr = zero_pc
    for j in range(spec_size):
        hit = sp_m1 == cas(domain, m.const(j, width=spec_ptr_w), cycle=0)
        tos_m1_addr = mux(hit, spec_addr[j], tos_m1_addr)

    # Push: same address → increment counter; different → advance pointer
    same_addr = tos_addr == push_addr
    push_fire = s0_fire & do_push
    pop_fire = s0_fire & do_pop & (~do_push)

    tos_ctr_is_zero = tos_ctr == cas(domain, m.const(0, width=ctr_width), cycle=0)
    mux(tos_ctr_is_zero, tos_m1_addr, tos_addr)

    # Prediction output
    m.output(f"{prefix}_ras_target", wire_of(tos_addr))
    _out["ras_target"] = tos_addr
    m.output(f"{prefix}_ras_sp", wire_of(sp))
    _out["ras_sp"] = sp
    m.output(f"{prefix}_ras_top_addr", wire_of(tos_addr))
    _out["ras_top_addr"] = tos_addr
    m.output(f"{prefix}_ras_top_ctr", wire_of(tos_ctr))
    _out["ras_top_ctr"] = tos_ctr

    # ── Commit stack storage ─────────────────────────────────────────
    c_addr = [
        domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_ca_{i}")
        for i in range(commit_size)
    ]
    c_ctr = [
        domain.signal(width=ctr_width, reset_value=0, name=f"{prefix}_cc_{i}")
        for i in range(commit_size)
    ]
    c_sp = domain.signal(width=commit_ptr_w, reset_value=0, name=f"{prefix}_commit_sp")

    c_sp_m1 = cas(domain, (wire_of(c_sp) - u(commit_ptr_w, 1))[0:commit_ptr_w], cycle=0)
    c_sp_p1 = cas(domain, (wire_of(c_sp) + u(commit_ptr_w, 1))[0:commit_ptr_w], cycle=0)

    c_tos_addr = zero_pc
    c_tos_ctr = cas(domain, m.const(0, width=ctr_width), cycle=0)
    for j in range(commit_size):
        hit = c_sp == cas(domain, m.const(j, width=commit_ptr_w), cycle=0)
        c_tos_addr = mux(hit, c_addr[j], c_tos_addr)
        c_tos_ctr = mux(hit, c_ctr[j], c_tos_ctr)

    c_same_addr = c_tos_addr == commit_push_addr
    c_tos_ctr_zero = c_tos_ctr == cas(domain, m.const(0, width=ctr_width), cycle=0)

    # ── domain.next() → Cycle 1: state updates ──────────────────────
    domain.next()

    # Speculative stack updates
    # Push: write new entry or increment counter
    for j in range(spec_size):
        # Increment counter case (push same address)
        hit_sp = sp == cas(domain, m.const(j, width=spec_ptr_w), cycle=0)
        we_inc = push_fire & same_addr & hit_sp
        old_c = spec_ctr[j]
        inc_c = mux(
            old_c == cas(domain, m.const(ctr_max, width=ctr_width), cycle=0),
            old_c,
            cas(domain, (wire_of(old_c) + u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        spec_ctr[j].assign(mux(we_inc, inc_c, old_c), when=we_inc)

        # New entry case (push different address)
        hit_sp1 = sp_p1 == cas(domain, m.const(j, width=spec_ptr_w), cycle=0)
        we_new = push_fire & (~same_addr) & hit_sp1
        spec_addr[j].assign(mux(we_new, push_addr, spec_addr[j]), when=we_new)
        spec_ctr[j].assign(
            mux(we_new, cas(domain, m.const(0, width=ctr_width), cycle=0), spec_ctr[j]),
            when=we_new,
        )

        # Pop: decrement counter or move pointer
        we_pop_dec = pop_fire & (~tos_ctr_is_zero) & hit_sp
        dec_c = mux(
            old_c == cas(domain, m.const(0, width=ctr_width), cycle=0),
            old_c,
            cas(domain, (wire_of(old_c) - u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        spec_ctr[j].assign(mux(we_pop_dec, dec_c, spec_ctr[j]), when=we_pop_dec)

    # Stack pointer update
    next_sp = sp
    next_sp = mux(push_fire & (~same_addr), sp_p1, next_sp)
    next_sp = mux(pop_fire & tos_ctr_is_zero, sp_m1, next_sp)
    next_sp = mux(redirect_valid, redirect_sp, next_sp)
    sp <<= next_sp

    # Redirect: restore TOS
    for j in range(spec_size):
        hit = redirect_sp == cas(domain, m.const(j, width=spec_ptr_w), cycle=0)
        we_restore = redirect_valid & hit
        spec_addr[j].assign(
            mux(we_restore, redirect_top_addr, spec_addr[j]), when=we_restore
        )
        spec_ctr[j].assign(
            mux(we_restore, redirect_top_ctr, spec_ctr[j]), when=we_restore
        )

    # Commit stack updates
    for j in range(commit_size):
        hit_csp = c_sp == cas(domain, m.const(j, width=commit_ptr_w), cycle=0)
        hit_csp1 = c_sp_p1 == cas(domain, m.const(j, width=commit_ptr_w), cycle=0)
        old_cc = c_ctr[j]

        we_c_inc = commit_push & c_same_addr & hit_csp
        inc_cc = mux(
            old_cc == cas(domain, m.const(ctr_max, width=ctr_width), cycle=0),
            old_cc,
            cas(domain, (wire_of(old_cc) + u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        c_ctr[j].assign(mux(we_c_inc, inc_cc, old_cc), when=we_c_inc)

        we_c_new = commit_push & (~c_same_addr) & hit_csp1
        c_addr[j].assign(mux(we_c_new, commit_push_addr, c_addr[j]), when=we_c_new)
        c_ctr[j].assign(
            mux(we_c_new, cas(domain, m.const(0, width=ctr_width), cycle=0), c_ctr[j]),
            when=we_c_new,
        )

        we_c_pop = commit_pop & (~c_tos_ctr_zero) & hit_csp
        dec_cc = mux(
            old_cc == cas(domain, m.const(0, width=ctr_width), cycle=0),
            old_cc,
            cas(domain, (wire_of(old_cc) - u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        c_ctr[j].assign(mux(we_c_pop, dec_cc, c_ctr[j]), when=we_c_pop)

    next_csp = c_sp
    next_csp = mux(commit_push & (~c_same_addr), c_sp_p1, next_csp)
    next_csp = mux(commit_pop & c_tos_ctr_zero, c_sp_m1, next_csp)
    c_sp <<= next_csp
    return _out


ras.__pycircuit_name__ = "ras"


if __name__ == "__main__":
    pass
