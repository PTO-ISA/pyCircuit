"""Regular-clock counter scenario; original physical-control oracles remain."""

from example_counter.counter import Counter
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseCounter():  # noqa: N802
    phase: bits[64] = 0
    enable = (
        (
            ((phase[:1] & 0) | 1)
            if phase < 256
            else (((phase[:1] & 0) | 0) if phase < 257 else ((phase[:1] & 0) | 1))
        )
        if phase < 259
        else (
            ((phase[:1] & 0) | 0)
            if phase < 261
            else (((phase[:1] & 0) | 1) if phase < 262 else ((phase[:1] & 0) | 0))
        )
    )
    dut = Counter(enable)

    expected_count = (
        phase[:8]
        if phase < 256
        else (
            (phase[:8] & 0)
            if phase < 258
            else (
                ((phase[:8] & 0) | 1)
                if phase < 259
                else (((phase[:8] & 0) | 2) if phase < 262 else ((phase[:8] & 0) | 3))
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 263:
            assert dut.count == expected_count, "counter: count"

        log("info", "counter.count", dut.count)

    check_and_advance()
    advance(phase)
