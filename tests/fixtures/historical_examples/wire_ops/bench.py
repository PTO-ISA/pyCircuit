"""Regular-clock wire_ops scenario; original physical-control oracles remain."""

from example_wire_ops.wire_ops import WireOps
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseWireOps():  # noqa: N802
    phase: bits[64] = 0
    a = (
        (((phase[:8] & 0) | 3) if phase < 1 else ((phase[:8] & 0) | 170))
        if phase < 2
        else (
            ((phase[:8] & 0) | 255)
            if phase < 5
            else (((phase[:8] & 0) | 128) if phase < 6 else ((phase[:8] & 0) | 0))
        )
    )
    b = (
        (
            ((phase[:8] & 0) | 1)
            if phase < 1
            else (((phase[:8] & 0) | 15) if phase < 2 else ((phase[:8] & 0) | 0))
        )
        if phase < 3
        else (
            ((phase[:8] & 0) | 255)
            if phase < 5
            else (((phase[:8] & 0) | 1) if phase < 6 else ((phase[:8] & 0) | 0))
        )
    )
    sel = (
        (((phase[:1] & 0) | 1) if phase < 1 else ((phase[:1] & 0) | 0))
        if phase < 2
        else (((phase[:1] & 0) | 1) if phase < 5 else ((phase[:1] & 0) | 0))
    )
    dut = WireOps(a, b, sel != 0)

    expected_y = (
        (
            ((phase[:8] & 0) | 0)
            if phase < 1
            else (((phase[:8] & 0) | 1) if phase < 2 else ((phase[:8] & 0) | 165))
        )
        if phase < 3
        else (
            ((phase[:8] & 0) | 0)
            if phase < 4
            else (((phase[:8] & 0) | 255) if phase < 6 else ((phase[:8] & 0) | 129))
        )
    )

    @rule
    def check_and_advance():
        if phase < 7:
            assert dut.y == expected_y, "wire_ops: y"

        log("info", "wire_ops.y", dut.y)

    check_and_advance()
    advance(phase)
