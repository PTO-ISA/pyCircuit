"""Common combinational logic primitives for XiangShan-pyc.

All functions operate on CycleAwareSignal values and are intended to be called
inside a ``build_*`` function body.
"""

from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    u,
    wire_of,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _zero(m: CycleAwareCircuit, domain: CycleAwareDomain, w: int) -> CycleAwareSignal:
    """Return a ``w``-bit zero constant anchored at the current cycle."""
    return cas(domain, m.const(0, width=w), cycle=domain.cycle_index)


def _ones(m: CycleAwareCircuit, domain: CycleAwareDomain, w: int) -> CycleAwareSignal:
    """Return a ``w``-bit all-ones constant."""
    return cas(domain, m.const((1 << w) - 1, width=w), cycle=domain.cycle_index)


# ---------------------------------------------------------------------------
# mux1h — one-hot multiplexer
# ---------------------------------------------------------------------------


def mux1h(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    sels: list[CycleAwareSignal],
    vals: list[CycleAwareSignal],
    width: int,
) -> CycleAwareSignal:
    """One-hot mux: exactly one bit in *sels* is expected to be high.

    When no selector is active the result is zero (safe default).
    """
    assert len(sels) == len(vals), "sels and vals must have the same length"
    result = _zero(m, domain, width)
    for sel, val in zip(sels, vals, strict=False):
        result = mux(sel, val, result)
    return result


def mux_lookup(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    key: CycleAwareSignal,
    table: list[tuple[int, CycleAwareSignal]],
    default: CycleAwareSignal,
    key_width: int,
) -> CycleAwareSignal:
    """Lookup-table multiplexer: match *key* against integer keys."""
    result = default
    for k, v in reversed(table):
        hit = key == cas(domain, m.const(k, width=key_width), cycle=domain.cycle_index)
        result = mux(hit, v, result)
    return result


# ---------------------------------------------------------------------------
# popcount — population count (reduction tree)
# ---------------------------------------------------------------------------


def popcount(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
    out_width: int,
) -> CycleAwareSignal:
    """Count the number of 1-bits in a list of 1-bit signals (reduction tree)."""
    if not bits:
        return _zero(m, domain, out_width)
    if len(bits) == 1:
        return cas(domain, wire_of(bits[0]) + u(out_width, 0), cycle=domain.cycle_index)

    extended = [
        cas(domain, wire_of(b) + u(out_width, 0), cycle=domain.cycle_index)
        for b in bits
    ]
    while len(extended) > 1:
        nxt: list[CycleAwareSignal] = []
        for i in range(0, len(extended) - 1, 2):
            nxt.append(
                cas(
                    domain,
                    (wire_of(extended[i]) + wire_of(extended[i + 1]))[0:out_width],
                    cycle=domain.cycle_index,
                )
            )
        if len(extended) % 2 == 1:
            nxt.append(extended[-1])
        extended = nxt
    return extended[0]


# ---------------------------------------------------------------------------
# priority_enc — priority encoder (LSB has highest priority)
# ---------------------------------------------------------------------------


def priority_enc(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
    out_width: int,
) -> CycleAwareSignal:
    """Return the index of the lowest-set bit (LSB = highest priority).

    If no bit is set the output is zero.
    """
    result = _zero(m, domain, out_width)
    for i in reversed(range(len(bits))):
        result = mux(
            bits[i],
            cas(domain, m.const(i, width=out_width), cycle=domain.cycle_index),
            result,
        )
    return result


def priority_enc_with_valid(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
    out_width: int,
) -> tuple[CycleAwareSignal, CycleAwareSignal]:
    """Like :func:`priority_enc` but also returns a 1-bit ``valid`` signal."""
    any_set = _zero(m, domain, 1)
    for b in bits:
        any_set = any_set | b
    idx = priority_enc(m, domain, bits, out_width)
    return any_set, idx


# ---------------------------------------------------------------------------
# leading_zeros
# ---------------------------------------------------------------------------


def leading_zeros(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
    out_width: int,
) -> CycleAwareSignal:
    """Count leading zeros from MSB (``bits[-1]``) to LSB (``bits[0]``)."""
    n = len(bits)
    result = cas(domain, m.const(n, width=out_width), cycle=domain.cycle_index)
    for i in range(n):
        result = mux(
            bits[i],
            cas(domain, m.const(n - 1 - i, width=out_width), cycle=domain.cycle_index),
            result,
        )
    return result


# ---------------------------------------------------------------------------
# or_reduce / and_reduce / xor_reduce
# ---------------------------------------------------------------------------


def or_reduce(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
) -> CycleAwareSignal:
    """OR-reduce a list of 1-bit signals."""
    result = _zero(m, domain, 1)
    for b in bits:
        result = result | b
    return result


def and_reduce(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
) -> CycleAwareSignal:
    """AND-reduce a list of 1-bit signals."""
    result = _ones(m, domain, 1)
    for b in bits:
        result = result & b
    return result


def xor_reduce(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    bits: list[CycleAwareSignal],
) -> CycleAwareSignal:
    """XOR-reduce a list of 1-bit signals."""
    result = _zero(m, domain, 1)
    for b in bits:
        result = result ^ b
    return result


# ---------------------------------------------------------------------------
# Standalone build wrappers (for emit_mlir / testing)
# ---------------------------------------------------------------------------


def mux1h(
    m: CycleAwareCircuit, domain: CycleAwareDomain, *, n: int = 4, width: int = 8
):
    sels = [
        cas(domain, m.input(f"{prefix}_sel{i}", width=1), cycle=0) for i in range(n)
    ]
    vals = [
        cas(domain, m.input(f"{prefix}_val{i}", width=width), cycle=0) for i in range(n)
    ]
    result = mux1h(m, domain, sels, vals, width)
    m.output("out", wire_of(result))


mux1h.__pycircuit_name__ = "mux1h"


def popcount(m: CycleAwareCircuit, domain: CycleAwareDomain, *, n: int = 8):
    out_w = max(1, (n).bit_length())
    bits = [
        cas(domain, m.input(f"{prefix}_bit{i}", width=1), cycle=0) for i in range(n)
    ]
    result = popcount(m, domain, bits, out_w)
    m.output("count", wire_of(result))


popcount.__pycircuit_name__ = "popcount"


def priority_enc(m: CycleAwareCircuit, domain: CycleAwareDomain, *, n: int = 8):
    out_w = max(1, (n - 1).bit_length())
    bits = [
        cas(domain, m.input(f"{prefix}_bit{i}", width=1), cycle=0) for i in range(n)
    ]
    valid, idx = priority_enc_with_valid(m, domain, bits, out_w)
    m.output("valid", wire_of(valid))
    m.output("idx", wire_of(idx))


priority_enc.__pycircuit_name__ = "priority_enc"


def leading_zeros(m: CycleAwareCircuit, domain: CycleAwareDomain, *, n: int = 8):
    out_w = max(1, n.bit_length())
    bits = [
        cas(domain, m.input(f"{prefix}_bit{i}", width=1), cycle=0) for i in range(n)
    ]
    result = leading_zeros(m, domain, bits, out_w)
    m.output("count", wire_of(result))


leading_zeros.__pycircuit_name__ = "leading_zeros"


if __name__ == "__main__":
    for _builder, _name in [
        (mux1h, "mux1h"),
        (popcount, "popcount"),
        (priority_enc, "priority_enc"),
        (leading_zeros, "leading_zeros"),
    ]:
        pass
