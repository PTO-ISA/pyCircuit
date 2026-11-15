# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac


@ac.module
def Producer(
    fifo: ac.AsyncFIFO[ac.u9, 4], reset: ac.u1, permit: ac.u1, enable: ac.u1
) -> None:
    token: ac.u9 = ac.state(init=0, reset=reset, reset_kind="async")
    shared = fifo

    @ac.rule
    def send():
        if enable:
            shared.push(token)
            ac.log("info", "sent", token)
            token = token + 1
            assert permit, "injected endpoint failure"


@ac.module
def Consumer(fifo: ac.AsyncFIFO[ac.u9, 4]) -> None:
    @ac.rule
    def receive():
        token = fifo.pop()
        ac.log("info", "received", token)
