"""Sparse enum decoding with explicit fallback and balanced classifications."""

# ruff: noqa: F821, N802 -- forward ready wire and hardware root name.
from enum import Enum
import pycircuit as ac


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
    decoded_valid: ac.u1
    onehot: Opcode
    onehot_present: ac.u1
    onehot_conflict: ac.u1
    selected: ac.u1
    classification: ac.u8


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: EnumResult


@ac.rule
def classify(command: Command) -> EnumResult:
    raw_enum, decoded_valid = ac.enum_from_bits[Opcode](command.raw_opcode)
    r_none = raw_enum == Opcode.NONE
    r_read = raw_enum == Opcode.READ
    r_write = raw_enum == Opcode.WRITE
    decode_low = Opcode.NONE if r_none else Opcode.READ
    decode_high = Opcode.WRITE if r_write else Opcode.ERROR
    decode_candidate = decode_low if (r_none or r_read) else decode_high
    decoded = decode_candidate if decoded_valid else Opcode.NONE

    b0 = command.onehot_mask[0:1] != 0
    b1 = command.onehot_mask[1:2] != 0
    present = b0 or b1
    conflict = ac.popcount(command.onehot_mask) > 1
    onehot_candidate = Opcode.READ if b0 else Opcode.WRITE
    nonconflict = onehot_candidate if present else Opcode.NONE
    onehot = Opcode.ERROR if conflict else nonconflict

    s_none = command.selector == Opcode.NONE
    s_read = command.selector == Opcode.READ
    s_write = command.selector == Opcode.WRITE
    s_error = command.selector == Opcode.ERROR
    selected = s_read or s_write
    class_low: ac.u8 = 10 if s_none else 11
    class_high: ac.u8 = 12 if s_write else 13
    class_candidate = class_low if (s_none or s_read) else class_high
    classification: ac.u8 = (
        class_candidate if ((s_none or s_read) or (s_write or s_error)) else 255
    )
    return EnumResult(
        decoded=decoded,
        decoded_valid=decoded_valid,
        onehot=onehot,
        onehot_present=present,
        onehot_conflict=conflict,
        selected=selected,
        classification=classification,
    )


@ac.module
def EnumHelpers(valid: ac.u1, data: Command, take: ac.u1) -> Result:
    ready, available, command = ac.queue[Command](
        valid,
        data,
        result_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    classified = classify(command)
    result_ready, out_valid, out_data = ac.queue[EnumResult](
        available,
        classified,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
