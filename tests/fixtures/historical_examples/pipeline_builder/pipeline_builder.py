# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
"""Two registered packet stages with an increment between them."""

import pycircuit as ac


@ac.struct
class PipelinePacket:
    word: ac.u32
    valid: ac.u1


@ac.module
def PipelineStorage(packet: PipelinePacket) -> PipelinePacket:  # noqa: N802
    stage0: PipelinePacket = PipelinePacket()
    stage1: PipelinePacket = PipelinePacket()

    @ac.rule
    def advance() -> PipelinePacket:
        result = PipelinePacket(word=stage1.word, valid=stage1.valid)
        advanced_packet = PipelinePacket(word=stage0.word + 1, valid=stage0.valid)
        stage0 = packet
        stage1 = advanced_packet
        return result

    return advance()


@ac.module
def PipelineBuilder(word: ac.u32, valid: ac.u1) -> PipelinePacket:  # noqa: N802
    packet = PipelinePacket(word=word, valid=valid)
    result = PipelineStorage(packet)
    return result
