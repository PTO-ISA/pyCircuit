from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    u,
    wire_of,
)

MODE_RUN = 0
MODE_SET_HOUR = 1
MODE_SET_MIN = 2
MODE_SET_SEC = 3


def _to_bcd8(m: CycleAwareCircuit, domain: CycleAwareDomain, v, width: int):
    vw = wire_of(v)
    ten = u(width, 10)
    ones_w = vw % ten
    tens_w = vw // ten
    return cas(domain, m.cat(tens_w[0:4], ones_w[0:4]), cycle=v.cycle)


def build(
    m: CycleAwareCircuit, domain: CycleAwareDomain, clk_freq: int = 50_000_000
) -> None:
    prescaler_w = max((int(clk_freq) - 1).bit_length(), 1)

    prescaler = domain.signal(width=prescaler_w, reset_value=0, name="prescaler")
    sec = domain.signal(width=6, reset_value=0, name="sec")
    minute = domain.signal(width=6, reset_value=0, name="minute")
    hour = domain.signal(width=5, reset_value=0, name="hour")
    mode = domain.signal(width=2, reset_value=MODE_RUN, name="mode")
    blink = domain.signal(width=1, reset_value=0, name="blink")

    btn_set = cas(domain, m.input("btn_set", width=1), cycle=0)
    btn_plus = cas(domain, m.input("btn_plus", width=1), cycle=0)
    btn_minus = cas(domain, m.input("btn_minus", width=1), cycle=0)

    clk_max = u(prescaler_w, clk_freq - 1)
    zero_p = u(prescaler_w, 0)
    one_p = u(prescaler_w, 1)
    zero6 = u(6, 0)
    one6 = u(6, 1)
    fifty_nine6 = u(6, 59)
    zero5 = u(5, 0)
    one5 = u(5, 1)
    twenty_three5 = u(5, 23)

    mode_run_2 = u(2, MODE_RUN)
    mode_set_hour_2 = u(2, MODE_SET_HOUR)
    mode_set_min_2 = u(2, MODE_SET_MIN)
    mode_set_sec_2 = u(2, MODE_SET_SEC)
    one2 = u(2, 1)

    tick_1hz = prescaler == clk_max
    prescaler_n = mux(tick_1hz, zero_p, prescaler + one_p)

    is_run = mode == mode_run_2
    is_set_hour = mode == mode_set_hour_2
    is_set_min = mode == mode_set_min_2
    is_set_sec = mode == mode_set_sec_2

    sec_wrap = sec == fifty_nine6
    min_wrap = minute == fifty_nine6
    hr_wrap = hour == twenty_three5

    sec_next_run = mux(sec_wrap, zero6, sec + one6)
    min_next_run = mux(sec_wrap, mux(min_wrap, zero6, minute + one6), minute)
    hr_next_run = mux(sec_wrap & min_wrap, mux(hr_wrap, zero5, hour + one5), hour)

    tick_run = tick_1hz & is_run
    sec_n_tick = mux(tick_run, sec_next_run, sec)
    min_n_tick = mux(tick_run, min_next_run, minute)
    hr_n_tick = mux(tick_run, hr_next_run, hour)

    mode_next = mux(mode == mode_set_sec_2, mode_run_2, mode + one2)
    blink_next = mux(tick_1hz, ~blink, blink)

    hour_p = mux(
        btn_plus & is_set_hour,
        mux(hour == twenty_three5, zero5, hour + one5),
        hr_n_tick,
    )
    min_p = mux(
        btn_plus & is_set_min,
        mux(minute == fifty_nine6, zero6, minute + one6),
        min_n_tick,
    )
    sec_p = mux(
        btn_plus & is_set_sec, mux(sec == fifty_nine6, zero6, sec + one6), sec_n_tick
    )

    hour_n = mux(
        btn_minus & is_set_hour, mux(hour == zero5, twenty_three5, hour - one5), hour_p
    )
    min_n = mux(
        btn_minus & is_set_min, mux(minute == zero6, fifty_nine6, minute - one6), min_p
    )
    sec_n = mux(
        btn_minus & is_set_sec, mux(sec == zero6, fifty_nine6, sec - one6), sec_p
    )

    m.output("hours_bcd", wire_of(_to_bcd8(m, domain, hour, 5)))
    m.output("minutes_bcd", wire_of(_to_bcd8(m, domain, minute, 6)))
    m.output("seconds_bcd", wire_of(_to_bcd8(m, domain, sec, 6)))
    m.output("setting_mode", wire_of(mode))
    m.output("colon_blink", wire_of(blink))

    domain.next()

    prescaler <<= prescaler_n
    sec <<= sec_n
    minute <<= min_n
    hour <<= hour_n
    mode.assign(mode_next, when=btn_set)
    blink <<= blink_next


build.__pycircuit_name__ = "digital_clock"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="digital_clock", clk_freq=50_000_000, eager=True
        ).emit_mlir()
    )
