from enum import Enum

import agentic_circuit as ac


class Mode(Enum):
    IDLE = 0
    RUN = 1


@ac.struct
class Inner:
    tag: ac.u8
    mode: Mode


@ac.struct
class Payload:
    inner: Inner
    pair: tuple[ac.u8, ac.u8]
    lanes: ac.array[8, ac.u8]
    valid: bool


@ac.struct
class ComparisonRequest:
    left: Payload
    right: Payload
    equal: bool


@ac.invariant
def valid_payload(value: Payload) -> bool:
    return (
        (value.inner.mode == Mode.RUN)
        and (value.pair[0] == value.inner.tag)
        and (value.lanes[0] < 8)
    )


@ac.rule
def compare(request: ComparisonRequest) -> ComparisonRequest:
    return request.with_fields(
        equal=(request.left == request.right) and not (request.left != request.right)
    )


@ac.rule
def validate(checked) -> Payload:
    if valid_payload(checked):
        result = checked
    else:
        return
    return result


@ac.system
def aggregate_equality_invariant(
    request: ComparisonRequest, checked: Payload
) -> tuple[ComparisonRequest, Payload]:
    compared = compare(request)
    validated = validate(checked)
    return compared, validated
