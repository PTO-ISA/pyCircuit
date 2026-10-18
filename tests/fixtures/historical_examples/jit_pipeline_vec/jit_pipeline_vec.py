# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
"""Three registered stages carrying a comparison tag and a 16-bit result."""

import pycircuit as ac


@ac.struct
class Packet:
    tag: ac.u1
    data: ac.u16


@ac.struct
class PipelineInput:
    a: ac.u16
    b: ac.u16
    sel: ac.u1


@ac.struct
class PipelineResult:
    tag: ac.u1
    data: ac.u16
    lo8: ac.u8


@ac.module
def PipelineStorage(request: PipelineInput) -> PipelineResult:  # noqa: N802
    first: Packet = Packet()
    second: Packet = Packet()
    third: Packet = Packet()

    @ac.rule
    def advance() -> PipelineResult:
        result = PipelineResult(tag=third.tag, data=third.data, lo8=third.data[:8])
        third = second
        second = first
        first = Packet(
            tag=request.a == request.b,
            data=(request.a + request.b) if request.sel else (request.a ^ request.b),
        )
        return result

    return advance()


@ac.module
def JitPipelineVec(a: ac.u16, b: ac.u16, sel: ac.u1) -> PipelineResult:  # noqa: N802
    request = PipelineInput(a=a, b=b, sel=sel)
    return PipelineStorage(request)
