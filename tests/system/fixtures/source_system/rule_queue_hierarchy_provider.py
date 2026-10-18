# ruff: noqa: F823 -- captured module-owned state proposals.
import pycircuit as ac


@ac.module
def HierarchyQueue():
    phase: ac.u1 = 0
    cycle: ac.u8 = 0
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def send(value: ac.u8) -> None:
        tokens.push(value)

    @ac.rule
    def selected() -> ac.u1:
        return phase

    @ac.rule
    def take() -> ac.u8:
        return tokens.pop()

    @ac.rule
    def local_take() -> None:
        if not phase:
            value = tokens.pop()
            ac.log("info", "queue hierarchy local", cycle, value)

    @ac.rule
    def tick() -> None:
        phase = ~phase  # noqa: F841 - captured hardware state or child instance.
        cycle = cycle + 1  # noqa: F841 - captured hardware state or child instance.

    local_take()
    tick()


@ac.module
def HierarchyRelay():
    child = HierarchyQueue()

    @ac.rule
    def send(value: ac.u8) -> None:
        child.send(value)

    @ac.rule
    def selected() -> ac.u1:
        return child.selected()

    @ac.rule
    def take() -> ac.u8:
        return child.take()
