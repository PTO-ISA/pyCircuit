# ruff: noqa: F821, N802 -- the dead forward cycle is the rejection oracle.
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
        valid, data, b_ready & take, depth=2, ready_policy="downstream_pop"
    )
    b_ready, b_valid, b_data = ac.queue[ac.u13](
        valid, data, a_ready & take, depth=3, ready_policy="downstream_pop"
    )
    return GraphResult()
