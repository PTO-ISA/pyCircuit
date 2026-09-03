import agentic_circuit as ac


@ac.struct
class Item:
    value: int
    remaining: int


@ac.system
def pyc_struct_pipeline() -> None:
    input_queue = ac.source(Item, depth=2, latency=1)
    output_queue = input_queue.apply(
        lambda item: item.with_fields(
            value=item.value + 1,
            remaining=item.remaining - 1,
        ),
        depth=2,
        latency=1,
    )
    ac.sink(output_queue)
