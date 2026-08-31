from __future__ import annotations

from dataclasses import dataclass

from pycircuit import Circuit, Reg, Wire, function, u
from pycircuit.hw import cat


@dataclass(frozen=True)
class Consts:
    one1: Wire
    zero1: Wire
    zero3: Wire
    zero4: Wire
    zero6: Wire
    zero8: Wire
    zero32: Wire
    zero64: Wire
    one64: Wire


@function
def make_consts(m: Circuit) -> Consts:
    c = m.const
    return Consts(
        one1=c(1, width=1),
        zero1=c(0, width=1),
        zero3=c(0, width=3),
        zero4=c(0, width=4),
        zero6=c(0, width=6),
        zero8=c(0, width=8),
        zero32=c(0, width=32),
        zero64=c(0, width=64),
        one64=c(1, width=64),
    )


@function
def masked_eq(m: Circuit, x: Wire, *, mask: int, match: int) -> Wire:
    _ = m
    return x & int(mask) == int(match)


@function
def mux_read(
    m: Circuit, idx: Wire, entries: list[Wire | Reg], *, default: int = 0
) -> Wire:
    """Read `entries[idx]` using a mux-chain (small-table helper)."""
    if not entries:
        raise ValueError("entries must be non-empty")
    width = entries[0].width
    v = u(width, int(default))
    for i, e in enumerate(entries):
        ev = e.out() if isinstance(e, Reg) else e
        v = ev if idx == u(idx.width, i) else v
    return v


@function
def make_bp_table(
    m: Circuit, clk, rst, *, entries: int, en: Wire
) -> tuple[list[Reg], list[Reg], list[Reg], list[Reg]]:
    """Allocate a tiny BTB/BHT table as named regs (JIT-time elaboration)."""
    bp_valid: list[Reg] = []
    bp_tag: list[Reg] = []
    bp_target: list[Reg] = []
    bp_ctr: list[Reg] = []
    for i in range(int(entries)):
        bp_valid.append(m.out(f"valid{i}", clk=clk, rst=rst, width=1, init=0, en=en))
        bp_tag.append(m.out(f"tag{i}", clk=clk, rst=rst, width=64, init=0, en=en))
        bp_target.append(m.out(f"target{i}", clk=clk, rst=rst, width=64, init=0, en=en))
        bp_ctr.append(m.out(f"ctr{i}", clk=clk, rst=rst, width=2, init=0, en=en))
    return (bp_valid, bp_tag, bp_target, bp_ctr)


@function
def shl_var(m: Circuit, value: Wire, shamt: Wire) -> Wire:
    """Variable shift-left by `shamt` (uses low 6 bits)."""
    _ = m
    s = shamt[0:6] if shamt.width >= 6 else cat(u(6 - shamt.width, 0), shamt)
    out = value
    out = out.shl(amount=1) if s[0] else out
    out = out.shl(amount=2) if s[1] else out
    out = out.shl(amount=4) if s[2] else out
    out = out.shl(amount=8) if s[3] else out
    out = out.shl(amount=16) if s[4] else out
    out = out.shl(amount=32) if s[5] else out
    return out


@function
def lshr_var(m: Circuit, value: Wire, shamt: Wire) -> Wire:
    """Variable logical shift-right by `shamt` (uses low 6 bits)."""
    _ = m
    s = shamt[0:6] if shamt.width >= 6 else cat(u(6 - shamt.width, 0), shamt)
    out = value
    out = out.lshr(amount=1) if s[0] else out
    out = out.lshr(amount=2) if s[1] else out
    out = out.lshr(amount=4) if s[2] else out
    out = out.lshr(amount=8) if s[3] else out
    out = out.lshr(amount=16) if s[4] else out
    out = out.lshr(amount=32) if s[5] else out
    return out


@function
def ashr_var(m: Circuit, value: Wire, shamt: Wire) -> Wire:
    """Variable arithmetic shift-right by `shamt` (uses low 6 bits)."""
    _ = m
    s = shamt[0:6] if shamt.width >= 6 else cat(u(6 - shamt.width, 0), shamt)
    out = value.as_signed()
    out = out.ashr(amount=1) if s[0] else out
    out = out.ashr(amount=2) if s[1] else out
    out = out.ashr(amount=4) if s[2] else out
    out = out.ashr(amount=8) if s[3] else out
    out = out.ashr(amount=16) if s[4] else out
    out = out.ashr(amount=32) if s[5] else out
    return out
