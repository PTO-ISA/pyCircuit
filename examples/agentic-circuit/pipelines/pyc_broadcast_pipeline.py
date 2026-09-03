import agentic_circuit as ac


@ac.system
def pyc_broadcast_pipeline() -> None:
    input_queue = ac.source(int, depth=2, latency=1)
    left = input_queue.apply(lambda item: item + 1)
    right = input_queue.apply(lambda item: item + 2)
    merged = left.merge(right, policy="priority", depth=2, latency=1)
    ac.sink(merged)
