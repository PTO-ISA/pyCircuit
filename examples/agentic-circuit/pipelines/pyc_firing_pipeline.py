import agentic_circuit as ac


@ac.struct
class FiringToken:
    value: ac.u16


@ac.system
def pyc_firing_pipeline() -> None:
    incoming = ac.source(FiringToken, depth=2, latency=1)
    outgoing = incoming.firing(
        lambda queue: queue.push(queue.pop().with_fields(value=queue.peek().value + 1)),
        depth=2,
        latency=1,
    )
    ac.sink(outgoing)
