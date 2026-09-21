"""Multi-domain modules keep one control pair per domain (Decision 0126).

The structural case signature carries a clock-then-reset pair per declared
domain and keeps each control's source port name, so an emitted module exposes
`clk_a`/`clk_b` instead of a generic `clk`/`clk_2`.
"""

from __future__ import annotations

import pytest

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
)
from pycircuit.hw import ClockDomain

pytestmark = pytest.mark.unit


def _emit(build) -> str:
    return compile_cycle_aware(build, name="multi_clock").emit_mlir()


def test_two_domains_emit_two_named_control_pairs() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        clk_a = m.clock("clk_a")
        rst_a = m.reset("rst_a")
        clk_b = m.clock("clk_b")
        rst_b = m.reset("rst_b")
        cd_a = ClockDomain(clk=clk_a, rst=rst_a)
        cd_b = ClockDomain(clk=clk_b, rst=rst_b)
        a = m.out("a_q", domain=cd_a, width=8, init=0)
        b = m.out("b_q", domain=cd_b, width=8, init=0)
        a.set(a.out() + 1)
        b.set(b.out() + 1)
        m.output("a_count", a)
        m.output("b_count", b)

    mlir = _emit(build)

    assert mlir.count('control_port_mapping<"clock"') == mlir.count(
        'control_port_mapping<"reset"'
    )
    for name in ("clk_a", "rst_a", "clk_b", "rst_b"):
        assert f'"{name}"' in mlir, name
    # Controls are contiguous clock-then-reset pairs from physical input zero.
    for marker in ('"clock", 0,', '"reset", 1,', '"clock", 2,', '"reset", 3,'):
        assert marker in mlir, marker


def test_reset_without_a_preceding_clock_is_rejected() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        rst_a = m.reset("rst_a")
        clk_a = m.clock("clk_a")
        cd = ClockDomain(clk=clk_a, rst=rst_a)
        a = m.out("a_q", domain=cd, width=8, init=0)
        a.set(a.out() + 1)
        m.output("a_count", a)

    with pytest.raises(ValueError, match="no preceding clock"):
        _emit(build)
