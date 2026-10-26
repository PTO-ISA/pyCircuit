"""Regular-clock digital_clock scenario; original physical-control oracles remain."""

from example_digital_clock.digital_clock import DigitalClock
from pycircuit import bits, log, rule, system, table


def editing_sequence() -> table[64, bits[8]]:
    # fmt: off
    values: table[64, bits[8]] = (
        89, 0, 1, 2, 3, 4, 5, 6,
        7, 8, 9, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 64, 65, 66, 67, 68, 69, 70,
        71, 72, 73, 80, 81, 82, 83, 84,
        85, 86, 87, 88, 89, 0, 89, 88,
    )
    # fmt: on
    return values


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDigitalClockSettings():  # noqa: N802
    phase: bits[64] = 0

    sequence = editing_sequence()
    minute_index = (phase - 30)[:6]
    second_index = (phase - 94)[:6]
    btn_set = (
        (((1) if phase < 1 else (0)) if phase < 28 else ((1) if phase < 29 else (0)))
        if phase < 92
        else (
            ((1) if phase < 93 else (0))
            if phase < 156
            else ((1) if phase < 157 else (0))
        )
    )
    btn_plus = (
        ((0) if phase < 2 else ((1) if phase < 29 else (0)))
        if phase < 30
        else (
            ((1) if phase < 92 else (0))
            if phase < 94
            else ((1) if phase < 159 else (0))
        )
    )
    btn_minus = (
        (
            ((0) if phase < 1 else (1))
            if phase < 2
            else ((0) if phase < 27 else ((1) if phase < 28 else (0)))
        )
        if phase < 29
        else (
            ((1) if phase < 30 else ((0) if phase < 91 else (1)))
            if phase < 94
            else ((0) if phase < 155 else ((1) if phase < 159 else (0)))
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
        0 if phase < 30 else (sequence[minute_index] if phase < 94 else 88)
    )
    expected_seconds_bcd = (
        0 if phase < 94 else (sequence[second_index] if phase < 158 else 88)
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
    expected_colon_blink = 0

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
    zero1 = 0
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

    advance(phase)
