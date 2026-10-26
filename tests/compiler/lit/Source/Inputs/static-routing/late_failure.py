"""A routed write and sibling write must both discard on a later source check."""

# ruff: noqa: F823, F841 -- hardware state and allocations.
import pycircuit as ac


@ac.module
def Counter():
    count: ac.u8 = 13

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def write(incoming: ac.u8) -> None:
        count = incoming


@ac.module
def Worker(peer: Counter):
    progress: ac.u8 = 0

    @ac.rule
    def step() -> None:
        previous = peer.read()
        peer.write(previous + 3)
        progress = progress + 1
        assert previous == 13, "routed_late_failure"
        ac.log("info", "accepted", previous, progress)



@ac.system
def Top():
    peer = Counter()
    worker = Worker(peer)
    sibling: ac.u8 = 29

    @ac.rule
    def advance() -> None:
        sibling = sibling + 5
