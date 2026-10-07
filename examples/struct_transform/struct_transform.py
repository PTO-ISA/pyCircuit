"""One registered packet with an old-state arithmetic projection."""

import pycircuit as ac


@ac.struct
class Packet:
    op: ac.u4
    dst: ac.u6
    word: ac.u32
    valid: ac.u1


@ac.rule
def capture_and_transform(state, op, dst, word, valid) -> Packet:
    op_wide: ac.u32 = state.op
    result = Packet(
        op=state.op,
        dst=state.dst,
        word=state.word + op_wide + 1,
        valid=state.valid,
    )
    state = Packet(op=op, dst=dst, word=word, valid=valid)
    return result


@ac.module
def StructTransformStorage(op: ac.u4, dst: ac.u6, word: ac.u32,  # noqa: N802
                           valid: ac.u1) -> Packet:
    state: Packet = Packet()
    return capture_and_transform(state, op, dst, word, valid)


@ac.module
def StructTransform(op: ac.u4, dst: ac.u6, word: ac.u32,  # noqa: N802
                    valid: ac.u1) -> Packet:
    result = StructTransformStorage(op, dst, word, valid)
    return result
