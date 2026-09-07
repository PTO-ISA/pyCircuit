from enum import Enum

import agentic_circuit as ac


class Mode(Enum):
    IDLE = 0
    RUN = 1


@ac.struct
class PackedPayload:
    tag: ac.u4
    mode: Mode
    pair: tuple[ac.u2, ac.u2]
    lanes: ac.array[2, ac.u2]
    valid: bool


@ac.struct
class PackedRequest:
    left: PackedPayload
    right: PackedPayload
    equal: bool


@ac.invariant
def valid_packed(value: PackedPayload) -> bool:
    return (
        (value.mode == Mode.RUN)
        and (value.pair[0] == value.tag[0:2])
        and (value.lanes[0] < 3)
    )


@ac.rule
def evaluate(request: PackedRequest) -> PackedRequest:
    return request.with_fields(
        equal=(request.left == request.right) and not (request.left != request.right)
    )


@ac.rule
def validate(checked: PackedPayload) -> PackedPayload:
    return checked.with_fields(valid=valid_packed(checked))


@ac.system
def aggregate_equality_packed(
    request: PackedRequest, checked: PackedPayload
) -> tuple[PackedRequest, PackedPayload]:
    verdict = evaluate(request)
    validated = validate(checked)
    return verdict, validated
