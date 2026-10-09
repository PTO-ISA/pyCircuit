"""Closed three-request oracle for the array-combinator queue pipeline."""

from example_array_combinators.array_combinators import (
    ArrayCombinators,
    CombinatorResult,
    Request,
)
from pycircuit import log, rule, struct, system, table, u1, u2, u3, u4, u8


@struct
class Stimulus:
    first: u8
    second: u8
    third: u8


@struct
class Expected:
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
class Fixtures:
    requests: table[3, Stimulus] = (
        Stimulus(first=0, second=1, third=4),
        Stimulus(first=3, second=5, third=7),
        Stimulus(first=255, second=2, third=254),
    )
    expected: table[3, Expected] = (
        Expected(
            mapped_first=1,
            mapped_last=5,
            zipped_left=1,
            zipped_right=1,
            tuple_next=5,
            record_valid=1,
            nested_value=1,
            updated_nested_value=1,
            checked_first=0,
            checked_second=1,
            checked_third=4,
        ),
        Expected(
            mapped_first=4,
            mapped_last=8,
            zipped_left=5,
            zipped_right=5,
            tuple_next=8,
            record_valid=1,
            nested_value=4,
            updated_nested_value=1,
            checked_first=3,
            checked_second=0,
            checked_third=0,
        ),
        Expected(
            mapped_first=0,
            mapped_last=255,
            zipped_left=2,
            zipped_right=2,
            tuple_next=255,
            record_valid=1,
            nested_value=0,
            updated_nested_value=1,
            checked_first=0,
            checked_second=2,
            checked_third=0,
        ),
    )


@rule
def select_request(request_index: u2, fixtures: Fixtures) -> Request:
    selected = fixtures.requests[request_index % 3]
    return Request(
        first=selected.first,
        second=selected.second,
        third=selected.third,
    )


@rule
def observe(
    phase: u8,
    request_index: u2,
    accepted_count: u2,
    output_count: u2,
    ready: u1,
    valid: u1,
    data: CombinatorResult,
    expected: Expected,
):
    if request_index < 3 and ready:
        request_index = request_index + 1
        accepted_count = accepted_count + 1

    if valid:
        assert (
            data.mapped_first == expected.mapped_first
        ), "array_combinators: mapped_first"
        assert (
            data.mapped_last == expected.mapped_last
        ), "array_combinators: mapped_last"
        assert (
            data.zipped_left == expected.zipped_left
        ), "array_combinators: zipped_left"
        assert (
            data.zipped_right == expected.zipped_right
        ), "array_combinators: zipped_right"
        assert data.tuple_next == expected.tuple_next, "array_combinators: tuple_next"
        assert (
            data.record_valid == expected.record_valid
        ), "array_combinators: record_valid"
        assert (
            data.nested_value == expected.nested_value
        ), "array_combinators: nested_value"
        assert (
            data.updated_nested_value == expected.updated_nested_value
        ), "array_combinators: updated_nested_value"
        assert (
            data.checked_first == expected.checked_first
        ), "array_combinators: checked_first"
        assert (
            data.checked_second == expected.checked_second
        ), "array_combinators: checked_second"
        assert (
            data.checked_third == expected.checked_third
        ), "array_combinators: checked_third"
        assert output_count < 3, "array_combinators: no duplicate output"
        output_count = output_count + 1

    log("info", "array_combinators.output_valid", valid)
    log("info", "array_combinators.output_count", output_count)
    if phase == 5:
        assert accepted_count == 3, "array_combinators: all requests accepted"
        assert output_count == 3, "array_combinators: all results observed"
        assert not valid, "array_combinators: pipeline drained"
    if phase < 6:
        phase = phase + 1


@system
def ArrayCombinatorsSystem():  # noqa: N802
    phase: u8 = 0
    request_index: u2 = 0
    accepted_count: u2 = 0
    output_count: u2 = 0
    fixtures = Fixtures()
    request = select_request(request_index, fixtures)
    expected = fixtures.expected[output_count % 3]
    dut = ArrayCombinators(request_index < 3, request, 1)
    observe(
        phase,
        request_index,
        accepted_count,
        output_count,
        dut.ready,
        dut.valid,
        dut.data,
        expected,
    )
