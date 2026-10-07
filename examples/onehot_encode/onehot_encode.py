"""Encode every flags token, including zero and multi-hot values."""
# ruff: noqa: F821, N802 -- immutable forward queue wires, hardware root name.
import pycircuit as ac


@ac.struct
class EncodedFlags:
    flags: ac.u8
    index: ac.u3
    # Explicit one-bit storage for the historical Boolean field(s).
    valid: ac.u1
    conflict: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: EncodedFlags


@ac.rule
def encode_flags(value: EncodedFlags) -> EncodedFlags:
    index, valid, conflict = ac.onehot_encode(value.flags, order="low")
    result = value
    result.index = index
    # Each field boundary is fresh fixed u1; valid/conflict stay Boolean aliases.
    result.valid = valid
    result.conflict = conflict
    return result


@ac.module
def OnehotEncode(valid: ac.u1, data: EncodedFlags, take: ac.u1) -> Result:
    ready, available, value = ac.queue[EncodedFlags](
        valid, data, result_ready, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    result = encode_flags(value)
    result_ready, out_valid, out_data = ac.queue[EncodedFlags](
        available, result, take, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
