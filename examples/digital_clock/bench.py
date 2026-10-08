"""Regular-clock digital_clock scenario; original physical-control oracles remain."""

from example_digital_clock.digital_clock import DigitalClock
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDigitalClockSettings():  # noqa: N802
    phase: bits[64] = 0
    btn_set = (
        (
            (((phase[:1] & 0) | 1) if phase < 1 else ((phase[:1] & 0) | 0))
            if phase < 28
            else (((phase[:1] & 0) | 1) if phase < 29 else ((phase[:1] & 0) | 0))
        )
        if phase < 92
        else (
            (((phase[:1] & 0) | 1) if phase < 93 else ((phase[:1] & 0) | 0))
            if phase < 156
            else (((phase[:1] & 0) | 1) if phase < 157 else ((phase[:1] & 0) | 0))
        )
    )
    btn_plus = (
        (
            ((phase[:1] & 0) | 0)
            if phase < 2
            else (((phase[:1] & 0) | 1) if phase < 29 else ((phase[:1] & 0) | 0))
        )
        if phase < 30
        else (
            (((phase[:1] & 0) | 1) if phase < 92 else ((phase[:1] & 0) | 0))
            if phase < 94
            else (((phase[:1] & 0) | 1) if phase < 159 else ((phase[:1] & 0) | 0))
        )
    )
    btn_minus = (
        (
            (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
            if phase < 2
            else (
                ((phase[:1] & 0) | 0)
                if phase < 27
                else (((phase[:1] & 0) | 1) if phase < 28 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 29
        else (
            (
                ((phase[:1] & 0) | 1)
                if phase < 30
                else (((phase[:1] & 0) | 0) if phase < 91 else ((phase[:1] & 0) | 1))
            )
            if phase < 94
            else (
                ((phase[:1] & 0) | 0)
                if phase < 155
                else (((phase[:1] & 0) | 1) if phase < 159 else ((phase[:1] & 0) | 0))
            )
        )
    )
    dut = DigitalClock(btn_set, btn_plus, btn_minus)

    expected_hours_bcd = (
        (
            (
                (
                    ((phase[:8] & 0) | 0)
                    if phase < 2
                    else (
                        ((phase[:8] & 0) | 35) if phase < 3 else ((phase[:8] & 0) | 0)
                    )
                )
                if phase < 4
                else (
                    (((phase[:8] & 0) | 1) if phase < 5 else ((phase[:8] & 0) | 2))
                    if phase < 6
                    else (((phase[:8] & 0) | 3) if phase < 7 else ((phase[:8] & 0) | 4))
                )
            )
            if phase < 8
            else (
                (
                    ((phase[:8] & 0) | 5)
                    if phase < 9
                    else (
                        ((phase[:8] & 0) | 6) if phase < 10 else ((phase[:8] & 0) | 7)
                    )
                )
                if phase < 11
                else (
                    (((phase[:8] & 0) | 8) if phase < 12 else ((phase[:8] & 0) | 9))
                    if phase < 13
                    else (
                        ((phase[:8] & 0) | 16) if phase < 14 else ((phase[:8] & 0) | 17)
                    )
                )
            )
        )
        if phase < 15
        else (
            (
                (
                    ((phase[:8] & 0) | 18)
                    if phase < 16
                    else (
                        ((phase[:8] & 0) | 19) if phase < 17 else ((phase[:8] & 0) | 20)
                    )
                )
                if phase < 18
                else (
                    (((phase[:8] & 0) | 21) if phase < 19 else ((phase[:8] & 0) | 22))
                    if phase < 20
                    else (
                        ((phase[:8] & 0) | 23) if phase < 21 else ((phase[:8] & 0) | 24)
                    )
                )
            )
            if phase < 22
            else (
                (
                    (((phase[:8] & 0) | 25) if phase < 23 else ((phase[:8] & 0) | 32))
                    if phase < 24
                    else (
                        ((phase[:8] & 0) | 33) if phase < 25 else ((phase[:8] & 0) | 34)
                    )
                )
                if phase < 26
                else (
                    (((phase[:8] & 0) | 35) if phase < 27 else ((phase[:8] & 0) | 0))
                    if phase < 28
                    else (
                        ((phase[:8] & 0) | 35) if phase < 29 else ((phase[:8] & 0) | 0)
                    )
                )
            )
        )
    )
    expected_minutes_bcd = (
        (
            (
                (
                    (
                        (
                            ((phase[:8] & 0) | 0)
                            if phase < 30
                            else ((phase[:8] & 0) | 89)
                        )
                        if phase < 31
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 32
                            else ((phase[:8] & 0) | 1)
                        )
                    )
                    if phase < 33
                    else (
                        (((phase[:8] & 0) | 2) if phase < 34 else ((phase[:8] & 0) | 3))
                        if phase < 35
                        else (
                            ((phase[:8] & 0) | 4)
                            if phase < 36
                            else ((phase[:8] & 0) | 5)
                        )
                    )
                )
                if phase < 37
                else (
                    (
                        (((phase[:8] & 0) | 6) if phase < 38 else ((phase[:8] & 0) | 7))
                        if phase < 39
                        else (
                            ((phase[:8] & 0) | 8)
                            if phase < 40
                            else ((phase[:8] & 0) | 9)
                        )
                    )
                    if phase < 41
                    else (
                        (
                            ((phase[:8] & 0) | 16)
                            if phase < 42
                            else ((phase[:8] & 0) | 17)
                        )
                        if phase < 43
                        else (
                            ((phase[:8] & 0) | 18)
                            if phase < 44
                            else ((phase[:8] & 0) | 19)
                        )
                    )
                )
            )
            if phase < 45
            else (
                (
                    (
                        (
                            ((phase[:8] & 0) | 20)
                            if phase < 46
                            else ((phase[:8] & 0) | 21)
                        )
                        if phase < 47
                        else (
                            ((phase[:8] & 0) | 22)
                            if phase < 48
                            else ((phase[:8] & 0) | 23)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:8] & 0) | 24)
                            if phase < 50
                            else ((phase[:8] & 0) | 25)
                        )
                        if phase < 51
                        else (
                            ((phase[:8] & 0) | 32)
                            if phase < 52
                            else ((phase[:8] & 0) | 33)
                        )
                    )
                )
                if phase < 53
                else (
                    (
                        (
                            ((phase[:8] & 0) | 34)
                            if phase < 54
                            else ((phase[:8] & 0) | 35)
                        )
                        if phase < 55
                        else (
                            ((phase[:8] & 0) | 36)
                            if phase < 56
                            else ((phase[:8] & 0) | 37)
                        )
                    )
                    if phase < 57
                    else (
                        (
                            ((phase[:8] & 0) | 38)
                            if phase < 58
                            else ((phase[:8] & 0) | 39)
                        )
                        if phase < 59
                        else (
                            ((phase[:8] & 0) | 40)
                            if phase < 60
                            else ((phase[:8] & 0) | 41)
                        )
                    )
                )
            )
        )
        if phase < 61
        else (
            (
                (
                    (
                        (
                            ((phase[:8] & 0) | 48)
                            if phase < 62
                            else ((phase[:8] & 0) | 49)
                        )
                        if phase < 63
                        else (
                            ((phase[:8] & 0) | 50)
                            if phase < 64
                            else ((phase[:8] & 0) | 51)
                        )
                    )
                    if phase < 65
                    else (
                        (
                            ((phase[:8] & 0) | 52)
                            if phase < 66
                            else ((phase[:8] & 0) | 53)
                        )
                        if phase < 67
                        else (
                            ((phase[:8] & 0) | 54)
                            if phase < 68
                            else ((phase[:8] & 0) | 55)
                        )
                    )
                )
                if phase < 69
                else (
                    (
                        (
                            ((phase[:8] & 0) | 56)
                            if phase < 70
                            else ((phase[:8] & 0) | 57)
                        )
                        if phase < 71
                        else (
                            ((phase[:8] & 0) | 64)
                            if phase < 72
                            else ((phase[:8] & 0) | 65)
                        )
                    )
                    if phase < 73
                    else (
                        (
                            ((phase[:8] & 0) | 66)
                            if phase < 74
                            else ((phase[:8] & 0) | 67)
                        )
                        if phase < 75
                        else (
                            ((phase[:8] & 0) | 68)
                            if phase < 76
                            else ((phase[:8] & 0) | 69)
                        )
                    )
                )
            )
            if phase < 77
            else (
                (
                    (
                        (
                            ((phase[:8] & 0) | 70)
                            if phase < 78
                            else ((phase[:8] & 0) | 71)
                        )
                        if phase < 79
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 80
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                    if phase < 81
                    else (
                        (
                            ((phase[:8] & 0) | 80)
                            if phase < 82
                            else ((phase[:8] & 0) | 81)
                        )
                        if phase < 83
                        else (
                            ((phase[:8] & 0) | 82)
                            if phase < 84
                            else ((phase[:8] & 0) | 83)
                        )
                    )
                )
                if phase < 85
                else (
                    (
                        (
                            ((phase[:8] & 0) | 84)
                            if phase < 86
                            else ((phase[:8] & 0) | 85)
                        )
                        if phase < 87
                        else (
                            ((phase[:8] & 0) | 86)
                            if phase < 88
                            else ((phase[:8] & 0) | 87)
                        )
                    )
                    if phase < 89
                    else (
                        (
                            ((phase[:8] & 0) | 88)
                            if phase < 90
                            else ((phase[:8] & 0) | 89)
                        )
                        if phase < 91
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 92
                            else (
                                ((phase[:8] & 0) | 89)
                                if phase < 93
                                else ((phase[:8] & 0) | 88)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_seconds_bcd = (
        (
            (
                (
                    (
                        (
                            ((phase[:8] & 0) | 0)
                            if phase < 94
                            else ((phase[:8] & 0) | 89)
                        )
                        if phase < 95
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 96
                            else ((phase[:8] & 0) | 1)
                        )
                    )
                    if phase < 97
                    else (
                        (((phase[:8] & 0) | 2) if phase < 98 else ((phase[:8] & 0) | 3))
                        if phase < 99
                        else (
                            ((phase[:8] & 0) | 4)
                            if phase < 100
                            else ((phase[:8] & 0) | 5)
                        )
                    )
                )
                if phase < 101
                else (
                    (
                        (
                            ((phase[:8] & 0) | 6)
                            if phase < 102
                            else ((phase[:8] & 0) | 7)
                        )
                        if phase < 103
                        else (
                            ((phase[:8] & 0) | 8)
                            if phase < 104
                            else ((phase[:8] & 0) | 9)
                        )
                    )
                    if phase < 105
                    else (
                        (
                            ((phase[:8] & 0) | 16)
                            if phase < 106
                            else ((phase[:8] & 0) | 17)
                        )
                        if phase < 107
                        else (
                            ((phase[:8] & 0) | 18)
                            if phase < 108
                            else ((phase[:8] & 0) | 19)
                        )
                    )
                )
            )
            if phase < 109
            else (
                (
                    (
                        (
                            ((phase[:8] & 0) | 20)
                            if phase < 110
                            else ((phase[:8] & 0) | 21)
                        )
                        if phase < 111
                        else (
                            ((phase[:8] & 0) | 22)
                            if phase < 112
                            else ((phase[:8] & 0) | 23)
                        )
                    )
                    if phase < 113
                    else (
                        (
                            ((phase[:8] & 0) | 24)
                            if phase < 114
                            else ((phase[:8] & 0) | 25)
                        )
                        if phase < 115
                        else (
                            ((phase[:8] & 0) | 32)
                            if phase < 116
                            else ((phase[:8] & 0) | 33)
                        )
                    )
                )
                if phase < 117
                else (
                    (
                        (
                            ((phase[:8] & 0) | 34)
                            if phase < 118
                            else ((phase[:8] & 0) | 35)
                        )
                        if phase < 119
                        else (
                            ((phase[:8] & 0) | 36)
                            if phase < 120
                            else ((phase[:8] & 0) | 37)
                        )
                    )
                    if phase < 121
                    else (
                        (
                            ((phase[:8] & 0) | 38)
                            if phase < 122
                            else ((phase[:8] & 0) | 39)
                        )
                        if phase < 123
                        else (
                            ((phase[:8] & 0) | 40)
                            if phase < 124
                            else ((phase[:8] & 0) | 41)
                        )
                    )
                )
            )
        )
        if phase < 125
        else (
            (
                (
                    (
                        (
                            ((phase[:8] & 0) | 48)
                            if phase < 126
                            else ((phase[:8] & 0) | 49)
                        )
                        if phase < 127
                        else (
                            ((phase[:8] & 0) | 50)
                            if phase < 128
                            else ((phase[:8] & 0) | 51)
                        )
                    )
                    if phase < 129
                    else (
                        (
                            ((phase[:8] & 0) | 52)
                            if phase < 130
                            else ((phase[:8] & 0) | 53)
                        )
                        if phase < 131
                        else (
                            ((phase[:8] & 0) | 54)
                            if phase < 132
                            else ((phase[:8] & 0) | 55)
                        )
                    )
                )
                if phase < 133
                else (
                    (
                        (
                            ((phase[:8] & 0) | 56)
                            if phase < 134
                            else ((phase[:8] & 0) | 57)
                        )
                        if phase < 135
                        else (
                            ((phase[:8] & 0) | 64)
                            if phase < 136
                            else ((phase[:8] & 0) | 65)
                        )
                    )
                    if phase < 137
                    else (
                        (
                            ((phase[:8] & 0) | 66)
                            if phase < 138
                            else ((phase[:8] & 0) | 67)
                        )
                        if phase < 139
                        else (
                            ((phase[:8] & 0) | 68)
                            if phase < 140
                            else ((phase[:8] & 0) | 69)
                        )
                    )
                )
            )
            if phase < 141
            else (
                (
                    (
                        (
                            ((phase[:8] & 0) | 70)
                            if phase < 142
                            else ((phase[:8] & 0) | 71)
                        )
                        if phase < 143
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 144
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:8] & 0) | 80)
                            if phase < 146
                            else ((phase[:8] & 0) | 81)
                        )
                        if phase < 147
                        else (
                            ((phase[:8] & 0) | 82)
                            if phase < 148
                            else ((phase[:8] & 0) | 83)
                        )
                    )
                )
                if phase < 149
                else (
                    (
                        (
                            ((phase[:8] & 0) | 84)
                            if phase < 150
                            else ((phase[:8] & 0) | 85)
                        )
                        if phase < 151
                        else (
                            ((phase[:8] & 0) | 86)
                            if phase < 152
                            else ((phase[:8] & 0) | 87)
                        )
                    )
                    if phase < 153
                    else (
                        (
                            ((phase[:8] & 0) | 88)
                            if phase < 154
                            else ((phase[:8] & 0) | 89)
                        )
                        if phase < 155
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 156
                            else (
                                ((phase[:8] & 0) | 89)
                                if phase < 157
                                else ((phase[:8] & 0) | 88)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_setting_mode = (
        (((phase[:2] & 0) | 0) if phase < 1 else ((phase[:2] & 0) | 1))
        if phase < 29
        else (
            ((phase[:2] & 0) | 2)
            if phase < 93
            else (((phase[:2] & 0) | 3) if phase < 157 else ((phase[:2] & 0) | 0))
        )
    )
    expected_colon_blink = (phase[:1] & 0) | 0

    @rule
    def check_and_advance():
        if phase < 160:
            assert dut.hours_bcd == expected_hours_bcd, "digital_clock: hours_bcd"
            assert dut.minutes_bcd == expected_minutes_bcd, "digital_clock: minutes_bcd"
            assert dut.seconds_bcd == expected_seconds_bcd, "digital_clock: seconds_bcd"
            assert (
                dut.setting_mode == expected_setting_mode
            ), "digital_clock: setting_mode"
            assert dut.colon_blink == expected_colon_blink, "digital_clock: colon_blink"

        log("info", "digital_clock.hours_bcd", dut.hours_bcd)
        log("info", "digital_clock.minutes_bcd", dut.minutes_bcd)
        log("info", "digital_clock.seconds_bcd", dut.seconds_bcd)
        log("info", "digital_clock.setting_mode", dut.setting_mode)
        log("info", "digital_clock.colon_blink", dut.colon_blink)

    check_and_advance()
    advance(phase)


@system
def ExerciseDigitalClock():  # noqa: N802
    # Exactly two physical 50 MHz divider periods, followed by observation.
    phase: bits[64] = 0
    set_pressed = (
        (phase < 4)
        | ((phase >= 50_000_000) & (phase < 50_000_003))
        | (phase == 99_999_999)
    )
    minus_pressed = (phase >= 1) & (phase < 4)
    zero1 = phase[:1] & 0
    dut = DigitalClock(set_pressed, zero1, minus_pressed)

    # Calendar oracle in closed form: first four raw presses set 23:59:59.
    # The first second rolls midnight; the second tick occurs in setting mode.
    before_midnight = (phase >= 4) & (phase < 50_000_000)
    expected_hours = (
        0x23 if ((phase == 2) | (phase == 3) | before_midnight) else (phase[:8] & 0)
    )
    expected_minutes = 0x59 if ((phase == 3) | before_midnight) else (phase[:8] & 0)
    expected_seconds = 0x59 if before_midnight else (phase[:8] & 0)
    expected_mode = (
        ((phase[:2] & 0) | 1)
        if ((phase == 1) | (phase == 50_000_001))
        else (
            ((phase[:2] & 0) | 2)
            if ((phase == 2) | (phase == 50_000_002))
            else (
                ((phase[:2] & 0) | 3)
                if ((phase == 3) | ((phase >= 50_000_003) & (phase < 100_000_000)))
                else (phase[:2] & 0)
            )
        )
    )
    expected_blink = (
        ((phase[:1] & 0) | 1)
        if (phase >= 50_000_000) & (phase < 100_000_000)
        else (phase[:1] & 0)
    )

    @rule
    def check_calendar():
        if phase <= 100_000_000:
            assert dut.hours_bcd == expected_hours, "digital_clock: hours"
            assert dut.minutes_bcd == expected_minutes, "digital_clock: minutes"
            assert dut.seconds_bcd == expected_seconds, "digital_clock: seconds"
            assert dut.setting_mode == expected_mode, "digital_clock: setting mode"
            assert dut.colon_blink == expected_blink, "digital_clock: second divider"

    check_calendar()
    advance(phase)
