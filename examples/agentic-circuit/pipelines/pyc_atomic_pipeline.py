import agentic_circuit as ac


@ac.system
def pyc_atomic_pipeline() -> None:
    left = ac.source(int)
    right = ac.source(int)
    with ac.atomic():
        left_next = left.apply(lambda item: item + 1)
        right_next = right.apply(lambda item: item * 2)
    ac.sink(left_next)
    ac.sink(right_next)
