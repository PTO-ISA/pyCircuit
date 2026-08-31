"""Traffic Lights Controller — pyCircuit v6 design.

Reimplements the Traffic-lights-ce project in the pyCircuit unified signal model.
Outputs are BCD countdowns per direction plus discrete red/yellow/green lights.

JIT parameters:
  CLK_FREQ     — system clock frequency in Hz (default 50 MHz)
  EW_GREEN_S   — east/west green time in seconds
  EW_YELLOW_S  — east/west yellow time in seconds
  NS_GREEN_S   — north/south green time in seconds
  NS_YELLOW_S  — north/south yellow time in seconds

Derived:
  EW_RED_S = NS_GREEN_S + NS_YELLOW_S
  NS_RED_S = EW_GREEN_S + EW_YELLOW_S
"""

from __future__ import annotations

import os

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    function,
    u,
)

# Phase encoding
PH_EW_GREEN = 0
PH_EW_YELLOW = 1
PH_NS_GREEN = 2
PH_NS_YELLOW = 3


@function
def bin_to_bcd_60(m: Circuit, val, width):
    """Convert 0-59 binary value to 8-bit packed BCD (tens in [7:4], units in [3:0])."""
    tens = (
        u(4, 5)
        if (val >= u(width, 50))
        else (
            u(4, 4)
            if (val >= u(width, 40))
            else (
                u(4, 3)
                if (val >= u(width, 30))
                else (
                    u(4, 2)
                    if (val >= u(width, 20))
                    else u(4, 1) if (val >= u(width, 10)) else u(4, 0)
                )
            )
        )
    )
    tens_w = tens | u(width, 0)
    units = (val - tens_w * u(width, 10))[0:4]
    return (tens | u(8, 0)) << 4 | (units | u(8, 0))


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    CLK_FREQ: int = 50_000_000,
    EW_GREEN_S: int = 45,
    EW_YELLOW_S: int = 5,
    NS_GREEN_S: int = 30,
    NS_YELLOW_S: int = 5,
) -> None:
    if min(EW_GREEN_S, EW_YELLOW_S, NS_GREEN_S, NS_YELLOW_S) <= 0:
        raise ValueError("all durations must be > 0")

    EW_RED_S = NS_GREEN_S + NS_YELLOW_S
    NS_RED_S = EW_GREEN_S + EW_YELLOW_S

    max_dur = max(EW_GREEN_S, EW_YELLOW_S, NS_GREEN_S, NS_YELLOW_S, EW_RED_S, NS_RED_S)
    if max_dur > 59:
        raise ValueError("all durations must be <= 59 to fit bin_to_bcd_60")
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    # ================================================================
    # Inputs
    # ================================================================
    go = m.input("go", width=1)
    emergency = m.input("emergency", width=1)

    # ================================================================
    # Registers
    # ================================================================
    PRESCALER_W = max((CLK_FREQ - 1).bit_length(), 1)
    CNT_W = max(max_dur.bit_length(), 1)

    prescaler_r = m.out(
        "prescaler", domain=cd, width=PRESCALER_W, init=u(PRESCALER_W, 0)
    )
    phase_r = m.out("phase", domain=cd, width=2, init=u(2, PH_EW_GREEN))
    ew_cnt_r = m.out("ew_cnt", domain=cd, width=CNT_W, init=u(CNT_W, EW_GREEN_S))
    ns_cnt_r = m.out("ns_cnt", domain=cd, width=CNT_W, init=u(CNT_W, NS_RED_S))
    blink_r = m.out("blink", domain=cd, width=1, init=u(1, 0))

    # ================================================================
    # Combinational logic
    # ================================================================
    pv = prescaler_r.out()
    ph = phase_r.out()
    ew = ew_cnt_r.out()
    ns = ns_cnt_r.out()
    bl = blink_r.out()

    en = go & (~emergency)

    # 1 Hz tick via prescaler (gated by en)
    tick_raw = pv == u(PRESCALER_W, CLK_FREQ - 1)
    tick_1hz = tick_raw & en
    inner_prescaler = u(PRESCALER_W, 0) if tick_raw else (pv + 1)
    prescaler_next = inner_prescaler if en else pv

    # Phase flags
    is_ew_green = ph == u(2, PH_EW_GREEN)
    is_ew_yellow = ph == u(2, PH_EW_YELLOW)
    is_ns_green = ph == u(2, PH_NS_GREEN)
    is_ns_yellow = ph == u(2, PH_NS_YELLOW)
    yellow_active = is_ew_yellow | is_ns_yellow

    # Countdown end flags
    ew_end = ew == u(CNT_W, 0)
    ns_end = ns == u(CNT_W, 0)

    ew_cnt_dec = ew - 1
    ns_cnt_dec = ns - 1

    # Phase transitions (when counter reaches 0 on a tick)
    cond_ew_to_yellow = tick_1hz & is_ew_green & ew_end
    cond_ew_to_ns_green = tick_1hz & is_ew_yellow & ew_end
    cond_ns_to_yellow = tick_1hz & is_ns_green & ns_end
    cond_ns_to_ew_green = tick_1hz & is_ns_yellow & ns_end

    phase_next = ph
    phase_next = u(2, PH_EW_YELLOW) if cond_ew_to_yellow else phase_next
    phase_next = u(2, PH_NS_GREEN) if cond_ew_to_ns_green else phase_next
    phase_next = u(2, PH_NS_YELLOW) if cond_ns_to_yellow else phase_next
    phase_next = u(2, PH_EW_GREEN) if cond_ns_to_ew_green else phase_next

    # EW countdown
    ew_cnt_next = ew
    ew_cnt_next = ew_cnt_dec if (tick_1hz & (~ew_end)) else ew_cnt_next
    ew_cnt_next = u(CNT_W, EW_YELLOW_S) if cond_ew_to_yellow else ew_cnt_next
    ew_cnt_next = u(CNT_W, EW_RED_S) if cond_ew_to_ns_green else ew_cnt_next
    ew_cnt_next = u(CNT_W, EW_GREEN_S) if cond_ns_to_ew_green else ew_cnt_next

    # NS countdown
    ns_cnt_next = ns
    ns_cnt_next = ns_cnt_dec if (tick_1hz & (~ns_end)) else ns_cnt_next
    ns_cnt_next = u(CNT_W, NS_GREEN_S) if cond_ew_to_ns_green else ns_cnt_next
    ns_cnt_next = u(CNT_W, NS_YELLOW_S) if cond_ns_to_yellow else ns_cnt_next
    ns_cnt_next = u(CNT_W, NS_RED_S) if cond_ns_to_ew_green else ns_cnt_next

    # BCD conversion (combinational)
    ew_bcd_raw = bin_to_bcd_60(m, ew, CNT_W)
    ns_bcd_raw = bin_to_bcd_60(m, ns, CNT_W)

    # Lights (base, before emergency override)
    ew_red_base = is_ns_green | is_ns_yellow
    ew_green_base = is_ew_green
    ew_yellow_base = is_ew_yellow & bl

    ns_red_base = is_ew_green | is_ew_yellow
    ns_green_base = is_ns_green
    ns_yellow_base = is_ns_yellow & bl

    # Emergency overrides
    ew_bcd = u(8, 0x88) if emergency else ew_bcd_raw
    ns_bcd = u(8, 0x88) if emergency else ns_bcd_raw

    ew_red = u(1, 1) if emergency else ew_red_base
    ew_yellow = u(1, 0) if emergency else ew_yellow_base
    ew_green = u(1, 0) if emergency else ew_green_base

    ns_red = u(1, 1) if emergency else ns_red_base
    ns_yellow = u(1, 0) if emergency else ns_yellow_base
    ns_green = u(1, 0) if emergency else ns_green_base

    # ================================================================
    # Register updates
    # ================================================================
    prescaler_r.set(prescaler_next)
    phase_r.set(phase_next)
    ew_cnt_r.set(ew_cnt_next)
    ns_cnt_r.set(ns_cnt_next)

    # Blink: reset to 0 when not in yellow; toggle on tick_1hz while yellow.
    blink_r.set(u(1, 0), when=~yellow_active)
    blink_r.set(~bl, when=tick_1hz & yellow_active)

    # ================================================================
    # Outputs
    # ================================================================
    m.output("ew_bcd", ew_bcd)
    m.output("ns_bcd", ns_bcd)
    m.output("ew_red", ew_red)
    m.output("ew_yellow", ew_yellow)
    m.output("ew_green", ew_green)
    m.output("ns_red", ns_red)
    m.output("ns_yellow", ns_yellow)
    m.output("ns_green", ns_green)


build.__pycircuit_name__ = "traffic_lights_ce_pyc"

if __name__ == "__main__":

    def _env_int(key: str, default: int) -> int:
        raw = os.getenv(key)
        if raw is None:
            return default
        try:
            return int(raw, 0)
        except ValueError as exc:
            raise ValueError(f"invalid {key}={raw!r}") from exc

    print(
        compile_cycle_aware(
            build,
            name="traffic_lights_ce_pyc",
            CLK_FREQ=_env_int("PYC_TL_CLK_FREQ", 50_000_000),
            EW_GREEN_S=_env_int("PYC_TL_EW_GREEN_S", 45),
            EW_YELLOW_S=_env_int("PYC_TL_EW_YELLOW_S", 5),
            NS_GREEN_S=_env_int("PYC_TL_NS_GREEN_S", 30),
            NS_YELLOW_S=_env_int("PYC_TL_NS_YELLOW_S", 5),
        ).emit_mlir()
    )
