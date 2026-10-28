# ruff: noqa: F823, F841 -- captured hardware locals and state proposals.
import pycircuit as ac


@ac.system
def Top():
    phase: ac.u8 = 0
    saved: ac.u8 = 0
    output = ac.queue[ac.u8](depth=1, latency=1)

    @ac.method(guard=lambda enabled: enabled)
    def guarded(enabled: ac.u1) -> ac.u8:
        return 7

    @ac.rule
    def drive() -> None:
        value = guarded(phase != 0)
        if phase == 1:
            saved = value
        output.push(phase)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
