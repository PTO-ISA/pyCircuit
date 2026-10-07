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


@ac.rule
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
    ready, available, command = ac.queue[Command](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    classified = classify(command)
    stage_ready, out_valid, out_data = ac.queue[Command](
        available, classified, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
