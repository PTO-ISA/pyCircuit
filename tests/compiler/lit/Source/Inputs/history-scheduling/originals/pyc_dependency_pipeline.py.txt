import agentic_circuit as ac


@ac.struct
class Token:
    sequence: ac.u4
    waits_for: ac.u4
    resource: ac.u1
    cycles: ac.u4
    value: ac.u16


@ac.system
def pyc_dependency_pipeline() -> None:
    issued = ac.source(Token, depth=4, latency=1)
    completed = issued.depend(
        key=lambda item: item.sequence,
        waits_for=lambda item: item.waits_for,
        resource=lambda item: item.resource,
        cost=lambda item: item.cycles,
        capacity=4,
        resources=2,
        no_dependency=15,
        depth=4,
        latency=1,
    )
    ac.sink(completed)
