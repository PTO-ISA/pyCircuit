# ruff: noqa: F821, N802 -- forward hardware wires and module names are intentional.
import pycircuit as ac
from pycircuit import queue as fifo


@ac.struct
class GraphResult:
    a_ready: ac.u1
    a_valid: ac.u1
    a_data: ac.u13
    b_ready: ac.u1
    b_valid: ac.u1
    b_data: ac.u13


@ac.module
def Top(valid: ac.u1, data: ac.u13, take: ac.u1) -> GraphResult:
    a_ready, a_valid, a_data = fifo[ac.u13](
        valid, data, b_ready & take, depth=2, ready_policy="local_occupancy"
    )
    b_ready, b_valid, b_data = ac.queue[ac.u13](
        valid, data, a_ready & take, depth=3, ready_policy="local_occupancy"
    )
    return GraphResult(
        a_ready=a_ready,
        a_valid=a_valid,
        a_data=a_data,
        b_ready=b_ready,
        b_valid=b_valid,
        b_data=b_data,
    )
