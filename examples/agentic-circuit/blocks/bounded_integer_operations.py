"""Verifier-visible bounded integers and dynamic immutable-array selection."""

from enum import Enum

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
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
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
    updated_values = request.values.with_element(checked_value, request.raw_index)
    updated_selected = updated_values[checked_value]
    updated_first = updated_values[zero_index]
    updated_last = updated_values[4]
    source_after_update = request.values[checked_value]
    chained_values = updated_values.with_element(zero_index, 99)
    chained_first = chained_values[zero_index]
    chained_selected = chained_values[checked_value]
    updated_after_chain = updated_values[checked_value]
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
        updated_selected,
        updated_first,
        updated_last,
        source_after_update,
        chained_first,
        chained_selected,
        updated_after_chain,
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
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
    ac.u8,
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
        updated_selected,
        updated_first,
        updated_last,
        source_after_update,
        chained_first,
        chained_selected,
        updated_after_chain,
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
        updated_selected,
        updated_first,
        updated_last,
        source_after_update,
        chained_first,
        chained_selected,
        updated_after_chain,
    )


class ArrayMode(Enum):
    IDLE = 0
    RUN = 1


@ac.struct
class ArrayInner:
    tag: ac.u8
    mode: ArrayMode
    ordinal: ac.index[3]


@ac.struct
class RecursiveArrays:
    structs: ac.array[3, ArrayInner]
    enums: ac.array[5, ArrayMode]
    ranges: ac.array[3, ac.index[3]]
    wide: ac.array[65, ac.u3]


@ac.struct
class RecursiveArrayRequest:
    raw_index: ac.u8
    replacement: ac.u8


@ac.struct
class RecursiveArrayResult:
    source_struct_tag: ac.u8
    updated_struct_tag: ac.u8
    source_struct_mode: ArrayMode
    updated_struct_mode: ArrayMode
    source_enum: ArrayMode
    updated_enum: ArrayMode
    source_range: ac.index[3]
    updated_range: ac.index[3]
    source_wide: ac.u3
    updated_wide: ac.u3
    wide_first: ac.u3
    wide_last: ac.u3
    chained_selected: ac.u3
    updated_after_chain: ac.u3


@ac.rule
def update_recursive_arrays(
    request: RecursiveArrayRequest,
) -> RecursiveArrayResult:
    struct_index = ac.wrap(request.raw_index, ac.index[3])
    enum_index = ac.wrap(request.raw_index, ac.index[5])
    wide_index = ac.wrap(request.raw_index, ac.index[65])
    zero_index = ac.wrap(request.raw_index, ac.index[1])
    source_range = ac.wrap(request.raw_index, ac.index[3])
    replacement_range = ac.wrap(request.replacement, ac.index[3])
    source_wide = ac.truncate(request.raw_index, ac.u3)
    replacement_wide = ac.truncate(request.replacement, ac.u3)
    seed = ArrayInner(
        tag=request.raw_index,
        mode=ArrayMode.IDLE,
        ordinal=source_range,
    )
    values = RecursiveArrays(
        structs=(seed, seed, seed),
        enums=(
            ArrayMode.IDLE,
            ArrayMode.IDLE,
            ArrayMode.IDLE,
            ArrayMode.IDLE,
            ArrayMode.IDLE,
        ),
        ranges=(source_range, source_range, source_range),
        wide=(
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
            source_wide,
        ),
    )
    replacement_inner = ArrayInner(
        tag=request.replacement,
        mode=ArrayMode.RUN,
        ordinal=replacement_range,
    )
    updated_structs = values.structs.with_element(struct_index, replacement_inner)
    updated_enums = values.enums.with_element(enum_index, ArrayMode.RUN)
    updated_ranges = values.ranges.with_element(struct_index, replacement_range)
    updated_wide = values.wide.with_element(wide_index, replacement_wide)
    chained_wide = updated_wide.with_element(zero_index, 7)
    source_struct = values.structs[struct_index]
    updated_struct = updated_structs[struct_index]
    return RecursiveArrayResult(
        source_struct_tag=source_struct.tag,
        updated_struct_tag=updated_struct.tag,
        source_struct_mode=source_struct.mode,
        updated_struct_mode=updated_struct.mode,
        source_enum=values.enums[enum_index],
        updated_enum=updated_enums[enum_index],
        source_range=values.ranges[struct_index],
        updated_range=updated_ranges[struct_index],
        source_wide=values.wide[wide_index],
        updated_wide=updated_wide[wide_index],
        wide_first=updated_wide[zero_index],
        wide_last=updated_wide[64],
        chained_selected=chained_wide[wide_index],
        updated_after_chain=updated_wide[wide_index],
    )


@ac.system
def recursive_array_updates(
    request: RecursiveArrayRequest,
) -> RecursiveArrayResult:
    result = update_recursive_arrays(request)
    return result


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
