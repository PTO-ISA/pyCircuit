# ruff: noqa: F823, F841 -- captured hardware locals and state proposals.
import pycircuit as ac


@ac.system
def Top():
    phase: ac.u8 = 0
    left = ac.queue[ac.u8](depth=1, latency=1)
    right = ac.queue[ac.u8](depth=1, latency=1)
    output = ac.queue[ac.u8](depth=1, latency=1)

    @ac.method(guard=lambda selector, enabled: enabled)
    def consume(selector: ac.u1, enabled: ac.u1) -> ac.u8:
        if selector:
            value = left.pop()
        else:
            value = right.pop()
        output.push(value)
        return value

    @ac.rule
    def drive() -> None:
        if phase == 0:
            left.push(11)
            right.push(7)
        else:
            value = consume(phase == 1, phase != 3)
            ac.log("info", "demand consume", phase, value)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
