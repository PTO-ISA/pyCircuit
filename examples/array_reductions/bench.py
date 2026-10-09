"""Closed five-request oracle for the array-reduction queue pipeline."""

from example_array_reductions.array_reductions import (
    ArrayReductions,
    ReductionResult,
    Request,
)
from pycircuit import log, rule, struct, system, table, u1, u2, u3, u8


@struct
class Stimulus:
    first: u8
    second: u8
    third: u8


@struct
class Expected:
    complete: u1
    present: u1
    count: u2
    helper_count: u2
    total: u8
    product: u8
    minimum: u8
    maximum: u8
    parity: u1
    first_index: u2
    first_valid: u1
    best_index: u2
    best_valid: u1
    range_best_index: u2


@struct
class Fixtures:
    requests: table[5, Stimulus] = (
        Stimulus(first=0, second=0, third=0),
        Stimulus(first=0, second=0, third=5),
        Stimulus(first=7, second=7, third=9),
        Stimulus(first=3, second=1, third=2),
        Stimulus(first=255, second=2, third=3),
    )
    expected: table[5, Expected] = (
        Expected(),
        Expected(
            present=1,
            count=1,
            helper_count=1,
            total=5,
            maximum=5,
            parity=1,
            first_index=2,
            first_valid=1,
            best_index=2,
            best_valid=1,
        ),
        Expected(
            complete=1,
            present=1,
            count=3,
            helper_count=3,
            total=23,
            product=185,
            minimum=7,
            maximum=9,
            parity=1,
            first_valid=1,
            best_valid=1,
        ),
        Expected(
            complete=1,
            present=1,
            count=3,
            helper_count=3,
            total=6,
            product=6,
            minimum=1,
            maximum=3,
            parity=1,
            first_valid=1,
            best_index=1,
            best_valid=1,
            range_best_index=1,
        ),
        Expected(
            complete=1,
            present=1,
            count=3,
            helper_count=3,
            total=4,
            product=250,
            minimum=2,
            maximum=255,
            parity=1,
            first_valid=1,
            best_index=1,
            best_valid=1,
        ),
    )


@rule
def select_request(request_index: u3, fixtures: Fixtures) -> Request:
    selected = fixtures.requests[request_index % 5]
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
    data: ReductionResult,
    expected: Expected,
):
    if request_index < 5 and ready:
        request_index = request_index + 1
        accepted_count = accepted_count + 1

    if valid:
        assert data.complete == expected.complete, "array_reductions: complete"
        assert data.present == expected.present, "array_reductions: present"
        assert data.count == expected.count, "array_reductions: count"
        assert (
            data.helper_count == expected.helper_count
        ), "array_reductions: helper_count"
        assert data.total == expected.total, "array_reductions: total"
        assert data.product == expected.product, "array_reductions: product"
        assert data.minimum == expected.minimum, "array_reductions: minimum"
        assert data.maximum == expected.maximum, "array_reductions: maximum"
        assert data.parity == expected.parity, "array_reductions: parity"
        assert data.first_index == expected.first_index, "array_reductions: first_index"
        assert data.first_valid == expected.first_valid, "array_reductions: first_valid"
        assert data.best_index == expected.best_index, "array_reductions: best_index"
        assert data.best_valid == expected.best_valid, "array_reductions: best_valid"
        assert (
            data.range_best_index == expected.range_best_index
        ), "array_reductions: range_best_index"

    log("info", "array_reductions.output_valid", valid)
    log("info", "array_reductions.complete", data.complete)
    log("info", "array_reductions.present", data.present)
    log("info", "array_reductions.count", data.count)
    log("info", "array_reductions.helper_count", data.helper_count)
    log("info", "array_reductions.total", data.total)
    log("info", "array_reductions.product", data.product)
    log("info", "array_reductions.minimum", data.minimum)
    log("info", "array_reductions.maximum", data.maximum)
    log("info", "array_reductions.parity", data.parity)
    log("info", "array_reductions.first_index", data.first_index)
    log("info", "array_reductions.first_valid", data.first_valid)
    log("info", "array_reductions.best_index", data.best_index)
    log("info", "array_reductions.best_valid", data.best_valid)
    log("info", "array_reductions.range_best_index", data.range_best_index)

    if valid:
        assert output_count < 5, "array_reductions: no duplicate output"
        output_count = output_count + 1
    if phase == 7:
        assert accepted_count == 5, "array_reductions: all requests accepted"
        assert output_count == 5, "array_reductions: all results observed"
        assert not valid, "array_reductions: pipeline drained"
    log("info", "array_reductions.accepted_count", accepted_count)
    if phase < 8:
        phase = phase + 1


@system
def ArrayReductionsSystem():  # noqa: N802
    phase: u8 = 0
    request_index: u3 = 0
    accepted_count: u3 = 0
    output_count: u3 = 0
    fixtures = Fixtures()
    request = select_request(request_index, fixtures)
    expected = fixtures.expected[output_count % 5]
    dut = ArrayReductions(request_index < 5, request, 1)
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
