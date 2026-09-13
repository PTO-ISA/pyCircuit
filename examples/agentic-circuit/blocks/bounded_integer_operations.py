"""Verifier-visible bounded integers and dynamic immutable-array selection."""

import agentic_circuit as ac


@ac.struct
class Request:
    values: ac.array[5, ac.u8]
    raw_index: ac.u8


@ac.rule
def decode(
    request: Request,
) -> tuple[
    ac.index[5],
    ac.range[4, 9],
    ac.index[5],
    bool,
    ac.range[1, 6],
    ac.u8,
    ac.index[2],
    ac.index[8],
    ac.index[256],
    ac.u8,
    ac.u8,
    ac.range[4, 9],
    bool,
    bool,
    bool,
    ac.index[5],
]:
    wrapped = ac.wrap(request.raw_index, ac.index[5])
    saturated = ac.saturate(request.raw_index, ac.range[4, 9])
    checked = ac.checked(request.raw_index, ac.index[5])
    checked_value = checked.value
    checked_valid = checked.valid
    advanced = wrapped + 1
    selected = request.values[checked_value]
    wrapped_two = ac.wrap(request.raw_index, ac.index[2])
    wrapped_eight = ac.wrap(request.raw_index, ac.index[8])
    wrapped_full = ac.wrap(request.raw_index, ac.index[256])
    narrow_index = ac.truncate(request.raw_index, ac.u2)
    narrow_selected = request.values[narrow_index]
    zero_index = ac.wrap(request.raw_index, ac.index[1])
    zero_selected = request.values[zero_index]
    checked_window = ac.checked(request.raw_index, ac.range[4, 9])
    checked_window_value = checked_window.value
    checked_window_valid = checked_window.valid
    below_upper = wrapped < 5
    cross_domain_ordered = saturated > wrapped
    restored = advanced - 1
    return (
        wrapped,
        saturated,
        checked_value,
        checked_valid,
        advanced,
        selected,
        wrapped_two,
        wrapped_eight,
        wrapped_full,
        narrow_selected,
        zero_selected,
        checked_window_value,
        checked_window_valid,
        below_upper,
        cross_domain_ordered,
        restored,
    )


@ac.system
def bounded_integer_operations(
    request: Request,
) -> tuple[
    ac.index[5],
    ac.range[4, 9],
    ac.index[5],
    bool,
    ac.range[1, 6],
    ac.u8,
    ac.index[2],
    ac.index[8],
    ac.index[256],
    ac.u8,
    ac.u8,
    ac.range[4, 9],
    bool,
    bool,
    bool,
    ac.index[5],
]:
    (
        wrapped,
        saturated,
        checked_value,
        checked_valid,
        advanced,
        selected,
        wrapped_two,
        wrapped_eight,
        wrapped_full,
        narrow_selected,
        zero_selected,
        checked_window_value,
        checked_window_valid,
        below_upper,
        cross_domain_ordered,
        restored,
    ) = decode(request)
    return (
        wrapped,
        saturated,
        checked_value,
        checked_valid,
        advanced,
        selected,
        wrapped_two,
        wrapped_eight,
        wrapped_full,
        narrow_selected,
        zero_selected,
        checked_window_value,
        checked_window_valid,
        below_upper,
        cross_domain_ordered,
        restored,
    )


@ac.rule
def decode_full_u64(
    raw: ac.u64,
) -> tuple[
    ac.range[0, 18446744073709551616],
    ac.range[0, 18446744073709551616],
    ac.range[0, 18446744073709551616],
    bool,
]:
    wrapped = ac.wrap(raw, ac.range[0, 18446744073709551616])
    saturated = ac.saturate(raw, ac.range[0, 18446744073709551616])
    checked = ac.checked(raw, ac.range[0, 18446744073709551616])
    checked_value = checked.value
    checked_valid = checked.valid
    return wrapped, saturated, checked_value, checked_valid


@ac.system
def bounded_full_u64(
    raw: ac.u64,
) -> tuple[
    ac.range[0, 18446744073709551616],
    ac.range[0, 18446744073709551616],
    ac.range[0, 18446744073709551616],
    bool,
]:
    wrapped, saturated, checked_value, checked_valid = decode_full_u64(raw)
    return wrapped, saturated, checked_value, checked_valid
