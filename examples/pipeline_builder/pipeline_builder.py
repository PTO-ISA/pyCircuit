"""Two registered packet stages with an increment between them."""

import pycircuit as ac


@ac.struct
class PipelinePacket:
    word: ac.u32
    valid: ac.u1


@ac.rule
def advance(stage0, stage1, word, valid) -> PipelinePacket:
    result = PipelinePacket(word=stage1.word, valid=stage1.valid)
    advanced_packet = PipelinePacket(word=stage0.word + 1, valid=stage0.valid)
    stage0 = PipelinePacket(word=word, valid=valid)
    stage1 = advanced_packet
    return result


@ac.module
def PipelineStorage(word: ac.u32, valid: ac.u1) -> PipelinePacket:  # noqa: N802
    stage0: PipelinePacket = PipelinePacket()
    stage1: PipelinePacket = PipelinePacket()
    return advance(stage0, stage1, word, valid)


@ac.module
def PipelineBuilder(word: ac.u32, valid: ac.u1) -> PipelinePacket:  # noqa: N802
    result = PipelineStorage(word, valid)
    return result
