"""Complete known-input table from the retained independent hier_modules oracle."""

from example_hier_modules.hier_modules import HierModules
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseHierModules():  # noqa: N802
    phase: bits[64] = 0
    x = (
        1
        if phase == 0
        else (
            0
            if phase == 1
            else (
                252
                if phase == 2
                else (
                    253
                    if phase == 3
                    else (
                        254
                        if phase == 4
                        else (
                            255
                            if phase == 5
                            else (
                                2
                                if phase == 6
                                else (
                                    127
                                    if phase == 7
                                    else (
                                        128
                                        if phase == 8
                                        else (
                                            250
                                            if phase == 9
                                            else (
                                                5
                                                if phase == 10
                                                else (
                                                    17
                                                    if phase == 11
                                                    else (phase[:8] & 0)
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    dut = HierModules(x)

    expected_y = (
        4
        if phase == 0
        else (
            3
            if phase == 1
            else (
                255
                if phase == 2
                else (
                    0
                    if phase == 3
                    else (
                        1
                        if phase == 4
                        else (
                            2
                            if phase == 5
                            else (
                                5
                                if phase == 6
                                else (
                                    130
                                    if phase == 7
                                    else (
                                        131
                                        if phase == 8
                                        else (
                                            253
                                            if phase == 9
                                            else (
                                                8
                                                if phase == 10
                                                else (
                                                    20
                                                    if phase == 11
                                                    else (phase[:8] & 0)
                                                )
                                            )
                                        )
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
        if phase < 12:
            assert dut.y == expected_y, "hier_modules: y"

        log("info", "hier_modules.y", dut.y)

    check_and_advance()
    advance(phase)
