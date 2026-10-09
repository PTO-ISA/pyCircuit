"""An eight-bit increment between the original two token stages."""

import pycircuit as ac
from pycircuit import module, rule


@ac.struct
class IncrementResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u8


@module
def InferredBoundaryPipeline(  # noqa: N802
    valid: ac.u1, data: ac.u8, take: ac.u1
) -> IncrementResult:
    ready, available, value = ac.queue[ac.u8](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    incremented = value + 1
    stage_ready, out_valid, out_data = ac.queue[ac.u8](
        available, incremented, take, depth=1, ready_policy="downstream_pop"
    )
    return IncrementResult(ready=ready, valid=out_valid, data=out_data)
