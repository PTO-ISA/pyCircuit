"""A fixed-width sparse protocol enum preserved through every backend."""

from enum import Enum

import agentic_circuit as ac


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
    matched: bool


@ac.rule
def classify(command: Command) -> Command:
    return command.with_fields(
        selected=Opcode.WRITE,
        wide=WideOpcode.MAX,
        matched=command.opcode == Opcode.READ,
    )


@ac.system
def encoded_enum_pipeline(command: Command) -> Command:
    result = classify(command)
    return result


specialization = ac.jit(encoded_enum_pipeline)
