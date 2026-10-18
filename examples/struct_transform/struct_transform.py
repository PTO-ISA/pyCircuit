# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
"""One registered packet with an old-state arithmetic projection."""

import pycircuit as ac


@ac.struct
class Packet:
    op: ac.u4
    dst: ac.u6
    word: ac.u32
    valid: ac.u1


@ac.module
def StructTransformStorage(  # noqa: N802 - hardware module definition
    packet: Packet,
) -> Packet:
    state: Packet = Packet()

    @ac.rule
    def capture_and_transform() -> Packet:
        op_wide: ac.u32 = state.op
        result = Packet(
            op=state.op,
            dst=state.dst,
            word=state.word + op_wide + 1,
            valid=state.valid,
        )
        state = packet
        return result

    return capture_and_transform()


@ac.module
def StructTransform(  # noqa: N802 - hardware module definition
    op: ac.u4,
    dst: ac.u6,
    word: ac.u32,
    valid: ac.u1,  # noqa: N802
) -> Packet:
    packet = Packet(op=op, dst=dst, word=word, valid=valid)
    result = StructTransformStorage(packet)
    return result
