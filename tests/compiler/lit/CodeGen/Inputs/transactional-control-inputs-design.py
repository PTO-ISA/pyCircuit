# ruff: noqa: F823, F841, N802 -- Captured Module state.
import pycircuit as ac


@ac.system
def Top():
    phase: ac.u8 = 0
    commits: ac.u8 = 0
    stored: ac.u8 = 0
    queue = ac.queue[ac.u8](depth=2)

    @ac.rule(guard=lambda: (phase == 0) | (phase == 3))
    def produce() -> ac.u8:
        return phase + 10

    queue = produce()

    @ac.rule(guard=lambda value: phase != 1)
    def consume(value: ac.u8) -> ac.u8:
        if (phase == 0) | (phase == 5):
            return ac.u8(5)
        if phase == 2:
            first = value + 1
        else:
            first = value + 2
        commits = commits + 1
        ac.log("info", "read", value, first, value, commits)
        return first + value

    stored = consume(queue)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, commits, stored)
        phase = phase + 1
