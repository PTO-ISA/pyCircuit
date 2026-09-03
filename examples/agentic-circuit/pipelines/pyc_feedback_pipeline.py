import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u32
    remaining: ac.u4


@ac.system
def pyc_feedback_pipeline() -> None:
    current = ac.source(Item, depth=2, latency=1)
    while current.remaining > 0:
        current = current.apply(
            lambda item: item.with_fields(
                value=item.value + 1,
                remaining=item.remaining - 1,
            ),
            depth=1,
            latency=1,
        )
    ac.sink(current)
