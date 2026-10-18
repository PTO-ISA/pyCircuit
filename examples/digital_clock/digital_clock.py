"""A fixed 50 MHz clock with raw, repeating setting buttons."""

# ruff: noqa: F823 -- nested Rules update captured Module-owned state.

import pycircuit as ac


@ac.struct
class BcdResult:
    value: ac.u8


@ac.struct
class ClockResult:
    hours_bcd: ac.u8
    minutes_bcd: ac.u8
    seconds_bcd: ac.u8
    setting_mode: ac.u2
    colon_blink: ac.u1


def encode_bcd(value: ac.u6) -> BcdResult:
    tens = value // 10
    ones = value % 10
    tens8: ac.u8 = tens[:4]
    ones8: ac.u8 = ones[:4]
    return BcdResult(value=(tens8 << 4) | ones8)


@ac.module
def EncodeBcd(value: ac.u6) -> BcdResult:  # noqa: N802
    return encode_bcd(value)


@ac.module
def DigitalClock(  # noqa: N802
    btn_set: ac.u1, btn_plus: ac.u1, btn_minus: ac.u1
) -> ClockResult:
    prescaler: ac.u26 = 0
    sec: ac.u6 = 0
    minute: ac.u6 = 0
    hour: ac.u5 = 0
    mode: ac.u2 = 0
    blink: ac.u1 = 0
    # EncodeBcd's u6 input boundary zero-extends the u5 hour before arithmetic.
    hours_encoded = EncodeBcd(hour)
    minutes_encoded = EncodeBcd(minute)
    seconds_encoded = EncodeBcd(sec)

    @ac.rule
    def advance_clock() -> ClockResult:
        # Published outputs and all transition predicates use the old-Q snapshot.
        prescaler_q = prescaler
        sec_q = sec
        minute_q = minute
        hour_q = hour
        mode_q = mode
        blink_q = blink
        result = ClockResult(
            hours_bcd=hours_encoded.value,
            minutes_bcd=minutes_encoded.value,
            seconds_bcd=seconds_encoded.value,
            setting_mode=mode_q,
            colon_blink=blink_q,
        )

        tick_1hz: ac.u1 = prescaler_q == 49_999_999
        is_run: ac.u1 = mode_q == 0
        is_set_hour: ac.u1 = mode_q == 1
        is_set_min: ac.u1 = mode_q == 2
        is_set_sec: ac.u1 = mode_q == 3
        sec_wrap: ac.u1 = sec_q == 59
        min_wrap: ac.u1 = minute_q == 59
        hour_wrap: ac.u1 = hour_q == 23

        prescaler = 0 if tick_1hz else prescaler_q + 1
        tick_run = tick_1hz & is_run
        sec = (0 if sec_wrap else sec_q + 1) if tick_run else sec_q
        minute = (0 if min_wrap else minute_q + 1) if tick_run & sec_wrap else minute_q
        hour = (
            (0 if hour_wrap else hour_q + 1)
            if tick_run & sec_wrap & min_wrap
            else hour_q
        )

        # Plus is applied first; minus intentionally wins when both are asserted.
        hour = (0 if hour_wrap else hour_q + 1) if btn_plus & is_set_hour else hour
        minute = (0 if min_wrap else minute_q + 1) if btn_plus & is_set_min else minute
        sec = (0 if sec_wrap else sec_q + 1) if btn_plus & is_set_sec else sec
        hour = (23 if hour_q == 0 else hour_q - 1) if btn_minus & is_set_hour else hour
        minute = (
            (59 if minute_q == 0 else minute_q - 1)
            if btn_minus & is_set_min
            else minute
        )
        sec = (59 if sec_q == 0 else sec_q - 1) if btn_minus & is_set_sec else sec

        mode = (0 if mode_q == 3 else mode_q + 1) if btn_set else mode_q
        blink = ~blink_q if tick_1hz else blink_q
        return result

    return advance_clock()
