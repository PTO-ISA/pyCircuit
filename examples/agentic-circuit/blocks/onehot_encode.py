"""Encode one-hot or one-or-zero flags while preserving malformed-input status."""

import agentic_circuit as ac


@ac.struct
class EncodedFlags:
    flags: ac.u8
    index: ac.u3
    valid: bool
    conflict: bool


@ac.rule
def encode_flags(value: EncodedFlags) -> EncodedFlags:
    return value.with_fields(
        index=ac.onehot_encode(value.flags, order="low").index,
        valid=ac.onehot_encode(value.flags, order="low").valid,
        conflict=ac.onehot_encode(value.flags, order="low").conflict,
    )


@ac.system
def onehot_encode(value: EncodedFlags) -> EncodedFlags:
    result = encode_flags(value)
    return result


specialization = ac.jit(onehot_encode)
