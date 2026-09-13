"""Pythonic enum classification and explicit protocol decoding policies."""

from enum import Enum

import agentic_circuit as ac


@ac.encoding(width=4)
class Opcode(Enum):
    NONE = 1
    READ = 3
    WRITE = 9
    ERROR = 15


@ac.struct
class Command:
    raw_opcode: ac.u4
    onehot_mask: ac.u2
    selector: Opcode


@ac.struct
class EnumResult:
    decoded: Opcode
    decoded_valid: bool
    onehot: Opcode
    onehot_present: bool
    onehot_conflict: bool
    selected: bool
    classification: ac.u8


@ac.rule
def classify(command: Command) -> EnumResult:
    decoded = ac.checked(command.raw_opcode, Opcode, fallback=Opcode.NONE)
    onehot = ac.onehot_enum(
        command.onehot_mask,
        members=(Opcode.READ, Opcode.WRITE),
        empty=Opcode.NONE,
        conflict=Opcode.ERROR,
    )
    return EnumResult(
        decoded=decoded.value,
        decoded_valid=decoded.valid,
        onehot=onehot.value,
        onehot_present=onehot.present,
        onehot_conflict=onehot.conflict,
        selected=command.selector.is_one_of(Opcode.READ, Opcode.WRITE),
        classification=ac.match_enum(
            command.selector,
            {
                Opcode.NONE: ac.literal(10, ac.u8),
                Opcode.READ: ac.literal(11, ac.u8),
                Opcode.WRITE: ac.literal(12, ac.u8),
                Opcode.ERROR: ac.literal(13, ac.u8),
            },
            invalid=ac.literal(255, ac.u8),
        ),
    )


@ac.system
def enum_helpers(command: Command) -> EnumResult:
    result = classify(command)
    return result


specialization = ac.jit(enum_helpers)
