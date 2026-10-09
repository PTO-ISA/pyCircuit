"""Immutable updates over nested nominal Tables, including all 65 wide lanes."""

# ruff: noqa: F821, N802 -- forward stage-ready wire and hardware module name.

from enum import Enum

from pycircuit import encoding, module, queue, rule, struct, table, u1, u2, u3, u8


@encoding(width=1)
class ArrayMode(Enum):
    IDLE = 0
    RUN = 1


@struct
class ArrayInner:
    tag: u8
    mode: ArrayMode
    ordinal: u2


@struct
class RecursiveArrays:
    structs: table[3, ArrayInner]
    enums: table[5, ArrayMode]
    ranges: table[3, u2]
    wide: table[65, u3]


@struct
class Request:
    raw_index: u8
    replacement: u8


@struct
class RecursiveArrayResult:
    source_struct_tag: u8
    updated_struct_tag: u8
    source_struct_mode: ArrayMode
    updated_struct_mode: ArrayMode
    source_enum: ArrayMode
    updated_enum: ArrayMode
    source_range: u2
    updated_range: u2
    source_wide: u3
    updated_wide: u3
    wide_first: u3
    wide_last: u3
    chained_selected: u3
    updated_after_chain: u3


@struct
class Result:
    ready: u1
    valid: u1
    data: RecursiveArrayResult


@rule
def update_recursive_arrays(request: Request) -> RecursiveArrayResult:
    struct_index = request.raw_index % 3
    enum_index = request.raw_index % 5
    wide_index = request.raw_index % 65
    zero_index = 0
    source_range = struct_index[:2]
    replacement_range = (request.replacement % 3)[:2]
    source_wide = request.raw_index[:3]
    replacement_wide = request.replacement[:3]
    seed = ArrayInner(
        tag=request.raw_index,
        mode=ArrayMode.IDLE,
        ordinal=source_range,
    )
    zero = RecursiveArrays()
    values = RecursiveArrays(
        structs=zero.structs.map(lambda item: seed),
        enums=zero.enums,
        ranges=zero.ranges.map(lambda value: source_range),
        wide=zero.wide.map(lambda value: source_wide),
    )
    replacement_inner = ArrayInner(
        tag=request.replacement,
        mode=ArrayMode.RUN,
        ordinal=replacement_range,
    )

    updated_structs = values.structs
    updated_structs[struct_index] = replacement_inner
    updated_enums = values.enums
    updated_enums[enum_index] = ArrayMode.RUN
    updated_ranges = values.ranges
    updated_ranges[struct_index] = replacement_range
    updated_wide = values.wide
    updated_wide[wide_index] = replacement_wide
    chained_wide = updated_wide
    chained_wide[zero_index] = 7

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


@module
def RecursiveArrayUpdates(valid: u1, data: Request, take: u1) -> Result:
    ready, available, request = queue[Request](
        valid,
        data,
        stage_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    updated = update_recursive_arrays(request)
    stage_ready, output_valid, result = queue[RecursiveArrayResult](
        available,
        updated,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=output_valid, data=result)
