"""Permute a word and restore its original mode bits without losing X/Z."""

import pycircuit as ac


@ac.struct
class ScalarResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u32


@ac.module
def BitfieldScalarPipeline(  # noqa: N802
    valid: ac.u1, data: ac.u32, take: ac.u1
) -> ScalarResult:
    ready, available, word = ac.queue[ac.u32](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    permuted = ac.concat(word[:17], word[20:32], word[:3])
    stage_ready, out_valid, out_data = ac.queue[ac.u32](
        available, permuted, take, depth=1, ready_policy="downstream_pop"
    )
    return ScalarResult(ready=ready, valid=out_valid, data=out_data)
