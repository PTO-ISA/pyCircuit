"""Traffic controller preserving the historical three-bit display encoding."""

import pycircuit as ac


@ac.struct
class BcdResult:
    value: ac.u8


@ac.struct
class TrafficResult:
    ew_bcd: ac.u8
    ns_bcd: ac.u8
    ew_red: ac.u1
    ew_yellow: ac.u1
    ew_green: ac.u1
    ns_red: ac.u1
    ns_yellow: ac.u1
    ns_green: ac.u1


@ac.rule
def encode_countdown(count) -> BcdResult:
    # The original three-bit literals truncate before widening or comparison.
    threshold50: ac.u3 = 50 & 7
    threshold40: ac.u3 = 40 & 7
    threshold30: ac.u3 = 30 & 7
    threshold20: ac.u3 = 20 & 7
    threshold10: ac.u3 = 10 & 7
    tens: ac.u4 = (
        5
        if count >= threshold50
        else (
            4
            if count >= threshold40
            else (
                3
                if count >= threshold30
                else 2 if count >= threshold20 else 1 if count >= threshold10 else 0
            )
        )
    )
    zero3: ac.u3 = 0
    zero4: ac.u4 = zero3
    tens_w = tens | zero4
    count4: ac.u4 = count
    factor4: ac.u4 = threshold10
    units = (count4 - tens_w * factor4)[:4]
    tens8: ac.u8 = tens
    units8: ac.u8 = units
    return BcdResult(value=((tens8 | 0) << 4) | (units8 | 0))


@ac.module
def EncodeCountdown(count: ac.u3) -> BcdResult:  # noqa: N802
    return encode_countdown(count)


@ac.rule
def advance_traffic(
    prescaler, phase, ew_count, ns_count, blink, go, emergency, ew_encoded, ns_encoded
) -> TrafficResult:
    enable = go & ~emergency
    tick_raw: ac.u1 = prescaler == 3
    tick_1hz = tick_raw & enable
    is_ew_green: ac.u1 = phase == 0
    is_ew_yellow: ac.u1 = phase == 1
    is_ns_green: ac.u1 = phase == 2
    is_ns_yellow: ac.u1 = phase == 3
    yellow_active = is_ew_yellow | is_ns_yellow
    ew_end: ac.u1 = ew_count == 0
    ns_end: ac.u1 = ns_count == 0

    ew_red_base = is_ns_green | is_ns_yellow
    ew_yellow_base = is_ew_yellow & blink
    ns_red_base = is_ew_green | is_ew_yellow
    ns_yellow_base = is_ns_yellow & blink
    result = TrafficResult(
        ew_bcd=0x88 if emergency else ew_encoded.value,
        ns_bcd=0x88 if emergency else ns_encoded.value,
        ew_red=1 if emergency else ew_red_base,
        ew_yellow=0 if emergency else ew_yellow_base,
        ew_green=0 if emergency else is_ew_green,
        ns_red=1 if emergency else ns_red_base,
        ns_yellow=0 if emergency else ns_yellow_base,
        ns_green=0 if emergency else is_ns_green,
    )

    inner_prescaler: ac.u2 = 0 if tick_raw else prescaler + 1
    prescaler_value = inner_prescaler if enable else prescaler
    ew_to_yellow = tick_1hz & is_ew_green & ew_end
    ew_to_ns_green = tick_1hz & is_ew_yellow & ew_end
    ns_to_yellow = tick_1hz & is_ns_green & ns_end
    ns_to_ew_green = tick_1hz & is_ns_yellow & ns_end

    phase_value = phase
    phase_value = 1 if ew_to_yellow else phase_value
    phase_value = 2 if ew_to_ns_green else phase_value
    phase_value = 3 if ns_to_yellow else phase_value
    phase_value = 0 if ns_to_ew_green else phase_value

    ew_value = ew_count
    ew_value = ew_count - 1 if tick_1hz & ~ew_end else ew_value
    ew_value = 1 if ew_to_yellow else ew_value
    ew_value = 3 if ew_to_ns_green else ew_value
    ew_value = 3 if ns_to_ew_green else ew_value

    ns_value = ns_count
    ns_value = ns_count - 1 if tick_1hz & ~ns_end else ns_value
    ns_value = 2 if ew_to_ns_green else ns_value
    ns_value = 1 if ns_to_yellow else ns_value
    ns_value = 4 if ns_to_ew_green else ns_value

    blink_value = 0 if ~yellow_active else blink
    blink_value = ~blink if tick_1hz & yellow_active else blink_value
    prescaler = prescaler_value
    phase = phase_value
    ew_count = ew_value
    ns_count = ns_value
    blink = blink_value
    return result


@ac.module
def TopTrafficLights(go: ac.u1, emergency: ac.u1) -> TrafficResult:  # noqa: N802
    prescaler: ac.u2 = 0
    phase: ac.u2 = 0
    ew_count: ac.u3 = 3
    ns_count: ac.u3 = 4
    blink: ac.u1 = 0
    ew_encoded = EncodeCountdown(ew_count)
    ns_encoded = EncodeCountdown(ns_count)
    return advance_traffic(
        prescaler,
        phase,
        ew_count,
        ns_count,
        blink,
        go,
        emergency,
        ew_encoded,
        ns_encoded,
    )
