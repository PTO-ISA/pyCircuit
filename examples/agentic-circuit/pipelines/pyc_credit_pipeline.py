import agentic_circuit as ac


@ac.struct
class CreditToken:
    sequence: ac.u4
    cycles: ac.u4
    value: ac.u16


@ac.system
def pyc_credit_pipeline() -> None:
    issued = ac.source(CreditToken, depth=4, latency=1)
    completed = issued.credit(
        cost=lambda item: item.cycles,
        credits=2,
        depth=4,
        latency=1,
    )
    ac.sink(completed)
