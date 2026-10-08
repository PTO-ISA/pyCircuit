"""A fixed 50 MHz clock with raw, repeating setting buttons."""

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


@ac.rule
def encode_bcd(value) -> BcdResult:
    tens = value // 10
    ones = value % 10
    tens8: ac.u8 = tens[:4]
    ones8: ac.u8 = ones[:4]
    return BcdResult(value=(tens8 << 4) | ones8)


@ac.module
def EncodeBcd(value: ac.u6) -> BcdResult:  # noqa: N802
    return encode_bcd(value)


@ac.rule
def advance_clock(
    prescaler,
    sec,
    minute,
    hour,
    mode,
    blink,
    btn_set,
    btn_plus,
    btn_minus,
    hours_encoded,
    minutes_encoded,
    seconds_encoded,
) -> ClockResult:
    result = ClockResult(
        hours_bcd=hours_encoded.value,
        minutes_bcd=minutes_encoded.value,
        seconds_bcd=seconds_encoded.value,
        setting_mode=mode,
        colon_blink=blink,
    )

    tick_1hz: ac.u1 = prescaler == 49_999_999
    is_run: ac.u1 = mode == 0
    is_set_hour: ac.u1 = mode == 1
    is_set_min: ac.u1 = mode == 2
    is_set_sec: ac.u1 = mode == 3
    sec_wrap: ac.u1 = sec == 59
    min_wrap: ac.u1 = minute == 59
    hour_wrap: ac.u1 = hour == 23

    prescaler_value: ac.u26 = 0 if tick_1hz else prescaler + 1
    sec_run: ac.u6 = 0 if sec_wrap else sec + 1
    min_run: ac.u6 = (0 if min_wrap else minute + 1) if sec_wrap else minute
    hour_run: ac.u5 = (0 if hour_wrap else hour + 1) if sec_wrap & min_wrap else hour
    tick_run = tick_1hz & is_run
    sec_tick = sec_run if tick_run else sec
    min_tick = min_run if tick_run else minute
    hour_tick = hour_run if tick_run else hour

    mode_value: ac.u2 = 0 if mode == 3 else mode + 1
    blink_value = ~blink if tick_1hz else blink

    hour_plus: ac.u5 = (
        (0 if hour_wrap else hour + 1) if btn_plus & is_set_hour else hour_tick
    )
    min_plus: ac.u6 = (
        (0 if min_wrap else minute + 1) if btn_plus & is_set_min else min_tick
    )
    sec_plus: ac.u6 = (
        (0 if sec_wrap else sec + 1) if btn_plus & is_set_sec else sec_tick
    )
    hour_value: ac.u5 = (
        (23 if hour == 0 else hour - 1) if btn_minus & is_set_hour else hour_plus
    )
    min_value: ac.u6 = (
        (59 if minute == 0 else minute - 1) if btn_minus & is_set_min else min_plus
    )
    sec_value: ac.u6 = (
        (59 if sec == 0 else sec - 1) if btn_minus & is_set_sec else sec_plus
    )

    prescaler = prescaler_value
    sec = sec_value
    minute = min_value
    hour = hour_value
    mode = mode_value if btn_set else mode
    blink = blink_value
    return result


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
    return advance_clock(
        prescaler,
        sec,
        minute,
        hour,
        mode,
        blink,
        btn_set,
        btn_plus,
        btn_minus,
        hours_encoded,
        minutes_encoded,
        seconds_encoded,
    )
