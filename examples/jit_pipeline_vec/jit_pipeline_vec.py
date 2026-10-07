"""Three registered stages carrying a comparison tag and a 16-bit result."""

import pycircuit as ac


@ac.struct
class Packet:
    tag: ac.u1
    data: ac.u16


@ac.struct
class PipelineResult:
    tag: ac.u1
    data: ac.u16
    lo8: ac.u8


@ac.rule
def advance(first, second, third, a, b, sel) -> PipelineResult:
    result = PipelineResult(tag=third.tag, data=third.data, lo8=third.data[:8])
    third = second
    second = first
    first = Packet(tag=a == b, data=(a + b) if sel else (a ^ b))
    return result


@ac.module
def PipelineStorage(a: ac.u16, b: ac.u16, sel: ac.u1) -> PipelineResult:  # noqa: N802
    first: Packet = Packet()
    second: Packet = Packet()
    third: Packet = Packet()
    return advance(first, second, third, a, b, sel)


@ac.module
def JitPipelineVec(a: ac.u16, b: ac.u16, sel: ac.u1) -> PipelineResult:  # noqa: N802
    return PipelineStorage(a, b, sel)
