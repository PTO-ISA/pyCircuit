# ruff: noqa: F823 -- captured module-owned state proposal.
import pycircuit as ac


@ac.module
def SharedRawBound(x: ac.u1):
    tokens = ac.queue[ac.u1](depth=1, latency=1, out_ready=x)

    @ac.rule
    def send() -> None:
        # x is independently authored public payload and raw-bound take.
        tokens.push(x)


@ac.system
def Top():
    x: ac.u1 = 0
    owner = SharedRawBound(x)

    @ac.rule
    def send() -> None:
        owner.send()

    @ac.rule
    def tick() -> None:
        x = ~x  # noqa: F841 - captured hardware state or child instance.

    send()
    tick()
