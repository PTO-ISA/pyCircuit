"""Bounded conversions and immutable Table updates with atomic queued fanout."""

# ruff: noqa: N802 -- hardware module name.

from pycircuit import module, queue, rule, struct, table, u1, u3, u4, u8


@struct
class Request:
    values: table[5, u8]
    raw_index: u8


@struct
class Decoded:
    wrapped: u3
    saturated: u4
    checked_value: u3
    checked_valid: u1
    advanced: u3
    selected: u8
    wrapped_two: u1
    wrapped_eight: u3
    wrapped_full: u8
    narrow_selected: u8
    zero_selected: u8
    checked_window_value: u4
    checked_window_valid: u1
    below_upper: u1
    cross_domain_ordered: u1
    restored: u3
    updated_selected: u8
    updated_first: u8
    updated_last: u8
    source_after_update: u8
    chained_first: u8
    chained_selected: u8
    updated_after_chain: u8


@struct
class Ready:
    wrapped: u1
    saturated: u1
    checked_value: u1
    checked_valid: u1
    advanced: u1
    selected: u1
    wrapped_two: u1
    wrapped_eight: u1
    wrapped_full: u1
    narrow_selected: u1
    zero_selected: u1
    checked_window_value: u1
    checked_window_valid: u1
    below_upper: u1
    cross_domain_ordered: u1
    restored: u1
    updated_selected: u1
    updated_first: u1
    updated_last: u1
    source_after_update: u1
    chained_first: u1
    chained_selected: u1
    updated_after_chain: u1


@struct
class U1Channel:
    valid: u1
    data: u1


@struct
class U3Channel:
    valid: u1
    data: u3


@struct
class U4Channel:
    valid: u1
    data: u4


@struct
class U8Channel:
    valid: u1
    data: u8


def decode(request: Request) -> Decoded:
    raw = request.raw_index
    values = request.values
    wrapped = raw % 5
    saturated = 4 if raw < 4 else (8 if raw > 8 else raw)
    checked_value = raw if raw < 5 else 0
    checked_valid = raw < 5
    advanced = wrapped + 1
    selected = values[checked_value]
    wrapped_two = raw % 2
    wrapped_eight = raw % 8
    wrapped_full = raw
    narrow_index = raw[:2]
    narrow_selected = values[narrow_index]
    zero_index = 0
    zero_selected = values[zero_index]
    checked_window_valid = (raw >= 4) & (raw <= 8)
    checked_window_value = raw if checked_window_valid else 4
    below_upper = wrapped < 5
    cross_domain_ordered = saturated > wrapped
    restored = advanced - 1

    updated = values
    updated[checked_value] = raw
    updated_selected = updated[checked_value]
    updated_first = updated[zero_index]
    updated_last = updated[4]
    source_after_update = values[checked_value]
    chained = updated
    chained[zero_index] = 99
    chained_first = chained[zero_index]
    chained_selected = chained[checked_value]
    updated_after_chain = updated[checked_value]

    return Decoded(
        wrapped=wrapped[:3],
        saturated=saturated[:4],
        checked_value=checked_value[:3],
        checked_valid=checked_valid,
        advanced=advanced[:3],
        selected=selected,
        wrapped_two=wrapped_two[:1],
        wrapped_eight=wrapped_eight[:3],
        wrapped_full=wrapped_full,
        narrow_selected=narrow_selected,
        zero_selected=zero_selected,
        checked_window_value=checked_window_value[:4],
        checked_window_valid=checked_window_valid,
        below_upper=below_upper,
        cross_domain_ordered=cross_domain_ordered,
        restored=restored[:3],
        updated_selected=updated_selected,
        updated_first=updated_first,
        updated_last=updated_last,
        source_after_update=source_after_update,
        chained_first=chained_first,
        chained_selected=chained_selected,
        updated_after_chain=updated_after_chain,
    )


@module
def BoundedIntegerOperations(
    valid: u1, data: Request, take: Ready
) -> tuple[
    u1,
    U3Channel,
    U4Channel,
    U3Channel,
    U1Channel,
    U3Channel,
    U8Channel,
    U1Channel,
    U3Channel,
    U8Channel,
    U8Channel,
    U8Channel,
    U4Channel,
    U1Channel,
    U1Channel,
    U1Channel,
    U3Channel,
    U8Channel,
    U8Channel,
    U8Channel,
    U8Channel,
    U8Channel,
    U8Channel,
    U8Channel,
]:
    ingress = queue[Request](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    wrapped_queue = queue[u3](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.wrapped
    )
    saturated_queue = queue[u4](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.saturated
    )
    checked_value_queue = queue[u3](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.checked_value
    )
    checked_valid_queue = queue[u1](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.checked_valid
    )
    advanced_queue = queue[u3](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.advanced
    )
    selected_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.selected
    )
    wrapped_two_queue = queue[u1](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.wrapped_two
    )
    wrapped_eight_queue = queue[u3](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.wrapped_eight
    )
    wrapped_full_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.wrapped_full
    )
    narrow_selected_queue = queue[u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.narrow_selected,
    )
    zero_selected_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.zero_selected
    )
    checked_window_value_queue = queue[u4](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.checked_window_value,
    )
    checked_window_valid_queue = queue[u1](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.checked_window_valid,
    )
    below_upper_queue = queue[u1](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.below_upper
    )
    cross_domain_ordered_queue = queue[u1](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.cross_domain_ordered,
    )
    restored_queue = queue[u3](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.restored
    )
    updated_selected_queue = queue[u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.updated_selected,
    )
    updated_first_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.updated_first
    )
    updated_last_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.updated_last
    )
    source_after_update_queue = queue[u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.source_after_update,
    )
    chained_first_queue = queue[u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take.chained_first
    )
    chained_selected_queue = queue[u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.chained_selected,
    )
    updated_after_chain_queue = queue[u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take.updated_after_chain,
    )

    @rule
    def transfer() -> None:
        request = ingress.pop()
        decoded = decode(request)
        wrapped_queue.push(decoded.wrapped)
        saturated_queue.push(decoded.saturated)
        checked_value_queue.push(decoded.checked_value)
        checked_valid_queue.push(decoded.checked_valid)
        advanced_queue.push(decoded.advanced)
        selected_queue.push(decoded.selected)
        wrapped_two_queue.push(decoded.wrapped_two)
        wrapped_eight_queue.push(decoded.wrapped_eight)
        wrapped_full_queue.push(decoded.wrapped_full)
        narrow_selected_queue.push(decoded.narrow_selected)
        zero_selected_queue.push(decoded.zero_selected)
        checked_window_value_queue.push(decoded.checked_window_value)
        checked_window_valid_queue.push(decoded.checked_window_valid)
        below_upper_queue.push(decoded.below_upper)
        cross_domain_ordered_queue.push(decoded.cross_domain_ordered)
        restored_queue.push(decoded.restored)
        updated_selected_queue.push(decoded.updated_selected)
        updated_first_queue.push(decoded.updated_first)
        updated_last_queue.push(decoded.updated_last)
        source_after_update_queue.push(decoded.source_after_update)
        chained_first_queue.push(decoded.chained_first)
        chained_selected_queue.push(decoded.chained_selected)
        updated_after_chain_queue.push(decoded.updated_after_chain)

    transfer()
    return (
        ingress.ready,
        U3Channel(valid=wrapped_queue.valid, data=wrapped_queue.data),
        U4Channel(valid=saturated_queue.valid, data=saturated_queue.data),
        U3Channel(valid=checked_value_queue.valid, data=checked_value_queue.data),
        U1Channel(valid=checked_valid_queue.valid, data=checked_valid_queue.data),
        U3Channel(valid=advanced_queue.valid, data=advanced_queue.data),
        U8Channel(valid=selected_queue.valid, data=selected_queue.data),
        U1Channel(valid=wrapped_two_queue.valid, data=wrapped_two_queue.data),
        U3Channel(valid=wrapped_eight_queue.valid, data=wrapped_eight_queue.data),
        U8Channel(valid=wrapped_full_queue.valid, data=wrapped_full_queue.data),
        U8Channel(valid=narrow_selected_queue.valid, data=narrow_selected_queue.data),
        U8Channel(valid=zero_selected_queue.valid, data=zero_selected_queue.data),
        U4Channel(
            valid=checked_window_value_queue.valid,
            data=checked_window_value_queue.data,
        ),
        U1Channel(
            valid=checked_window_valid_queue.valid,
            data=checked_window_valid_queue.data,
        ),
        U1Channel(valid=below_upper_queue.valid, data=below_upper_queue.data),
        U1Channel(
            valid=cross_domain_ordered_queue.valid,
            data=cross_domain_ordered_queue.data,
        ),
        U3Channel(valid=restored_queue.valid, data=restored_queue.data),
        U8Channel(valid=updated_selected_queue.valid, data=updated_selected_queue.data),
        U8Channel(valid=updated_first_queue.valid, data=updated_first_queue.data),
        U8Channel(valid=updated_last_queue.valid, data=updated_last_queue.data),
        U8Channel(
            valid=source_after_update_queue.valid,
            data=source_after_update_queue.data,
        ),
        U8Channel(valid=chained_first_queue.valid, data=chained_first_queue.data),
        U8Channel(valid=chained_selected_queue.valid, data=chained_selected_queue.data),
        U8Channel(
            valid=updated_after_chain_queue.valid,
            data=updated_after_chain_queue.data,
        ),
    )
