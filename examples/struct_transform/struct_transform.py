# ruff: noqa: F821, F823, F842 -- Module state captured by Rules is compiler-owned.
"""One registered packet with an old-state arithmetic projection."""

import pycircuit as ac


@ac.struct
class Packet:
    op: ac.u4
    dst: ac.u6
    word: ac.u32
    valid: ac.u1


@ac.module
def StructTransformStorage() -> None:  # noqa: N802
    state: Packet = Packet()

    @ac.method
    def capture_and_transform(packet: Packet) -> Packet:
        op_wide: ac.u32 = state.op
        result = Packet(
            op=state.op,
            dst=state.dst,
            word=state.word + op_wide + 1,
            valid=state.valid,
        )
        state = packet
        return result


@ac.module
def StructTransform(  # noqa: N802 - hardware module definition
    op: ac.u4,
    dst: ac.u6,
    word: ac.u32,
    valid: ac.u1,  # noqa: N802
) -> Packet:
    storage: StructTransformStorage

    @ac.rule
    def capture_and_transform() -> Packet:
        packet = Packet(op=op, dst=dst, word=word, valid=valid)
        return storage.capture_and_transform(packet)

    return capture_and_transform()
