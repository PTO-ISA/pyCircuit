"""Complete known-input table from the retained independent arith oracle."""

from example_arith.arith import Arith
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseArith():  # noqa: N802
    phase: bits[64] = 0
    a = (
        1
        if phase == 0
        else (
            0
            if phase == 1
            else (
                524287
                if phase == 2
                else (
                    524287
                    if phase == 3
                    else (
                        524287
                        if phase == 4
                        else (
                            262144
                            if phase == 5
                            else (
                                262143
                                if phase == 6
                                else (
                                    524286
                                    if phase == 7
                                    else (
                                        273067
                                        if phase == 8
                                        else (12345 if phase == 9 else (phase[:19] & 0))
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    b = (
        2
        if phase == 0
        else (
            0
            if phase == 1
            else (
                0
                if phase == 2
                else (
                    1
                    if phase == 3
                    else (
                        524287
                        if phase == 4
                        else (
                            262144
                            if phase == 5
                            else (
                                1
                                if phase == 6
                                else (
                                    3
                                    if phase == 7
                                    else (
                                        174762
                                        if phase == 8
                                        else (54321 if phase == 9 else (phase[:19] & 0))
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    dut = Arith(a, b)

    expected_sum = (
        3
        if phase == 0
        else (
            0
            if phase == 1
            else (
                524287
                if phase == 2
                else (
                    0
                    if phase == 3
                    else (
                        524286
                        if phase == 4
                        else (
                            0
                            if phase == 5
                            else (
                                262144
                                if phase == 6
                                else (
                                    1
                                    if phase == 7
                                    else (
                                        447829
                                        if phase == 8
                                        else (66666 if phase == 9 else (phase[:19] & 0))
                                    )
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
        if phase < 10:
            assert dut.sum == expected_sum, "arith: sum"
            assert dut.lane_mask == 65535, "arith: lane mask"
            assert dut.acc_width == 19, "arith: accumulator width"

        log("info", "arith.sum", dut.sum)

    check_and_advance()
    advance(phase)
