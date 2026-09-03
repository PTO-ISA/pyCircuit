import agentic_circuit as ac


@ac.system
def pyc_queue_pipeline() -> None:
    input_queue = ac.source(int, depth=2, latency=1)
    output_queue = input_queue.apply(
        lambda item: item + 1,
        depth=2,
        latency=1,
    )
    ac.sink(output_queue)
