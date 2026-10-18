"""Regular-clock obs_points scenario; original physical-control oracles remain."""

from example_obs_points.obs_points import ObsPoints
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseObsPoints():  # noqa: N802
    phase: bits[64] = 0
    x = (
        ((phase[:8] & 0) | 0)
        if phase < 2
        else (
            ((phase[:8] & 0) | 10)
            if phase < 3
            else (
                ((phase[:8] & 0) | 20)
                if phase < 4
                else (
                    (phase[:8] - 4)
                    if phase < 260
                    else (
                        ((phase[:8] & 0) | 17)
                        if phase < 261
                        else (
                            ((phase[:8] & 0) | 255)
                            if phase < 262
                            else (
                                ((phase[:8] & 0) | 7)
                                if phase < 263
                                else ((phase[:8] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    dut = ObsPoints(x)

    expected_y = (
        ((phase[:8] & 0) | 1)
        if phase < 2
        else (
            ((phase[:8] & 0) | 11)
            if phase < 3
            else (
                ((phase[:8] & 0) | 21)
                if phase < 4
                else (
                    (phase[:8] - 3)
                    if phase < 260
                    else (
                        ((phase[:8] & 0) | 18)
                        if phase < 261
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 262
                            else (
                                ((phase[:8] & 0) | 8)
                                if phase < 263
                                else ((phase[:8] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_q = (
        ((phase[:8] & 0) | 0)
        if phase < 1
        else (
            ((phase[:8] & 0) | 1)
            if phase < 3
            else (
                ((phase[:8] & 0) | 11)
                if phase < 4
                else (
                    ((phase[:8] & 0) | 21)
                    if phase < 5
                    else (
                        (phase[:8] - 4)
                        if phase < 260
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 261
                            else (
                                ((phase[:8] & 0) | 18)
                                if phase < 262
                                else (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 263
                                    else ((phase[:8] & 0) | 8)
                                )
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 264:
            assert dut.y == expected_y, "obs_points: y"
            assert dut.q == expected_q, "obs_points: q"

        log("info", "obs_points.y", dut.y)
        log("info", "obs_points.q", dut.q)

    check_and_advance()
    advance(phase)
