"""Pythonic fixed-array map and zip with exact recursive result types."""

import agentic_circuit as ac


@ac.struct
class Request:
    first: ac.u8
    second: ac.u8
    third: ac.u8


@ac.struct
class Item:
    value: ac.u8
    valid: bool


@ac.struct
class Arrays:
    values: ac.array[3, ac.u8]
    narrow: ac.array[3, ac.u4]
    nested: ac.array[2, ac.array[3, ac.u8]]


@ac.struct
class Result:
    mapped_first: ac.u8
    mapped_last: ac.u8
    zipped_left: ac.u8
    zipped_right: ac.u4
    tuple_next: ac.u8
    record_valid: bool
    nested_value: ac.u8
    updated_nested_value: ac.u8
    checked_first: ac.index[5]
    checked_second: ac.index[5]
    checked_third: ac.index[5]


def bump(value: ac.u8) -> ac.u8:
    return value + 1


def replace_first(values: ac.array[3, ac.u8]) -> ac.array[3, ac.u8]:
    return values.with_element(0, 1)


def decode_checked(value: ac.u8) -> ac.index[5]:
    return ac.checked(value, ac.index[5]).value


@ac.rule
def combine(request: Request) -> Result:
    first_narrow = ac.truncate(request.first, ac.u4)
    second_narrow = ac.truncate(request.second, ac.u4)
    third_narrow = ac.truncate(request.third, ac.u4)
    arrays = Arrays(
        values=(request.first, request.second, request.third),
        narrow=(first_narrow, second_narrow, third_narrow),
        nested=(
            (request.first, request.second, request.third),
            (request.third, request.second, request.first),
        ),
    )
    offset = request.first
    mapped = arrays.values.map(lambda offset: offset + 1)
    helper_mapped = arrays.values.map(bump)
    zipped = arrays.values.zip(arrays.narrow)
    constant_offset = 3
    tuples = arrays.values.map(
        lambda constant_offset: (constant_offset, constant_offset + 1)
    )
    records = arrays.values.map(lambda value: Item(value=value, valid=value != 0))
    nested = arrays.nested.map(lambda lane: lane.map(lambda value: value + 1))
    updated_nested = arrays.nested.map(replace_first)
    checked = arrays.values.map(decode_checked)
    return Result(
        mapped_first=helper_mapped[0],
        mapped_last=mapped[2],
        zipped_left=zipped[1][0],
        zipped_right=zipped[1][1],
        tuple_next=tuples[2][1],
        record_valid=records[1].valid,
        nested_value=nested[1][2],
        updated_nested_value=updated_nested[1][0],
        checked_first=checked[0],
        checked_second=checked[1],
        checked_third=checked[2],
    )


@ac.system
def array_combinators(request: Request) -> Result:
    result = combine(request)
    return result


@ac.struct
class Ranked:
    age: ac.u8
    valid: bool


@ac.struct
class ReductionResult:
    complete: bool
    present: bool
    count: ac.range[0, 4]
    helper_count: ac.range[0, 4]
    total: ac.u8
    product: ac.u8
    minimum: ac.u8
    maximum: ac.u8
    parity: bool
    first_index: ac.index[3]
    first_valid: bool
    best_index: ac.index[3]
    best_valid: bool
    range_best_index: ac.index[3]


def count_flags(flags: ac.array[3, bool]) -> ac.range[0, 4]:
    return flags.count()


@ac.rule
def reduce_arrays(request: Request) -> ReductionResult:
    values = Arrays(
        values=(request.first, request.second, request.third),
        narrow=(
            ac.truncate(request.first, ac.u4),
            ac.truncate(request.second, ac.u4),
            ac.truncate(request.third, ac.u4),
        ),
        nested=(
            (request.first, request.second, request.third),
            (request.third, request.second, request.first),
        ),
    ).values
    flags = values.map(lambda value: value != 0)
    ranked = values.zip(flags).map(lambda pair: Ranked(age=pair[0], valid=pair[1]))
    first = ranked.first(where=lambda item: item.valid)
    best = ranked.argmin(
        key=lambda item: item.age,
        where=lambda item: item.valid,
    )
    bounded = values.map(lambda value: ac.wrap(value, ac.index[5]))
    range_best = bounded.argmin(key=lambda value: value)
    return ReductionResult(
        complete=flags.all(),
        present=flags.any(),
        count=flags.count(),
        helper_count=count_flags(flags),
        total=values.fold(kind="add"),
        product=values.fold(kind="mul"),
        minimum=values.fold(kind="min"),
        maximum=values.fold(kind="max"),
        parity=flags.fold(kind="xor"),
        first_index=first.index,
        first_valid=first.valid,
        best_index=best.index,
        best_valid=best.valid,
        range_best_index=range_best.index,
    )


@ac.system
def array_reductions(request: Request) -> ReductionResult:
    result = reduce_arrays(request)
    return result


@ac.struct
class ScanResult:
    subtract_first: ac.u8
    subtract_second: ac.u8
    subtract_third: ac.u8
    nonzero_last: ac.u8
    reset_second: ac.u8
    reset_third: ac.u8
    tuple_first: ac.u8
    tuple_third: ac.u8


def subtract(accumulator: ac.u8, value: ac.u8) -> ac.u8:
    return accumulator - value


@ac.rule
def scan_arrays(request: Request) -> ScanResult:
    values = Arrays(
        values=(request.first, request.second, request.third),
        narrow=(
            ac.truncate(request.first, ac.u4),
            ac.truncate(request.second, ac.u4),
            ac.truncate(request.third, ac.u4),
        ),
        nested=(
            (request.first, request.second, request.third),
            (request.third, request.second, request.first),
        ),
    ).values
    subtracted = values.scan(subtract, initial=ac.zero(ac.u8))
    nonzero = values.scan(
        lambda accumulator, value: accumulator + value,
        initial=ac.literal(10, ac.u8),
    )
    reset = values.scan(
        lambda accumulator, value: (0 if value == 0 else accumulator + value),
        initial=ac.literal(5, ac.u8),
    )
    tupled = values.scan(
        lambda accumulator, value: (
            0 if value == 0 else accumulator[0] + value,
            accumulator[1],
        ),
        initial=values.zip(values)[0],
    )
    return ScanResult(
        subtract_first=subtracted[0],
        subtract_second=subtracted[1],
        subtract_third=subtracted[2],
        nonzero_last=nonzero[2],
        reset_second=reset[1],
        reset_third=reset[2],
        tuple_first=tupled[0][0],
        tuple_third=tupled[2][0],
    )


@ac.system
def array_scans(request: Request) -> ScanResult:
    result = scan_arrays(request)
    return result
