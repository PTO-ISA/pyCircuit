import agentic_circuit as ac


@ac.system
def pyc_route_merge_pipeline() -> None:
    input_queue = ac.source(int, depth=2, latency=1)
    left, right = input_queue.route(
        outputs=2,
        key=lambda item: item,
        depth=2,
        latency=1,
    )
    left_done = left.apply(lambda item: item + 10)
    right_done = right.apply(lambda item: item + 20)
    merged = left_done.merge(right_done, policy="priority", depth=2, latency=1)
    ac.sink(merged)
