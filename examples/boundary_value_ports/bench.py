"""Complete known-input table from the retained independent boundary_value_ports oracle."""

from example_boundary_value_ports.boundary_value_ports import BoundaryValuePorts
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseBoundaryValuePorts():  # noqa: N802
    phase: bits[64] = 0
    seed = (
        10
        if phase == 0
        else (
            0
            if phase == 1
            else (
                1
                if phase == 2
                else (
                    4294967283
                    if phase == 3
                    else (
                        4294967284
                        if phase == 4
                        else (
                            4294967289
                            if phase == 5
                            else (
                                4294967290
                                if phase == 6
                                else (
                                    4294967291
                                    if phase == 7
                                    else (
                                        4294967294
                                        if phase == 8
                                        else (
                                            4294967295
                                            if phase == 9
                                            else (
                                                2147483648
                                                if phase == 10
                                                else (
                                                    2147483647
                                                    if phase == 11
                                                    else (
                                                        1073741824
                                                        if phase == 12
                                                        else (
                                                            1431655765
                                                            if phase == 13
                                                            else (
                                                                2863311530
                                                                if phase == 14
                                                                else (
                                                                    17
                                                                    if phase == 15
                                                                    else (
                                                                        phase[:32] & 0
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
                    )
                )
            )
        )
    )
    dut = BoundaryValuePorts(seed)

    expected_acc = (
        48
        if phase == 0
        else (
            18
            if phase == 1
            else (
                21
                if phase == 2
                else (
                    4294967275
                    if phase == 3
                    else (
                        4294967278
                        if phase == 4
                        else (
                            4294967293
                            if phase == 5
                            else (
                                0
                                if phase == 6
                                else (
                                    3
                                    if phase == 7
                                    else (
                                        12
                                        if phase == 8
                                        else (
                                            15
                                            if phase == 9
                                            else (
                                                2147483666
                                                if phase == 10
                                                else (
                                                    2147483663
                                                    if phase == 11
                                                    else (
                                                        3221225490
                                                        if phase == 12
                                                        else (
                                                            17
                                                            if phase == 13
                                                            else (
                                                                16
                                                                if phase == 14
                                                                else (
                                                                    69
                                                                    if phase == 15
                                                                    else (
                                                                        phase[:32] & 0
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
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 16:
            assert dut.acc == expected_acc, "boundary_value_ports: acc"

        log("info", "boundary_value_ports.acc", dut.acc)

    check_and_advance()
    advance(phase)
