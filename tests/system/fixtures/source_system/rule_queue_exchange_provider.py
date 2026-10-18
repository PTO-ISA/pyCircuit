import pycircuit as ac


@ac.module
def ConditionalQueueExchange():
    left = ac.queue[ac.u8](depth=1, latency=1)
    right = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def seed() -> None:
        left.push(10)
        right.push(40)

    @ac.rule
    def step(cycle: ac.u8, selected: ac.u1) -> None:
        if selected:
            first = left.pop()
            second = right.pop()
        else:
            first = right.pop()
            second = left.pop()
        left.push(first + 1)
        right.push(second + 2)
        ac.log("info", "queue conditional first", cycle, first)
        ac.log("info", "queue conditional second", cycle, second)
