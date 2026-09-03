import agentic_circuit as ac


@ac.struct
class Token:
    sequence: ac.u32
    value: ac.u32


@ac.system
def pyc_reorder_pipeline() -> None:
    completed = ac.source(Token, depth=8, latency=1)
    retired = completed.reorder(
        key=lambda item: item.sequence,
        capacity=16,
        start=0,
        depth=4,
        latency=1,
    )
    ac.sink(retired)
