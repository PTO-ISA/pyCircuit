"""Fixed-table map combinators through the standard two-stage queue contract."""

# ruff: noqa: F821, N802 -- the first queue consumes the later stage-ready wire.
from pycircuit import module, queue, rule, struct, table, u1, u2, u3, u4, u8


@struct
class Request:
    first: u8
    second: u8
    third: u8


@struct
class Item:
    value: u8
    valid: u1


@struct
class NamedPair:
    left: u8
    right: u4


@struct
class NextPair:
    current: u8
    next: u8


@struct
class Arrays:
    values: table[3, u8]
    narrow: table[3, u4]
    nested: table[6, u8]
    columns: table[6, u2]


@struct
class CombinatorResult:
    mapped_first: u8
    mapped_last: u8
    zipped_left: u8
    zipped_right: u4
    tuple_next: u8
    record_valid: u1
    nested_value: u8
    updated_nested_value: u8
    checked_first: u3
    checked_second: u3
    checked_third: u3


@struct
class Result:
    ready: u1
    valid: u1
    data: CombinatorResult


@rule
def bump(value: u8) -> u8:
    return value + 1


@rule
def decode_checked(value: u8) -> u8:
    return value if value < 5 else 0


@rule
def combine(request: Request) -> CombinatorResult:
    first_narrow = request.first[:4]
    second_narrow = request.second[:4]
    third_narrow = request.third[:4]
    arrays = Arrays(
        values=(
            request.first,
            request.second,
            request.third,
        ),
        narrow=(
            first_narrow,
            second_narrow,
            third_narrow,
        ),
        nested=(
            request.first,
            request.second,
            request.third,
            request.third,
            request.second,
            request.first,
        ),
        columns=(0, 1, 2, 0, 1, 2),
    )

    offset = request.first
    mapped = arrays.values.map(lambda offset: offset + 1)
    helper_mapped = arrays.values.map(bump)
    zipped = arrays.values.map(
        lambda value, narrow: NamedPair(left=value, right=narrow),
        arrays.narrow,
    )
    constant_offset = 3
    tuples = arrays.values.map(
        lambda constant_offset: NextPair(
            current=constant_offset,
            next=constant_offset + 1,
        )
    )
    records = arrays.values.map(lambda value: Item(value=value, valid=value != 0))
    nested = arrays.nested.map(bump)
    updated_nested = arrays.nested.map(
        lambda value, column: 1 if column == 0 else value,
        arrays.columns,
    )
    checked = arrays.values.map(decode_checked)

    return CombinatorResult(
        mapped_first=helper_mapped[0],
        mapped_last=mapped[2],
        zipped_left=zipped[1].left,
        zipped_right=zipped[1].right,
        tuple_next=tuples[2].next,
        record_valid=records[1].valid,
        nested_value=nested[5],
        updated_nested_value=updated_nested[3],
        checked_first=checked[0][:3],
        checked_second=checked[1][:3],
        checked_third=checked[2][:3],
    )


@module
def ArrayCombinators(valid: u1, data: Request, take: u1) -> Result:
    ready, available, request = queue[Request](
        valid,
        data,
        stage_ready,
        depth=1,
        ready_policy="downstream_pop",
    )
    combined = combine(request)
    stage_ready, out_valid, out_data = queue[CombinatorResult](
        available,
        combined,
        take,
        depth=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
