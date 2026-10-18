"""Carry sparse and full-width nominal enums through two token stages."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=4)
class Opcode(Enum):
    NONE = 0
    READ = 3
    WRITE = 9


@ac.encoding(width=64)
class WideOpcode(Enum):
    LOW = 9223372036854775807
    HIGH = 9223372036854775808
    MAX = 18446744073709551615


@ac.struct
class Command:
    opcode: Opcode
    selected: Opcode
    wide: WideOpcode
    matched: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Command


def classify(command: Command) -> Command:
    result = command
    result.selected = Opcode.WRITE
    result.wide = WideOpcode.MAX
    result.matched = command.opcode == Opcode.READ
    return result


@ac.module
def EncodedEnumPipeline(  # noqa: N802
    valid: ac.u1, data: Command, take: ac.u1
) -> Result:
    ingress = ac.queue[Command](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Command](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        command = ingress.pop()
        classified = classify(command)
        egress.push(classified)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
