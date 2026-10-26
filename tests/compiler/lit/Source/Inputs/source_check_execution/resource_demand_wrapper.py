# ruff: noqa: F823 -- captured module-owned state proposal.
import pycircuit as ac


@ac.module
def SharedLeaf():
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.method
    def push(value: ac.u8) -> None:
        tokens.push(value)

    @ac.method
    def take() -> ac.u8:
        return tokens.pop()


@ac.module
def CallerA():
    child = SharedLeaf()

    @ac.rule
    def call() -> None:
        child.push(7)



@ac.module
def WrapperB():
    child = SharedLeaf()  # noqa: F841 - captured hardware state or child instance.


@ac.system
def Top():
    caller = CallerA()  # noqa: F841 - captured hardware state or child instance.
    wrapper = WrapperB()  # noqa: F841 - captured hardware state or child instance.
    cycle: ac.u8 = 0

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1  # noqa: F841 - captured hardware state or child instance.
