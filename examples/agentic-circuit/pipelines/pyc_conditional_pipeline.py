import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u32
    route: ac.u1


@ac.system
def pyc_conditional_pipeline() -> None:
    input_queue = ac.source(Item, depth=2, latency=1)
    if input_queue.route == 0:
        output_queue = input_queue.apply(
            lambda item: item.with_fields(value=item.value + 10)
        )
    else:
        output_queue = input_queue.apply(
            lambda item: item.with_fields(value=item.value + 20)
        )
    ac.sink(output_queue)
