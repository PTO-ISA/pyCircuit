import agentic_circuit as ac


@ac.struct
class LeftToken:
    value: ac.u16


@ac.struct
class RightToken:
    value: ac.u32


@ac.system
def pyc_barrier_pipeline() -> None:
    left = ac.source(LeftToken, depth=2, latency=1)
    right = ac.source(RightToken, depth=2, latency=1)
    left_ready, right_ready = left.barrier(right, depth=2, latency=1)
    ac.sink(left_ready)
    ac.sink(right_ready)
