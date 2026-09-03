import agentic_circuit as ac


@ac.struct
class LoopToken:
    remaining: ac.u4
    stop: bool
    skip: bool


@ac.system
def pyc_loop_control_pipeline() -> None:
    current = ac.source(LoopToken, depth=2, latency=1)
    while current.remaining > 0:
        if current.stop:
            break
        current = current.apply(
            lambda item: item.with_fields(remaining=item.remaining - 1),
            depth=1,
            latency=1,
        )
        if current.skip:
            continue
    ac.sink(current)
