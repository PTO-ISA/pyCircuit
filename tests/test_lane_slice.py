"""Gate for ASL scaled slicing ``x[idx *: width]`` (ASL alignment TODO T5).

ASL's scaled/indexed slice ``x[i *: w]`` reads element ``i`` of a packed
vector where each element is ``w`` bits wide, i.e. bits
``[i*w, i*w + w - 1]``. We expose it as ``signal.lane(i, width=w)``, pure
front-end sugar over ``slice``/``extract``: it must emit byte-identical MLIR to
the equivalent Python half-open slice ``x[i*w : (i+1)*w]``. Both plain ``Wire``
and ``CycleAwareSignal`` are supported; the cycle-aware form keeps its cycle
tag. ``idx`` is an elaboration-time Python ``int``; out-of-range / bad width
raise at build time.
"""

from __future__ import annotations

import pytest
from pycircuit import Circuit, CycleAwareCircuit, cas, wire_of

# --- Wire lane == hand-written slice (byte-level MLIR equivalence) ----------


def _wire_lane(idx: int, width: int) -> str:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    m.output("y", bus.lane(idx, width=width))
    return m.emit_mlir()


def _wire_manual_slice(lsb: int, width: int) -> str:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    m.output("y", bus[lsb : lsb + width])
    return m.emit_mlir()


@pytest.mark.parametrize(
    "idx, width, lsb",
    [
        (0, 8, 0),
        (1, 8, 8),
        (3, 8, 24),
        (0, 16, 0),
        (1, 16, 16),
        (2, 4, 8),
        (7, 4, 28),
    ],
)
def test_lane_matches_manual_slice(idx, width, lsb) -> None:
    assert _wire_lane(idx, width) == _wire_manual_slice(lsb, width)


def test_lane_matches_slice_kwargs() -> None:
    def via_lane() -> str:
        m = Circuit("t")
        bus = m.input("bus", width=32)
        m.output("y", bus.lane(2, width=8))
        return m.emit_mlir()

    def via_slice() -> str:
        m = Circuit("t")
        bus = m.input("bus", width=32)
        m.output("y", bus.slice(lsb=16, width=8))
        return m.emit_mlir()

    assert via_lane() == via_slice()


def test_lane_result_width() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    assert bus.lane(0, width=8).width == 8
    assert bus.lane(1, width=4).width == 4
    assert bus.lane(3, width=8).width == 8


def test_lane_covers_exact_top_element() -> None:
    # last lane ends exactly at MSB -- must not raise
    m = Circuit("t")
    bus = m.input("bus", width=32)
    top = bus.lane(3, width=8)  # bits [24,31]
    assert top.width == 8


# --- tuple subscript sugar  x[i, w] == x.lane(i, width=w) -------------------


@pytest.mark.parametrize(
    "idx, width",
    [(0, 8), (1, 8), (3, 8), (0, 16), (1, 16), (2, 4), (7, 4)],
)
def test_tuple_subscript_matches_lane(idx, width) -> None:
    def via_tuple() -> str:
        m = Circuit("t")
        bus = m.input("bus", width=32)
        m.output("y", bus[idx, width])
        return m.emit_mlir()

    def via_lane() -> str:
        m = Circuit("t")
        bus = m.input("bus", width=32)
        m.output("y", bus.lane(idx, width=width))
        return m.emit_mlir()

    assert via_tuple() == via_lane()


def test_tuple_subscript_bad_arity_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(TypeError, match="index, width"):
        bus[1, 8, 4]  # type: ignore[misc]


def test_tuple_subscript_out_of_range_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="out of range"):
        bus[4, 8]


def test_cas_tuple_subscript_matches_lane() -> None:
    def via_tuple() -> str:
        m = CycleAwareCircuit("t")
        d = m.create_domain("clk")
        bus = cas(d, m.input("bus", width=32))
        m.output("y", wire_of(bus[1, 8]))
        return m.emit_mlir()

    def via_lane() -> str:
        m = CycleAwareCircuit("t")
        d = m.create_domain("clk")
        bus = cas(d, m.input("bus", width=32))
        m.output("y", wire_of(bus.lane(1, width=8)))
        return m.emit_mlir()

    assert via_tuple() == via_lane()


# --- error handling ---------------------------------------------------------


def test_lane_out_of_range_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="out of range"):
        bus.lane(4, width=8)  # bits [32,39] -- past the end


def test_lane_partial_overrun_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="out of range"):
        bus.lane(3, width=16)  # bits [48,63] -- overruns


def test_lane_zero_width_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="width"):
        bus.lane(0, width=0)


def test_lane_negative_width_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="width"):
        bus.lane(0, width=-4)


def test_lane_negative_index_raises() -> None:
    m = Circuit("t")
    bus = m.input("bus", width=32)
    with pytest.raises(ValueError, match="index"):
        bus.lane(-1, width=8)


# --- CycleAwareSignal -------------------------------------------------------


def test_cas_lane_matches_manual_slice() -> None:
    def via_lane() -> str:
        m = CycleAwareCircuit("t")
        d = m.create_domain("clk")
        bus = cas(d, m.input("bus", width=32))
        m.output("y", wire_of(bus.lane(1, width=8)))
        return m.emit_mlir()

    def manual() -> str:
        m = CycleAwareCircuit("t")
        d = m.create_domain("clk")
        bus = cas(d, m.input("bus", width=32))
        m.output("y", wire_of(bus[8:16]))
        return m.emit_mlir()

    assert via_lane() == manual()


def test_cas_lane_keeps_cycle() -> None:
    m = CycleAwareCircuit("t")
    d = m.create_domain("clk")
    bus = cas(d, m.input("bus", width=32))
    d.next()
    later = cas(d, m.input("later", width=32))
    lane = later.lane(2, width=8)
    assert lane.cycle == later.cycle


def test_cas_lane_out_of_range_raises() -> None:
    m = CycleAwareCircuit("t")
    d = m.create_domain("clk")
    bus = cas(d, m.input("bus", width=32))
    with pytest.raises(ValueError, match="out of range"):
        bus.lane(4, width=8)
