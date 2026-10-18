# ruff: noqa: F823, F841 -- captured hardware locals and state proposals.
import pycircuit as ac


@ac.system
def Top():
    enabled: ac.u1 = 0
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def probe(path: ac.u1) -> None:
        ignored = tokens.peek()

    @ac.rule
    def drive() -> None:
        probe(enabled)

    drive()
