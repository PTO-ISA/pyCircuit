"""Encode every flags token, including zero and multi-hot values."""

# ruff: noqa: N802 -- hardware module name.
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
    ingress = ac.queue[EncodedFlags](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[EncodedFlags](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        value = ingress.pop()
        encoded = encode_flags(value)
        egress.push(encoded)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
