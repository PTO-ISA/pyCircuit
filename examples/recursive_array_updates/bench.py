"""Four literal historical result records in a finite closed system."""

from example_recursive_array_updates.recursive_array_updates import (
    RecursiveArrayResult,
    RecursiveArrayUpdates,
    Request,
)
from pycircuit import enum_to_bits, log, rule, struct, system, table, u1, u2, u3, u8


@struct
class Stimulus:
    raw_index: u8
    replacement: u8


@struct
class Expected:
    source_struct_tag: u8
    updated_struct_tag: u8
    source_struct_mode: u1
    updated_struct_mode: u1
    source_enum: u1
    updated_enum: u1
    source_range: u2
    updated_range: u2
    source_wide: u3
    updated_wide: u3
    wide_first: u3
    wide_last: u3
    chained_selected: u3
    updated_after_chain: u3


@struct
class Fixtures:
    requests: table[4, Stimulus] = (
        Stimulus(raw_index=0, replacement=9),
        Stimulus(raw_index=2, replacement=14),
        Stimulus(raw_index=64, replacement=5),
        Stimulus(raw_index=255, replacement=131),
    )
    expected: table[4, Expected] = (
        Expected(
            source_struct_tag=0,
            updated_struct_tag=9,
            source_struct_mode=0,
            updated_struct_mode=1,
            source_enum=0,
            updated_enum=1,
            source_range=0,
            updated_range=0,
            source_wide=0,
            updated_wide=1,
            wide_first=1,
            wide_last=0,
            chained_selected=7,
            updated_after_chain=1,
        ),
        Expected(
            source_struct_tag=2,
            updated_struct_tag=14,
            source_struct_mode=0,
            updated_struct_mode=1,
            source_enum=0,
            updated_enum=1,
            source_range=2,
            updated_range=2,
            source_wide=2,
            updated_wide=6,
            wide_first=2,
            wide_last=2,
            chained_selected=6,
            updated_after_chain=6,
        ),
        Expected(
            source_struct_tag=64,
            updated_struct_tag=5,
            source_struct_mode=0,
            updated_struct_mode=1,
            source_enum=0,
            updated_enum=1,
            source_range=1,
            updated_range=2,
            source_wide=0,
            updated_wide=5,
            wide_first=0,
            wide_last=5,
            chained_selected=5,
            updated_after_chain=5,
        ),
        Expected(
            source_struct_tag=255,
            updated_struct_tag=131,
            source_struct_mode=0,
            updated_struct_mode=1,
            source_enum=0,
            updated_enum=1,
            source_range=0,
            updated_range=2,
            source_wide=7,
            updated_wide=3,
            wide_first=7,
            wide_last=7,
            chained_selected=3,
            updated_after_chain=3,
        ),
    )


@rule
def select_request(request_index: u3, fixtures: Fixtures) -> Request:
    selected = fixtures.requests[request_index % 4]
    return Request(raw_index=selected.raw_index, replacement=selected.replacement)


@rule
def observe(
    phase: u8,
    request_index: u3,
    accepted_count: u3,
    output_count: u3,
    ready: u1,
    valid: u1,
    data: RecursiveArrayResult,
    expected: Expected,
):
    if request_index < 4 and ready:
        request_index = request_index + 1
        accepted_count = accepted_count + 1

    if valid:
        assert output_count < 4, "recursive_array_updates: no duplicate output"
        assert (
            data.source_struct_tag == expected.source_struct_tag
        ), "recursive_array_updates: source_struct_tag"
        assert (
            data.updated_struct_tag == expected.updated_struct_tag
        ), "recursive_array_updates: updated_struct_tag"
        assert (
            enum_to_bits(data.source_struct_mode) == expected.source_struct_mode
        ), "recursive_array_updates: source_struct_mode"
        assert (
            enum_to_bits(data.updated_struct_mode) == expected.updated_struct_mode
        ), "recursive_array_updates: updated_struct_mode"
        assert (
            enum_to_bits(data.source_enum) == expected.source_enum
        ), "recursive_array_updates: source_enum"
        assert (
            enum_to_bits(data.updated_enum) == expected.updated_enum
        ), "recursive_array_updates: updated_enum"
        assert (
            data.source_range == expected.source_range
        ), "recursive_array_updates: source_range"
        assert (
            data.updated_range == expected.updated_range
        ), "recursive_array_updates: updated_range"
        assert (
            data.source_wide == expected.source_wide
        ), "recursive_array_updates: source_wide"
        assert (
            data.updated_wide == expected.updated_wide
        ), "recursive_array_updates: updated_wide"
        assert (
            data.wide_first == expected.wide_first
        ), "recursive_array_updates: wide_first"
        assert (
            data.wide_last == expected.wide_last
        ), "recursive_array_updates: wide_last"
        assert (
            data.chained_selected == expected.chained_selected
        ), "recursive_array_updates: chained_selected"
        assert (
            data.updated_after_chain == expected.updated_after_chain
        ), "recursive_array_updates: updated_after_chain"
        output_count = output_count + 1

    log("info", "recursive_array_updates.output_valid", valid)
    log("info", "recursive_array_updates.output_count", output_count)
    if phase == 15:
        assert accepted_count == 4, "recursive_array_updates: all requests accepted"
        assert output_count == 4, "recursive_array_updates: all results observed"
        assert not valid, "recursive_array_updates: pipeline drained"
    if phase < 16:
        phase = phase + 1


@system
def RecursiveArrayUpdatesSystem():  # noqa: N802
    phase: u8 = 0
    request_index: u3 = 0
    accepted_count: u3 = 0
    output_count: u3 = 0
    fixtures = Fixtures()
    request = select_request(request_index, fixtures)
    expected = fixtures.expected[output_count % 4]
    dut = RecursiveArrayUpdates(request_index < 4, request, 1)
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
