"""Seven independent known-value fixtures under the original 96-cycle stalls."""

# ruff: noqa: N802 -- hardware system name.

from example_bounded_integer_operations.bounded_integer_operations import (
    BoundedIntegerOperations,
    Decoded,
    Ready,
    Request,
)
from pycircuit import log, rule, struct, system, table, u1, u3, u4, u8


@struct
class Expected:
    wrapped: u3
    saturated: u4
    checked_value: u3
    checked_valid: u1
    advanced: u3
    selected: u8
    wrapped_two: u1
    wrapped_eight: u3
    wrapped_full: u8
    narrow_selected: u8
    zero_selected: u8
    checked_window_value: u4
    checked_window_valid: u1
    below_upper: u1
    cross_domain_ordered: u1
    restored: u3
    updated_selected: u8
    updated_first: u8
    updated_last: u8
    source_after_update: u8
    chained_first: u8
    chained_selected: u8
    updated_after_chain: u8


@struct
class Fixtures:
    raw_values: table[7, u8] = (0, 3, 4, 5, 8, 9, 255)
    expected: table[7, Expected] = (
        Expected(
            wrapped=0,
            saturated=4,
            checked_value=0,
            checked_valid=1,
            advanced=1,
            selected=11,
            wrapped_two=0,
            wrapped_eight=0,
            wrapped_full=0,
            narrow_selected=11,
            zero_selected=11,
            checked_window_value=4,
            checked_window_valid=0,
            below_upper=1,
            cross_domain_ordered=1,
            restored=0,
            updated_selected=0,
            updated_first=0,
            updated_last=55,
            source_after_update=11,
            chained_first=99,
            chained_selected=99,
            updated_after_chain=0,
        ),
        Expected(
            wrapped=3,
            saturated=4,
            checked_value=3,
            checked_valid=1,
            advanced=4,
            selected=44,
            wrapped_two=1,
            wrapped_eight=3,
            wrapped_full=3,
            narrow_selected=44,
            zero_selected=11,
            checked_window_value=4,
            checked_window_valid=0,
            below_upper=1,
            cross_domain_ordered=1,
            restored=3,
            updated_selected=3,
            updated_first=11,
            updated_last=55,
            source_after_update=44,
            chained_first=99,
            chained_selected=3,
            updated_after_chain=3,
        ),
        Expected(
            wrapped=4,
            saturated=4,
            checked_value=4,
            checked_valid=1,
            advanced=5,
            selected=55,
            wrapped_two=0,
            wrapped_eight=4,
            wrapped_full=4,
            narrow_selected=11,
            zero_selected=11,
            checked_window_value=4,
            checked_window_valid=1,
            below_upper=1,
            cross_domain_ordered=0,
            restored=4,
            updated_selected=4,
            updated_first=11,
            updated_last=4,
            source_after_update=55,
            chained_first=99,
            chained_selected=4,
            updated_after_chain=4,
        ),
        Expected(
            wrapped=0,
            saturated=5,
            checked_value=0,
            checked_valid=0,
            advanced=1,
            selected=11,
            wrapped_two=1,
            wrapped_eight=5,
            wrapped_full=5,
            narrow_selected=22,
            zero_selected=11,
            checked_window_value=5,
            checked_window_valid=1,
            below_upper=1,
            cross_domain_ordered=1,
            restored=0,
            updated_selected=5,
            updated_first=5,
            updated_last=55,
            source_after_update=11,
            chained_first=99,
            chained_selected=99,
            updated_after_chain=5,
        ),
        Expected(
            wrapped=3,
            saturated=8,
            checked_value=0,
            checked_valid=0,
            advanced=4,
            selected=11,
            wrapped_two=0,
            wrapped_eight=0,
            wrapped_full=8,
            narrow_selected=11,
            zero_selected=11,
            checked_window_value=8,
            checked_window_valid=1,
            below_upper=1,
            cross_domain_ordered=1,
            restored=3,
            updated_selected=8,
            updated_first=8,
            updated_last=55,
            source_after_update=11,
            chained_first=99,
            chained_selected=99,
            updated_after_chain=8,
        ),
        Expected(
            wrapped=4,
            saturated=8,
            checked_value=0,
            checked_valid=0,
            advanced=5,
            selected=11,
            wrapped_two=1,
            wrapped_eight=1,
            wrapped_full=9,
            narrow_selected=22,
            zero_selected=11,
            checked_window_value=4,
            checked_window_valid=0,
            below_upper=1,
            cross_domain_ordered=1,
            restored=4,
            updated_selected=9,
            updated_first=9,
            updated_last=55,
            source_after_update=11,
            chained_first=99,
            chained_selected=99,
            updated_after_chain=9,
        ),
        Expected(
            wrapped=0,
            saturated=8,
            checked_value=0,
            checked_valid=0,
            advanced=1,
            selected=11,
            wrapped_two=1,
            wrapped_eight=7,
            wrapped_full=255,
            narrow_selected=44,
            zero_selected=11,
            checked_window_value=4,
            checked_window_valid=0,
            below_upper=1,
            cross_domain_ordered=1,
            restored=0,
            updated_selected=255,
            updated_first=255,
            updated_last=55,
            source_after_update=11,
            chained_first=99,
            chained_selected=99,
            updated_after_chain=255,
        ),
    )


@rule
def select_request(request_index: u4, fixtures: Fixtures) -> Request:
    return Request(
        values=(11, 22, 33, 44, 55),
        raw_index=fixtures.raw_values[request_index % 7],
    )


@rule
def common_take(phase: u8) -> Ready:
    take = (phase % 7 != 2) & (phase % 7 != 3)
    return Ready(
        wrapped=take,
        saturated=take,
        checked_value=take,
        checked_valid=take,
        advanced=take,
        selected=take,
        wrapped_two=take,
        wrapped_eight=take,
        wrapped_full=take,
        narrow_selected=take,
        zero_selected=take,
        checked_window_value=take,
        checked_window_valid=take,
        below_upper=take,
        cross_domain_ordered=take,
        restored=take,
        updated_selected=take,
        updated_first=take,
        updated_last=take,
        source_after_update=take,
        chained_first=take,
        chained_selected=take,
        updated_after_chain=take,
    )


@rule
def observe(
    phase: u8,
    request_index: u4,
    accepted_count: u4,
    output_count: u4,
    ready: u1,
    valid: Ready,
    data: Decoded,
    take: Ready,
    expected: Expected,
):
    if request_index < 7 and ready:
        request_index = request_index + 1
        accepted_count = accepted_count + 1

    assert (
        valid.saturated == valid.wrapped
    ), "bounded_integer_operations: saturated valid"
    assert (
        valid.checked_value == valid.wrapped
    ), "bounded_integer_operations: checked_value valid"
    assert (
        valid.checked_valid == valid.wrapped
    ), "bounded_integer_operations: checked_valid valid"
    assert valid.advanced == valid.wrapped, "bounded_integer_operations: advanced valid"
    assert valid.selected == valid.wrapped, "bounded_integer_operations: selected valid"
    assert (
        valid.wrapped_two == valid.wrapped
    ), "bounded_integer_operations: wrapped_two valid"
    assert (
        valid.wrapped_eight == valid.wrapped
    ), "bounded_integer_operations: wrapped_eight valid"
    assert (
        valid.wrapped_full == valid.wrapped
    ), "bounded_integer_operations: wrapped_full valid"
    assert (
        valid.narrow_selected == valid.wrapped
    ), "bounded_integer_operations: narrow_selected valid"
    assert (
        valid.zero_selected == valid.wrapped
    ), "bounded_integer_operations: zero_selected valid"
    assert (
        valid.checked_window_value == valid.wrapped
    ), "bounded_integer_operations: checked_window_value valid"
    assert (
        valid.checked_window_valid == valid.wrapped
    ), "bounded_integer_operations: checked_window_valid valid"
    assert (
        valid.below_upper == valid.wrapped
    ), "bounded_integer_operations: below_upper valid"
    assert (
        valid.cross_domain_ordered == valid.wrapped
    ), "bounded_integer_operations: cross_domain_ordered valid"
    assert valid.restored == valid.wrapped, "bounded_integer_operations: restored valid"
    assert (
        valid.updated_selected == valid.wrapped
    ), "bounded_integer_operations: updated_selected valid"
    assert (
        valid.updated_first == valid.wrapped
    ), "bounded_integer_operations: updated_first valid"
    assert (
        valid.updated_last == valid.wrapped
    ), "bounded_integer_operations: updated_last valid"
    assert (
        valid.source_after_update == valid.wrapped
    ), "bounded_integer_operations: source_after_update valid"
    assert (
        valid.chained_first == valid.wrapped
    ), "bounded_integer_operations: chained_first valid"
    assert (
        valid.chained_selected == valid.wrapped
    ), "bounded_integer_operations: chained_selected valid"
    assert (
        valid.updated_after_chain == valid.wrapped
    ), "bounded_integer_operations: updated_after_chain valid"

    if valid.wrapped:
        assert output_count < 7, "bounded_integer_operations: no duplicate output"
        assert data.wrapped == expected.wrapped, "bounded_integer_operations: wrapped"
        assert (
            data.saturated == expected.saturated
        ), "bounded_integer_operations: saturated"
        assert (
            data.checked_value == expected.checked_value
        ), "bounded_integer_operations: checked_value"
        assert (
            data.checked_valid == expected.checked_valid
        ), "bounded_integer_operations: checked_valid"
        assert (
            data.advanced == expected.advanced
        ), "bounded_integer_operations: advanced"
        assert (
            data.selected == expected.selected
        ), "bounded_integer_operations: selected"
        assert (
            data.wrapped_two == expected.wrapped_two
        ), "bounded_integer_operations: wrapped_two"
        assert (
            data.wrapped_eight == expected.wrapped_eight
        ), "bounded_integer_operations: wrapped_eight"
        assert (
            data.wrapped_full == expected.wrapped_full
        ), "bounded_integer_operations: wrapped_full"
        assert (
            data.narrow_selected == expected.narrow_selected
        ), "bounded_integer_operations: narrow_selected"
        assert (
            data.zero_selected == expected.zero_selected
        ), "bounded_integer_operations: zero_selected"
        assert (
            data.checked_window_value == expected.checked_window_value
        ), "bounded_integer_operations: checked_window_value"
        assert (
            data.checked_window_valid == expected.checked_window_valid
        ), "bounded_integer_operations: checked_window_valid"
        assert (
            data.below_upper == expected.below_upper
        ), "bounded_integer_operations: below_upper"
        assert (
            data.cross_domain_ordered == expected.cross_domain_ordered
        ), "bounded_integer_operations: cross_domain_ordered"
        assert (
            data.restored == expected.restored
        ), "bounded_integer_operations: restored"
        assert (
            data.updated_selected == expected.updated_selected
        ), "bounded_integer_operations: updated_selected"
        assert (
            data.updated_first == expected.updated_first
        ), "bounded_integer_operations: updated_first"
        assert (
            data.updated_last == expected.updated_last
        ), "bounded_integer_operations: updated_last"
        assert (
            data.source_after_update == expected.source_after_update
        ), "bounded_integer_operations: source_after_update"
        assert (
            data.chained_first == expected.chained_first
        ), "bounded_integer_operations: chained_first"
        assert (
            data.chained_selected == expected.chained_selected
        ), "bounded_integer_operations: chained_selected"
        assert (
            data.updated_after_chain == expected.updated_after_chain
        ), "bounded_integer_operations: updated_after_chain"
        if take.wrapped:
            output_count = output_count + 1

    log("info", "bounded_integer_operations.output_valid", valid.wrapped)
    log("info", "bounded_integer_operations.output_count", output_count)
    if phase == 95:
        assert accepted_count == 7, "bounded_integer_operations: all requests accepted"
        assert output_count == 7, "bounded_integer_operations: all results observed"
        assert not valid.wrapped, "bounded_integer_operations: pipeline drained"
    if phase < 96:
        phase = phase + 1


@system
def BoundedIntegerOperationsSystem():
    phase: u8 = 0
    request_index: u4 = 0
    accepted_count: u4 = 0
    output_count: u4 = 0
    fixtures = Fixtures()
    request = select_request(request_index, fixtures)
    expected = fixtures.expected[output_count % 7]
    take = common_take(phase)
    dut = BoundedIntegerOperations(request_index < 7, request, take)
    observe(
        phase,
        request_index,
        accepted_count,
        output_count,
        dut.ready,
        dut.valid,
        dut.data,
        take,
        expected,
    )
