"""A register next wire must end up with exactly one driver.

`Reg.set` used to emit one `pyc.assign` per call, which the frontend contract
check rejects as "register next wire has multiple drivers". The calls now fold
into a single priority chain, so chained conditional updates stay legal while
keeping the later-write-wins order the emitted sequence used to produce.
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
    return compile_cycle_aware(build, name="fold_next").emit_mlir()


def test_chained_conditional_sets_emit_one_driver() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        x = m.input("x", width=8)
        r = m.out("r", domain=cd, width=8, init=0)
        r.set(1, when=x == 1)
        r.set(2, when=x == 2)
        m.output("y", r)

    mlir = _emit(build)

    assert mlir.count("pyc.assign") == 1
    selects = [line for line in mlir.splitlines() if "pyc.select" in line]
    assert len(selects) == 2


def test_later_set_wins_the_priority_chain() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        x = m.input("x", width=8)
        r = m.out("r", domain=cd, width=8, init=0)
        r.set(1, when=x == 1)
        r.set(2, when=x == 2)
        m.output("y", r)

    mlir = _emit(build)

    selects = [line.strip() for line in mlir.splitlines() if "pyc.select" in line]
    assigns = [line.strip() for line in mlir.splitlines() if "pyc.assign" in line]
    assert len(selects) == 2 and len(assigns) == 1

    first_result = selects[0].split("=")[0].strip()
    second_result = selects[1].split("=")[0].strip()
    # The second (later) set wraps the first one, so it owns the outer select
    # and drives the single assign.
    assert first_result in selects[1]
    assert second_result in assigns[0]


def test_unconditional_set_drives_without_a_select_chain() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        r = m.out("r", domain=cd, width=8, init=0)
        r.set(7)
        m.output("y", r)

    mlir = _emit(build)

    assert mlir.count("pyc.assign") == 1
    assert "pyc.select" not in mlir


def test_later_unconditional_set_supersedes_earlier_conditions() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        x = m.input("x", width=8)
        r = m.out("r", domain=cd, width=8, init=0)
        r.set(1, when=x == 1)
        r.set(9)
        m.output("y", r)

    mlir = _emit(build)

    # The unconditional write wins, so the earlier conditional entry is dropped
    # rather than left behind as a select nothing reads.
    assert mlir.count("pyc.assign") == 1
    assert "pyc.select" not in mlir


def test_every_register_reports_one_assign_in_a_real_example() -> None:
    import importlib.util
    from pathlib import Path

    design = (
        Path(__file__).resolve().parents[2]
        / "examples/pycircuit/applications/dodgeball_game/dodgeball_game.py"
    )
    spec = importlib.util.spec_from_file_location("dodgeball_fold_gate", design)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load dodgeball example")
    example = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(example)

    mlir = example.compile_cycle_aware(
        example.build,
        name="dodgeball_game",
    ).emit_mlir()

    # Chained conditional sets on one register must collapse to one driver: the
    # example sets player_x and friends several times per register.
    assert mlir.count("pyc.assign") == mlir.count("pyc.reg")


def _register_drivers(mlir: str) -> list[tuple[str, str, str | None]]:
    """Return (reg_result, next_wire, driver_source) for every register."""
    import re

    regs: list[tuple[str, str]] = []
    for line in mlir.splitlines():
        match = re.match(r"\s*%(\w+) = pyc\.reg ([^:]+):", line)
        if not match:
            continue
        operands = [item.strip() for item in match.group(2).split(",")]
        regs.append((match.group(1), operands[3].lstrip("%")))
    drivers: dict[str, str] = {}
    for line in mlir.splitlines():
        match = re.match(r"\s*pyc\.assign %(\w+), %(\w+)", line)
        if match:
            drivers[match.group(1)] = match.group(2)
    return [(result, nxt, drivers.get(nxt)) for result, nxt in regs]


def test_declared_but_unset_register_holds_its_value() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        r = m.out("r", domain=cd, width=4, init=3)
        m.output("y", r)

    mlir = _emit(build)

    drivers = _register_drivers(mlir)
    assert len(drivers) == 1
    result, next_wire, source = drivers[0]
    # The next wire is driven, and it holds the register's own value rather
    # than reaching the emitters as a floating net.
    assert source == result, (result, next_wire, source)


def test_explicit_assign_is_not_joined_by_a_hold_driver() -> None:
    def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
        cd: ClockDomain = domain.clock_domain
        x = m.input("x", width=4)
        r = m.out("r", domain=cd, width=4, init=0)
        m.assign(r.next, x)
        m.output("y", r)

    mlir = _emit(build)

    # The user's driver wins and the frontend must not add a hold driver on top
    # of it: that would be a second driver for the same next wire.
    assert mlir.count("pyc.assign") == 1
    drivers = _register_drivers(mlir)
    assert len(drivers) == 1
    result, next_wire, source = drivers[0]
    assert source is not None and source != result
