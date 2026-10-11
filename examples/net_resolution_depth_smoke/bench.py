"""Regular-clock net_resolution_depth_smoke scenario; original physical-control oracles remain."""

from example_net_resolution_depth_smoke.net_resolution_depth_smoke import (
    NetResolutionDepthSmoke,
)
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseNetResolutionDepthSmoke():  # noqa: N802
    phase: bits[64] = 0
    in_x = (
        ((phase[:8] & 0) | 0)
        if phase < 1
        else (
            ((phase[:8] & 0) | 255)
            if phase < 2
            else (
                ((phase[:8] & 0) | 1)
                if phase < 3
                else (
                    ((phase[:8] & 0) | 2)
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
    )
    dut = NetResolutionDepthSmoke(in_x)

    expected_y = (
        ((phase[:8] & 0) | 0)
        if phase < 1
        else (
            ((phase[:8] & 0) | 4)
            if phase < 2
            else (
                ((phase[:8] & 0) | 3)
                if phase < 3
                else (
                    ((phase[:8] & 0) | 5)
                    if phase < 4
                    else (
                        ((phase[:8] & 0) | 6)
                        if phase < 5
                        else (
                            (phase[:8] - 1)
                            if phase < 261
                            else (
                                ((phase[:8] & 0) | 21)
                                if phase < 262
                                else (
                                    ((phase[:8] & 0) | 3)
                                    if phase < 263
                                    else ((phase[:8] & 0) | 11)
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
            assert dut.y == expected_y, "net_resolution_depth_smoke: y"

        log("info", "net_resolution_depth_smoke.y", dut.y)

    check_and_advance()
    advance(phase)
