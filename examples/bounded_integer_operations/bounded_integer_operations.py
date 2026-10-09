"""Bounded conversions and immutable Table updates with atomic queued fanout."""

# ruff: noqa: F821, N802 -- forward queue-result wires and hardware module names.

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
class Result:
    ready: u1
    valid: Ready
    data: Decoded


@rule
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
def BoundedIntegerOperations(valid: u1, data: Request, take: Ready) -> Result:
    all_ready = (
        wrapped_ready
        & saturated_ready
        & checked_value_ready
        & checked_valid_ready
        & advanced_ready
        & selected_ready
        & wrapped_two_ready
        & wrapped_eight_ready
        & wrapped_full_ready
        & narrow_selected_ready
        & zero_selected_ready
        & checked_window_value_ready
        & checked_window_valid_ready
        & below_upper_ready
        & cross_domain_ordered_ready
        & restored_ready
        & updated_selected_ready
        & updated_first_ready
        & updated_last_ready
        & source_after_update_ready
        & chained_first_ready
        & chained_selected_ready
        & updated_after_chain_ready
    )
    ready, source_valid, request = queue[Request](
        valid,
        data,
        all_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    decoded = decode(request)
    fire = source_valid & all_ready
    wrapped_ready, wrapped_valid, wrapped = queue[u3](
        fire,
        decoded.wrapped,
        take.wrapped,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    saturated_ready, saturated_valid, saturated = queue[u4](
        fire,
        decoded.saturated,
        take.saturated,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    checked_value_ready, checked_value_valid, checked_value = queue[u3](
        fire,
        decoded.checked_value,
        take.checked_value,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    checked_valid_ready, checked_valid_valid, checked_valid = queue[u1](
        fire,
        decoded.checked_valid,
        take.checked_valid,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    advanced_ready, advanced_valid, advanced = queue[u3](
        fire,
        decoded.advanced,
        take.advanced,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    selected_ready, selected_valid, selected = queue[u8](
        fire,
        decoded.selected,
        take.selected,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    wrapped_two_ready, wrapped_two_valid, wrapped_two = queue[u1](
        fire,
        decoded.wrapped_two,
        take.wrapped_two,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    wrapped_eight_ready, wrapped_eight_valid, wrapped_eight = queue[u3](
        fire,
        decoded.wrapped_eight,
        take.wrapped_eight,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    wrapped_full_ready, wrapped_full_valid, wrapped_full = queue[u8](
        fire,
        decoded.wrapped_full,
        take.wrapped_full,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    narrow_selected_ready, narrow_selected_valid, narrow_selected = queue[u8](
        fire,
        decoded.narrow_selected,
        take.narrow_selected,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    zero_selected_ready, zero_selected_valid, zero_selected = queue[u8](
        fire,
        decoded.zero_selected,
        take.zero_selected,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    (
        checked_window_value_ready,
        checked_window_value_valid,
        checked_window_value,
    ) = queue[u4](
        fire,
        decoded.checked_window_value,
        take.checked_window_value,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    (
        checked_window_valid_ready,
        checked_window_valid_valid,
        checked_window_valid,
    ) = queue[u1](
        fire,
        decoded.checked_window_valid,
        take.checked_window_valid,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    below_upper_ready, below_upper_valid, below_upper = queue[u1](
        fire,
        decoded.below_upper,
        take.below_upper,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    (
        cross_domain_ordered_ready,
        cross_domain_ordered_valid,
        cross_domain_ordered,
    ) = queue[u1](
        fire,
        decoded.cross_domain_ordered,
        take.cross_domain_ordered,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    restored_ready, restored_valid, restored = queue[u3](
        fire,
        decoded.restored,
        take.restored,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    updated_selected_ready, updated_selected_valid, updated_selected = queue[u8](
        fire,
        decoded.updated_selected,
        take.updated_selected,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    updated_first_ready, updated_first_valid, updated_first = queue[u8](
        fire,
        decoded.updated_first,
        take.updated_first,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    updated_last_ready, updated_last_valid, updated_last = queue[u8](
        fire,
        decoded.updated_last,
        take.updated_last,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    source_after_update_ready, source_after_update_valid, source_after_update = queue[
        u8
    ](
        fire,
        decoded.source_after_update,
        take.source_after_update,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    chained_first_ready, chained_first_valid, chained_first = queue[u8](
        fire,
        decoded.chained_first,
        take.chained_first,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    chained_selected_ready, chained_selected_valid, chained_selected = queue[u8](
        fire,
        decoded.chained_selected,
        take.chained_selected,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    updated_after_chain_ready, updated_after_chain_valid, updated_after_chain = queue[
        u8
    ](
        fire,
        decoded.updated_after_chain,
        take.updated_after_chain,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return Result(
        ready=ready,
        valid=Ready(
            wrapped=wrapped_valid,
            saturated=saturated_valid,
            checked_value=checked_value_valid,
            checked_valid=checked_valid_valid,
            advanced=advanced_valid,
            selected=selected_valid,
            wrapped_two=wrapped_two_valid,
            wrapped_eight=wrapped_eight_valid,
            wrapped_full=wrapped_full_valid,
            narrow_selected=narrow_selected_valid,
            zero_selected=zero_selected_valid,
            checked_window_value=checked_window_value_valid,
            checked_window_valid=checked_window_valid_valid,
            below_upper=below_upper_valid,
            cross_domain_ordered=cross_domain_ordered_valid,
            restored=restored_valid,
            updated_selected=updated_selected_valid,
            updated_first=updated_first_valid,
            updated_last=updated_last_valid,
            source_after_update=source_after_update_valid,
            chained_first=chained_first_valid,
            chained_selected=chained_selected_valid,
            updated_after_chain=updated_after_chain_valid,
        ),
        data=Decoded(
            wrapped=wrapped,
            saturated=saturated,
            checked_value=checked_value,
            checked_valid=checked_valid,
            advanced=advanced,
            selected=selected,
            wrapped_two=wrapped_two,
            wrapped_eight=wrapped_eight,
            wrapped_full=wrapped_full,
            narrow_selected=narrow_selected,
            zero_selected=zero_selected,
            checked_window_value=checked_window_value,
            checked_window_valid=checked_window_valid,
            below_upper=below_upper,
            cross_domain_ordered=cross_domain_ordered,
            restored=restored,
            updated_selected=updated_selected,
            updated_first=updated_first,
            updated_last=updated_last,
            source_after_update=source_after_update,
            chained_first=chained_first,
            chained_selected=chained_selected,
            updated_after_chain=updated_after_chain,
        ),
    )
