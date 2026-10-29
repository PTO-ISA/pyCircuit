# ruff: noqa: F823 -- Module state captured by Methods is compiler-owned.
import pycircuit as ac


@ac.module
def Receiver():  # noqa: N802
    count: ac.bits[8] = 5
    has_room = count < 7

    @ac.method(guard=lambda: has_room)
    def accept(amount: ac.bits[8]) -> None:
        count = count + amount
        ac.log("info", "accept", count, amount)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "receiver", count)
