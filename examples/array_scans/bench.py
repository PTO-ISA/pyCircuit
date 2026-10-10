"""Four literal historical scan records in a finite closed system."""

from example_array_scans.array_scans import ArrayScans, Request, ScanResult
from pycircuit import log, rule, struct, system, table, u1, u3, u8


@struct
class Stimulus:
    first: u8
    second: u8
    third: u8


@struct
class Expected:
    subtract_first: u8
    subtract_second: u8
    subtract_third: u8
    nonzero_last: u8
    reset_second: u8
    reset_third: u8
    tuple_first: u8
    tuple_third: u8


@struct
class Fixtures:
    requests: table[4, Stimulus] = (
        Stimulus(first=0, second=0, third=0),
        Stimulus(first=1, second=2, third=3),
        Stimulus(first=255, second=1, third=2),
        Stimulus(first=7, second=9, third=11),
    )
    expected: table[4, Expected] = (
        Expected(
            subtract_first=0,
            subtract_second=0,
            subtract_third=0,
            nonzero_last=10,
            reset_second=0,
            reset_third=0,
            tuple_first=0,
            tuple_third=0,
        ),
        Expected(
            subtract_first=255,
            subtract_second=253,
            subtract_third=250,
            nonzero_last=16,
            reset_second=8,
            reset_third=11,
            tuple_first=2,
            tuple_third=7,
        ),
        Expected(
            subtract_first=1,
            subtract_second=0,
            subtract_third=254,
            nonzero_last=12,
            reset_second=5,
            reset_third=7,
            tuple_first=254,
            tuple_third=1,
        ),
        Expected(
            subtract_first=249,
            subtract_second=240,
            subtract_third=229,
            nonzero_last=37,
            reset_second=21,
            reset_third=32,
            tuple_first=14,
            tuple_third=34,
        ),
    )


@rule
def select_request(request_index: u3, fixtures: Fixtures) -> Request:
    selected = fixtures.requests[request_index % 4]
    return Request(
        first=selected.first,
        second=selected.second,
        third=selected.third,
    )


@rule
def observe(
    phase: u8,
    request_index: u3,
    accepted_count: u3,
    output_count: u3,
    ready: u1,
    valid: u1,
    data: ScanResult,
    expected: Expected,
):
    if request_index < 4 and ready:
        request_index = request_index + 1
        accepted_count = accepted_count + 1

    if valid:
        assert output_count < 4, "array_scans: no duplicate output"
        assert (
            data.subtract_first == expected.subtract_first
        ), "array_scans: subtract_first"
        assert (
            data.subtract_second == expected.subtract_second
        ), "array_scans: subtract_second"
        assert (
            data.subtract_third == expected.subtract_third
        ), "array_scans: subtract_third"
        assert data.nonzero_last == expected.nonzero_last, "array_scans: nonzero_last"
        assert data.reset_second == expected.reset_second, "array_scans: reset_second"
        assert data.reset_third == expected.reset_third, "array_scans: reset_third"
        assert data.tuple_first == expected.tuple_first, "array_scans: tuple_first"
        assert data.tuple_third == expected.tuple_third, "array_scans: tuple_third"
        output_count = output_count + 1

    log("info", "array_scans.output_valid", valid)
    log("info", "array_scans.output_count", output_count)
    if phase == 15:
        assert accepted_count == 4, "array_scans: all requests accepted"
        assert output_count == 4, "array_scans: all results observed"
        assert not valid, "array_scans: pipeline drained"
    if phase < 16:
        phase = phase + 1


@system
def ArrayScansSystem():  # noqa: N802
    phase: u8 = 0
    request_index: u3 = 0
    accepted_count: u3 = 0
    output_count: u3 = 0
    fixtures = Fixtures()
    request = select_request(request_index, fixtures)
    expected = fixtures.expected[output_count % 4]
    dut = ArrayScans(request_index < 4, request, 1)
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
