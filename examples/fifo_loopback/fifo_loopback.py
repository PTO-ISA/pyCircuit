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
    buffer = ac.queue[ac.u8](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=in_valid,
        in_data=in_data,
        out_ready=out_ready,
    )
    return FifoResult(
        in_ready=buffer.ready, out_valid=buffer.valid, out_data=buffer.data
    )
