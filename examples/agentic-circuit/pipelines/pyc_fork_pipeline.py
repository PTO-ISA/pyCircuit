import agentic_circuit as ac


@ac.system
def pyc_fork_pipeline() -> None:
    input_queue = ac.source(int)
    left, right = input_queue.fork(outputs=2, depth=2, latency=1)
    ac.sink(left)
    ac.sink(right)
