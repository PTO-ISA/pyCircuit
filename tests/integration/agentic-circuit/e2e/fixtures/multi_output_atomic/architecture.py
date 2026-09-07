import agentic_circuit as ac


@ac.struct
class Command:
    value: ac.u8
    emit_left: bool
    emit_right: bool
    cancel_left: bool


@ac.struct
class LeftEffect:
    value: ac.u8


@ac.struct
class RightEffect:
    value: ac.u7


@ac.struct
class ApplyAck:
    value: ac.u8


@ac.rule
def dispatch(count, entries, command) -> tuple[LeftEffect, RightEffect, ApplyAck]:
    count = count + 1
    entries[command.value[0]] = command.value
    left = None
    right = None
    ack = ApplyAck(value=command.value)
    if command.emit_left:
        left = LeftEffect(value=command.value)
    if command.emit_right:
        right = RightEffect(value=command.value[0:7])
    if command.cancel_left:
        left = None
    return left, right, ack


@ac.system
def multi_output_atomic(
    command: Command,
) -> tuple[LeftEffect, RightEffect, ApplyAck]:
    count: ac.u8 = 0
    entries: list[ac.u8] = [0] * 2
    left, right, ack = dispatch(count, entries, command)
    return left, right, ack
