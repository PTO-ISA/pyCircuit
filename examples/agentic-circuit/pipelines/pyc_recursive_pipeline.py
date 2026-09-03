import agentic_circuit as ac


def add_stages(queue, count):
    if count == 0:
        return queue
    return add_stages(
        queue.apply(lambda item: item + 1, depth=2, latency=1),
        count - 1,
    )


@ac.system
def pyc_recursive_pipeline() -> None:
    incoming = ac.source(int, depth=2, latency=1)
    outgoing = add_stages(incoming, 3)
    ac.sink(outgoing)
