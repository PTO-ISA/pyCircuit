"""An eight-bit depth-two FIFO with ordinary typed inputs and result."""

from __future__ import annotations

import pycircuit as ac


@ac.struct
class FifoResult:
    in_ready: ac.u1
    out_valid: ac.u1
    out_data: ac.u8


@ac.module
def FifoLoopback(  # noqa: N802
    in_valid: ac.u1, in_data: ac.u8, out_ready: ac.u1
) -> FifoResult:
    in_ready, out_valid, out_data = ac.queue[ac.u8](
        in_valid,
        in_data,
        out_ready,
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    return FifoResult(in_ready=in_ready, out_valid=out_valid, out_data=out_data)
