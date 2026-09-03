import agentic_circuit as ac


@ac.system
def pyc_latency_pipeline() -> None:
    input_queue = ac.source(int, depth=2, latency=1)
    output_queue = input_queue.apply(
        lambda item: item + 1,
        depth=4,
        latency=3,
    )
    ac.sink(output_queue)
