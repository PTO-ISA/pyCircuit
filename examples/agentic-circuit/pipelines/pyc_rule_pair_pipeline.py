import agentic_circuit as ac


@ac.rule
def increment(value):
    return value + 1


@ac.rule
def double(value):
    return value * 2


@ac.system
def pyc_rule_pair_pipeline() -> None:
    left = ac.source(int)
    right = ac.source(int)
    left_next = increment(left)
    right_next = double(right)
    ac.sink(left_next)
    ac.sink(right_next)
