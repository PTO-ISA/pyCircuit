"""Extract and replace instruction fields with generic slices and concat."""

import pycircuit as ac


@ac.struct
class DecodeItem:
    word: ac.u32
    opcode: ac.u6
    opcode_rd: ac.u11
    immediate: ac.u17
    mode: ac.u3
    rd: ac.u5
    low25: ac.u25
    updated: ac.u32


@ac.struct
class DecodeResult:
    ready: ac.u1
    valid: ac.u1
    data: DecodeItem


def decode(item: DecodeItem) -> DecodeItem:
    result = item
    result.opcode = item.word[26:32]
    result.opcode_rd = item.word[21:32]
    result.immediate = item.word[4:21]
    result.updated = ac.concat(
        item.word[26:32], item.rd, item.word[4:21], item.mode, item.word[:1]
    )
    return result


@ac.module
def BitfieldDecodePipeline(  # noqa: N802
    valid: ac.u1, data: DecodeItem, take: ac.u1
) -> DecodeResult:
    ingress = ac.queue[DecodeItem](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[DecodeItem](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        item = ingress.pop()
        decoded = decode(item)
        egress.push(decoded)

    transfer()
    return DecodeResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
