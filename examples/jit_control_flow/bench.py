"""Complete known-input table from the retained independent jit_control_flow oracle."""

from example_jit_control_flow.jit_control_flow import JitControlFlow
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseJitControlFlow():  # noqa: N802
    phase: bits[64] = 0
    a = (
        1
        if phase == 0
        else (
            0
            if phase == 1
            else (
                0
                if phase == 2
                else (
                    0
                    if phase == 3
                    else (
                        0
                        if phase == 4
                        else (
                            1
                            if phase == 5
                            else (
                                1
                                if phase == 6
                                else (
                                    1
                                    if phase == 7
                                    else (
                                        1
                                        if phase == 8
                                        else (
                                            255
                                            if phase == 9
                                            else (
                                                255
                                                if phase == 10
                                                else (
                                                    255
                                                    if phase == 11
                                                    else (
                                                        255
                                                        if phase == 12
                                                        else (
                                                            0
                                                            if phase == 13
                                                            else (
                                                                0
                                                                if phase == 14
                                                                else (
                                                                    0
                                                                    if phase == 15
                                                                    else (
                                                                        0
                                                                        if phase == 16
                                                                        else (
                                                                            254
                                                                            if phase
                                                                            == 17
                                                                            else (
                                                                                254
                                                                                if phase
                                                                                == 18
                                                                                else (
                                                                                    254
                                                                                    if phase
                                                                                    == 19
                                                                                    else (
                                                                                        254
                                                                                        if phase
                                                                                        == 20
                                                                                        else (
                                                                                            128
                                                                                            if phase
                                                                                            == 21
                                                                                            else (
                                                                                                128
                                                                                                if phase
                                                                                                == 22
                                                                                                else (
                                                                                                    128
                                                                                                    if phase
                                                                                                    == 23
                                                                                                    else (
                                                                                                        128
                                                                                                        if phase
                                                                                                        == 24
                                                                                                        else (
                                                                                                            128
                                                                                                            if phase
                                                                                                            == 25
                                                                                                            else (
                                                                                                                128
                                                                                                                if phase
                                                                                                                == 26
                                                                                                                else (
                                                                                                                    128
                                                                                                                    if phase
                                                                                                                    == 27
                                                                                                                    else (
                                                                                                                        128
                                                                                                                        if phase
                                                                                                                        == 28
                                                                                                                        else (
                                                                                                                            255
                                                                                                                            if phase
                                                                                                                            == 29
                                                                                                                            else (
                                                                                                                                255
                                                                                                                                if phase
                                                                                                                                == 30
                                                                                                                                else (
                                                                                                                                    255
                                                                                                                                    if phase
                                                                                                                                    == 31
                                                                                                                                    else (
                                                                                                                                        255
                                                                                                                                        if phase
                                                                                                                                        == 32
                                                                                                                                        else (
                                                                                                                                            85
                                                                                                                                            if phase
                                                                                                                                            == 33
                                                                                                                                            else (
                                                                                                                                                85
                                                                                                                                                if phase
                                                                                                                                                == 34
                                                                                                                                                else (
                                                                                                                                                    85
                                                                                                                                                    if phase
                                                                                                                                                    == 35
                                                                                                                                                    else (
                                                                                                                                                        85
                                                                                                                                                        if phase
                                                                                                                                                        == 36
                                                                                                                                                        else (
                                                                                                                                                            170
                                                                                                                                                            if phase
                                                                                                                                                            == 37
                                                                                                                                                            else (
                                                                                                                                                                170
                                                                                                                                                                if phase
                                                                                                                                                                == 38
                                                                                                                                                                else (
                                                                                                                                                                    170
                                                                                                                                                                    if phase
                                                                                                                                                                    == 39
                                                                                                                                                                    else (
                                                                                                                                                                        170
                                                                                                                                                                        if phase
                                                                                                                                                                        == 40
                                                                                                                                                                        else (
                                                                                                                                                                            252
                                                                                                                                                                            if phase
                                                                                                                                                                            == 41
                                                                                                                                                                            else (
                                                                                                                                                                                252
                                                                                                                                                                                if phase
                                                                                                                                                                                == 42
                                                                                                                                                                                else (
                                                                                                                                                                                    252
                                                                                                                                                                                    if phase
                                                                                                                                                                                    == 43
                                                                                                                                                                                    else (
                                                                                                                                                                                        252
                                                                                                                                                                                        if phase
                                                                                                                                                                                        == 44
                                                                                                                                                                                        else (
                                                                                                                                                                                            253
                                                                                                                                                                                            if phase
                                                                                                                                                                                            == 45
                                                                                                                                                                                            else (
                                                                                                                                                                                                253
                                                                                                                                                                                                if phase
                                                                                                                                                                                                == 46
                                                                                                                                                                                                else (
                                                                                                                                                                                                    253
                                                                                                                                                                                                    if phase
                                                                                                                                                                                                    == 47
                                                                                                                                                                                                    else (
                                                                                                                                                                                                        253
                                                                                                                                                                                                        if phase
                                                                                                                                                                                                        == 48
                                                                                                                                                                                                        else (
                                                                                                                                                                                                            250
                                                                                                                                                                                                            if phase
                                                                                                                                                                                                            == 49
                                                                                                                                                                                                            else (
                                                                                                                                                                                                                250
                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                == 50
                                                                                                                                                                                                                else (
                                                                                                                                                                                                                    250
                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                    == 51
                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                        250
                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                        == 52
                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                            7
                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                            == 53
                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                7
                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                == 54
                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                    7
                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                    == 55
                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                        7
                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                        == 56
                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                            17
                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                            == 57
                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                17
                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                == 58
                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                    17
                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                    == 59
                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                        17
                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                        == 60
                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                            254
                                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                                            == 61
                                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                                254
                                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                                == 62
                                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                                    254
                                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                                    == 63
                                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                                        254
                                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                                        == 64
                                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                                            phase[
                                                                                                                                                                                                                                                                                :8
                                                                                                                                                                                                                                                                            ]
                                                                                                                                                                                                                                                                            & 0
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
                    0
                    if phase == 3
                    else (
                        0
                        if phase == 4
                        else (
                            2
                            if phase == 5
                            else (
                                2
                                if phase == 6
                                else (
                                    2
                                    if phase == 7
                                    else (
                                        2
                                        if phase == 8
                                        else (
                                            1
                                            if phase == 9
                                            else (
                                                1
                                                if phase == 10
                                                else (
                                                    1
                                                    if phase == 11
                                                    else (
                                                        1
                                                        if phase == 12
                                                        else (
                                                            255
                                                            if phase == 13
                                                            else (
                                                                255
                                                                if phase == 14
                                                                else (
                                                                    255
                                                                    if phase == 15
                                                                    else (
                                                                        255
                                                                        if phase == 16
                                                                        else (
                                                                            255
                                                                            if phase
                                                                            == 17
                                                                            else (
                                                                                255
                                                                                if phase
                                                                                == 18
                                                                                else (
                                                                                    255
                                                                                    if phase
                                                                                    == 19
                                                                                    else (
                                                                                        255
                                                                                        if phase
                                                                                        == 20
                                                                                        else (
                                                                                            127
                                                                                            if phase
                                                                                            == 21
                                                                                            else (
                                                                                                127
                                                                                                if phase
                                                                                                == 22
                                                                                                else (
                                                                                                    127
                                                                                                    if phase
                                                                                                    == 23
                                                                                                    else (
                                                                                                        127
                                                                                                        if phase
                                                                                                        == 24
                                                                                                        else (
                                                                                                            128
                                                                                                            if phase
                                                                                                            == 25
                                                                                                            else (
                                                                                                                128
                                                                                                                if phase
                                                                                                                == 26
                                                                                                                else (
                                                                                                                    128
                                                                                                                    if phase
                                                                                                                    == 27
                                                                                                                    else (
                                                                                                                        128
                                                                                                                        if phase
                                                                                                                        == 28
                                                                                                                        else (
                                                                                                                            255
                                                                                                                            if phase
                                                                                                                            == 29
                                                                                                                            else (
                                                                                                                                255
                                                                                                                                if phase
                                                                                                                                == 30
                                                                                                                                else (
                                                                                                                                    255
                                                                                                                                    if phase
                                                                                                                                    == 31
                                                                                                                                    else (
                                                                                                                                        255
                                                                                                                                        if phase
                                                                                                                                        == 32
                                                                                                                                        else (
                                                                                                                                            170
                                                                                                                                            if phase
                                                                                                                                            == 33
                                                                                                                                            else (
                                                                                                                                                170
                                                                                                                                                if phase
                                                                                                                                                == 34
                                                                                                                                                else (
                                                                                                                                                    170
                                                                                                                                                    if phase
                                                                                                                                                    == 35
                                                                                                                                                    else (
                                                                                                                                                        170
                                                                                                                                                        if phase
                                                                                                                                                        == 36
                                                                                                                                                        else (
                                                                                                                                                            85
                                                                                                                                                            if phase
                                                                                                                                                            == 37
                                                                                                                                                            else (
                                                                                                                                                                85
                                                                                                                                                                if phase
                                                                                                                                                                == 38
                                                                                                                                                                else (
                                                                                                                                                                    85
                                                                                                                                                                    if phase
                                                                                                                                                                    == 39
                                                                                                                                                                    else (
                                                                                                                                                                        85
                                                                                                                                                                        if phase
                                                                                                                                                                        == 40
                                                                                                                                                                        else (
                                                                                                                                                                            4
                                                                                                                                                                            if phase
                                                                                                                                                                            == 41
                                                                                                                                                                            else (
                                                                                                                                                                                4
                                                                                                                                                                                if phase
                                                                                                                                                                                == 42
                                                                                                                                                                                else (
                                                                                                                                                                                    4
                                                                                                                                                                                    if phase
                                                                                                                                                                                    == 43
                                                                                                                                                                                    else (
                                                                                                                                                                                        4
                                                                                                                                                                                        if phase
                                                                                                                                                                                        == 44
                                                                                                                                                                                        else (
                                                                                                                                                                                            3
                                                                                                                                                                                            if phase
                                                                                                                                                                                            == 45
                                                                                                                                                                                            else (
                                                                                                                                                                                                3
                                                                                                                                                                                                if phase
                                                                                                                                                                                                == 46
                                                                                                                                                                                                else (
                                                                                                                                                                                                    3
                                                                                                                                                                                                    if phase
                                                                                                                                                                                                    == 47
                                                                                                                                                                                                    else (
                                                                                                                                                                                                        3
                                                                                                                                                                                                        if phase
                                                                                                                                                                                                        == 48
                                                                                                                                                                                                        else (
                                                                                                                                                                                                            9
                                                                                                                                                                                                            if phase
                                                                                                                                                                                                            == 49
                                                                                                                                                                                                            else (
                                                                                                                                                                                                                9
                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                == 50
                                                                                                                                                                                                                else (
                                                                                                                                                                                                                    9
                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                    == 51
                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                        9
                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                        == 52
                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                            200
                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                            == 53
                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                200
                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                == 54
                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                    200
                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                    == 55
                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                        200
                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                        == 56
                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                            31
                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                            == 57
                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                31
                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                == 58
                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                    31
                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                    == 59
                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                        31
                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                        == 60
                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                            0
                                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                                            == 61
                                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                                0
                                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                                == 62
                                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                                    0
                                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                                    == 63
                                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                                        0
                                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                                        == 64
                                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                                            phase[
                                                                                                                                                                                                                                                                                :8
                                                                                                                                                                                                                                                                            ]
                                                                                                                                                                                                                                                                            & 0
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
    op = (
        0
        if phase == 0
        else (
            0
            if phase == 1
            else (
                1
                if phase == 2
                else (
                    2
                    if phase == 3
                    else (
                        3
                        if phase == 4
                        else (
                            0
                            if phase == 5
                            else (
                                1
                                if phase == 6
                                else (
                                    2
                                    if phase == 7
                                    else (
                                        3
                                        if phase == 8
                                        else (
                                            0
                                            if phase == 9
                                            else (
                                                1
                                                if phase == 10
                                                else (
                                                    2
                                                    if phase == 11
                                                    else (
                                                        3
                                                        if phase == 12
                                                        else (
                                                            0
                                                            if phase == 13
                                                            else (
                                                                1
                                                                if phase == 14
                                                                else (
                                                                    2
                                                                    if phase == 15
                                                                    else (
                                                                        3
                                                                        if phase == 16
                                                                        else (
                                                                            0
                                                                            if phase
                                                                            == 17
                                                                            else (
                                                                                1
                                                                                if phase
                                                                                == 18
                                                                                else (
                                                                                    2
                                                                                    if phase
                                                                                    == 19
                                                                                    else (
                                                                                        3
                                                                                        if phase
                                                                                        == 20
                                                                                        else (
                                                                                            0
                                                                                            if phase
                                                                                            == 21
                                                                                            else (
                                                                                                1
                                                                                                if phase
                                                                                                == 22
                                                                                                else (
                                                                                                    2
                                                                                                    if phase
                                                                                                    == 23
                                                                                                    else (
                                                                                                        3
                                                                                                        if phase
                                                                                                        == 24
                                                                                                        else (
                                                                                                            0
                                                                                                            if phase
                                                                                                            == 25
                                                                                                            else (
                                                                                                                1
                                                                                                                if phase
                                                                                                                == 26
                                                                                                                else (
                                                                                                                    2
                                                                                                                    if phase
                                                                                                                    == 27
                                                                                                                    else (
                                                                                                                        3
                                                                                                                        if phase
                                                                                                                        == 28
                                                                                                                        else (
                                                                                                                            0
                                                                                                                            if phase
                                                                                                                            == 29
                                                                                                                            else (
                                                                                                                                1
                                                                                                                                if phase
                                                                                                                                == 30
                                                                                                                                else (
                                                                                                                                    2
                                                                                                                                    if phase
                                                                                                                                    == 31
                                                                                                                                    else (
                                                                                                                                        3
                                                                                                                                        if phase
                                                                                                                                        == 32
                                                                                                                                        else (
                                                                                                                                            0
                                                                                                                                            if phase
                                                                                                                                            == 33
                                                                                                                                            else (
                                                                                                                                                1
                                                                                                                                                if phase
                                                                                                                                                == 34
                                                                                                                                                else (
                                                                                                                                                    2
                                                                                                                                                    if phase
                                                                                                                                                    == 35
                                                                                                                                                    else (
                                                                                                                                                        3
                                                                                                                                                        if phase
                                                                                                                                                        == 36
                                                                                                                                                        else (
                                                                                                                                                            0
                                                                                                                                                            if phase
                                                                                                                                                            == 37
                                                                                                                                                            else (
                                                                                                                                                                1
                                                                                                                                                                if phase
                                                                                                                                                                == 38
                                                                                                                                                                else (
                                                                                                                                                                    2
                                                                                                                                                                    if phase
                                                                                                                                                                    == 39
                                                                                                                                                                    else (
                                                                                                                                                                        3
                                                                                                                                                                        if phase
                                                                                                                                                                        == 40
                                                                                                                                                                        else (
                                                                                                                                                                            0
                                                                                                                                                                            if phase
                                                                                                                                                                            == 41
                                                                                                                                                                            else (
                                                                                                                                                                                1
                                                                                                                                                                                if phase
                                                                                                                                                                                == 42
                                                                                                                                                                                else (
                                                                                                                                                                                    2
                                                                                                                                                                                    if phase
                                                                                                                                                                                    == 43
                                                                                                                                                                                    else (
                                                                                                                                                                                        3
                                                                                                                                                                                        if phase
                                                                                                                                                                                        == 44
                                                                                                                                                                                        else (
                                                                                                                                                                                            0
                                                                                                                                                                                            if phase
                                                                                                                                                                                            == 45
                                                                                                                                                                                            else (
                                                                                                                                                                                                1
                                                                                                                                                                                                if phase
                                                                                                                                                                                                == 46
                                                                                                                                                                                                else (
                                                                                                                                                                                                    2
                                                                                                                                                                                                    if phase
                                                                                                                                                                                                    == 47
                                                                                                                                                                                                    else (
                                                                                                                                                                                                        3
                                                                                                                                                                                                        if phase
                                                                                                                                                                                                        == 48
                                                                                                                                                                                                        else (
                                                                                                                                                                                                            0
                                                                                                                                                                                                            if phase
                                                                                                                                                                                                            == 49
                                                                                                                                                                                                            else (
                                                                                                                                                                                                                1
                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                == 50
                                                                                                                                                                                                                else (
                                                                                                                                                                                                                    2
                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                    == 51
                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                        3
                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                        == 52
                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                            0
                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                            == 53
                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                1
                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                == 54
                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                    2
                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                    == 55
                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                        3
                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                        == 56
                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                            0
                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                            == 57
                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                1
                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                == 58
                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                    2
                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                    == 59
                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                        3
                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                        == 60
                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                            0
                                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                                            == 61
                                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                                1
                                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                                == 62
                                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                                    2
                                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                                    == 63
                                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                                        3
                                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                                        == 64
                                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                                            phase[
                                                                                                                                                                                                                                                                                :2
                                                                                                                                                                                                                                                                            ]
                                                                                                                                                                                                                                                                            & 0
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
    dut = JitControlFlow(a, b, op)

    expected_result = (
        7
        if phase == 0
        else (
            4
            if phase == 1
            else (
                4
                if phase == 2
                else (
                    4
                    if phase == 3
                    else (
                        4
                        if phase == 4
                        else (
                            7
                            if phase == 5
                            else (
                                3
                                if phase == 6
                                else (
                                    7
                                    if phase == 7
                                    else (
                                        4
                                        if phase == 8
                                        else (
                                            4
                                            if phase == 9
                                            else (
                                                2
                                                if phase == 10
                                                else (
                                                    2
                                                    if phase == 11
                                                    else (
                                                        5
                                                        if phase == 12
                                                        else (
                                                            3
                                                            if phase == 13
                                                            else (
                                                                5
                                                                if phase == 14
                                                                else (
                                                                    3
                                                                    if phase == 15
                                                                    else (
                                                                        4
                                                                        if phase == 16
                                                                        else (
                                                                            1
                                                                            if phase
                                                                            == 17
                                                                            else (
                                                                                3
                                                                                if phase
                                                                                == 18
                                                                                else (
                                                                                    5
                                                                                    if phase
                                                                                    == 19
                                                                                    else (
                                                                                        2
                                                                                        if phase
                                                                                        == 20
                                                                                        else (
                                                                                            3
                                                                                            if phase
                                                                                            == 21
                                                                                            else (
                                                                                                5
                                                                                                if phase
                                                                                                == 22
                                                                                                else (
                                                                                                    3
                                                                                                    if phase
                                                                                                    == 23
                                                                                                    else (
                                                                                                        4
                                                                                                        if phase
                                                                                                        == 24
                                                                                                        else (
                                                                                                            4
                                                                                                            if phase
                                                                                                            == 25
                                                                                                            else (
                                                                                                                4
                                                                                                                if phase
                                                                                                                == 26
                                                                                                                else (
                                                                                                                    4
                                                                                                                    if phase
                                                                                                                    == 27
                                                                                                                    else (
                                                                                                                        132
                                                                                                                        if phase
                                                                                                                        == 28
                                                                                                                        else (
                                                                                                                            2
                                                                                                                            if phase
                                                                                                                            == 29
                                                                                                                            else (
                                                                                                                                4
                                                                                                                                if phase
                                                                                                                                == 30
                                                                                                                                else (
                                                                                                                                    4
                                                                                                                                    if phase
                                                                                                                                    == 31
                                                                                                                                    else (
                                                                                                                                        3
                                                                                                                                        if phase
                                                                                                                                        == 32
                                                                                                                                        else (
                                                                                                                                            3
                                                                                                                                            if phase
                                                                                                                                            == 33
                                                                                                                                            else (
                                                                                                                                                175
                                                                                                                                                if phase
                                                                                                                                                == 34
                                                                                                                                                else (
                                                                                                                                                    3
                                                                                                                                                    if phase
                                                                                                                                                    == 35
                                                                                                                                                    else (
                                                                                                                                                        4
                                                                                                                                                        if phase
                                                                                                                                                        == 36
                                                                                                                                                        else (
                                                                                                                                                            3
                                                                                                                                                            if phase
                                                                                                                                                            == 37
                                                                                                                                                            else (
                                                                                                                                                                89
                                                                                                                                                                if phase
                                                                                                                                                                == 38
                                                                                                                                                                else (
                                                                                                                                                                    3
                                                                                                                                                                    if phase
                                                                                                                                                                    == 39
                                                                                                                                                                    else (
                                                                                                                                                                        4
                                                                                                                                                                        if phase
                                                                                                                                                                        == 40
                                                                                                                                                                        else (
                                                                                                                                                                            4
                                                                                                                                                                            if phase
                                                                                                                                                                            == 41
                                                                                                                                                                            else (
                                                                                                                                                                                252
                                                                                                                                                                                if phase
                                                                                                                                                                                == 42
                                                                                                                                                                                else (
                                                                                                                                                                                    252
                                                                                                                                                                                    if phase
                                                                                                                                                                                    == 43
                                                                                                                                                                                    else (
                                                                                                                                                                                        8
                                                                                                                                                                                        if phase
                                                                                                                                                                                        == 44
                                                                                                                                                                                        else (
                                                                                                                                                                                            4
                                                                                                                                                                                            if phase
                                                                                                                                                                                            == 45
                                                                                                                                                                                            else (
                                                                                                                                                                                                254
                                                                                                                                                                                                if phase
                                                                                                                                                                                                == 46
                                                                                                                                                                                                else (
                                                                                                                                                                                                    2
                                                                                                                                                                                                    if phase
                                                                                                                                                                                                    == 47
                                                                                                                                                                                                    else (
                                                                                                                                                                                                        5
                                                                                                                                                                                                        if phase
                                                                                                                                                                                                        == 48
                                                                                                                                                                                                        else (
                                                                                                                                                                                                            7
                                                                                                                                                                                                            if phase
                                                                                                                                                                                                            == 49
                                                                                                                                                                                                            else (
                                                                                                                                                                                                                245
                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                == 50
                                                                                                                                                                                                                else (
                                                                                                                                                                                                                    247
                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                    == 51
                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                        12
                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                        == 52
                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                            211
                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                            == 53
                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                67
                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                == 54
                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                    211
                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                    == 55
                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                        4
                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                        == 56
                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                            52
                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                            == 57
                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                246
                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                == 58
                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                    18
                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                    == 59
                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                        21
                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                        == 60
                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                            2
                                                                                                                                                                                                                                                            if phase
                                                                                                                                                                                                                                                            == 61
                                                                                                                                                                                                                                                            else (
                                                                                                                                                                                                                                                                2
                                                                                                                                                                                                                                                                if phase
                                                                                                                                                                                                                                                                == 62
                                                                                                                                                                                                                                                                else (
                                                                                                                                                                                                                                                                    2
                                                                                                                                                                                                                                                                    if phase
                                                                                                                                                                                                                                                                    == 63
                                                                                                                                                                                                                                                                    else (
                                                                                                                                                                                                                                                                        4
                                                                                                                                                                                                                                                                        if phase
                                                                                                                                                                                                                                                                        == 64
                                                                                                                                                                                                                                                                        else (
                                                                                                                                                                                                                                                                            phase[
                                                                                                                                                                                                                                                                                :8
                                                                                                                                                                                                                                                                            ]
                                                                                                                                                                                                                                                                            & 0
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
        if phase < 65:
            assert dut.result == expected_result, "jit_control_flow: result"

        log("info", "jit_control_flow.result", dut.result)

    check_and_advance()
    advance(phase)
