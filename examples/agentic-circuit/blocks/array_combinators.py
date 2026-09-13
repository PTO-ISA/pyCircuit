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
