"""Three top choices contrast old Cell feedback, exclusive calls, and a real ring."""
from pycircuit import ac


@ac.module
def CellFeedback():
    a = ac.cell[ac.u32](initial=1)
    b = ac.cell[ac.u32](initial=2)

    @ac.rule
    def update_a():
        a.value = b.value + 1

    @ac.rule
    def update_b():
        b.value = a.value + 1

    update_a()
    update_b()
    return a, b


@ac.module
def ExclusiveRing(choose: bool):
    left = ac.queue[ac.u32](initial=1)
    right = ac.queue[ac.u32](initial=2)

    @ac.rule
    def forward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source.value

    @ac.rule
    def backward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source.value

    if choose:
        right = forward(left)
    else:
        left = backward(right)
    return left, right


@ac.module
def QueueRing():
    left = ac.queue[ac.u32](initial=1)
    right = ac.queue[ac.u32](initial=2)

    @ac.rule
    def forward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source.value

    @ac.rule
    def backward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source.value

    right = forward(left)
    left = backward(right)
    return left, right  # Full/full capacity dependency cycle: CLI should fail.
