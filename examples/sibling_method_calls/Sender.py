# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
import pycircuit as ac


@ac.module
def Sender(peer: ac.module):  # noqa: N802
    count: ac.bits[8] = 0

    @ac.rule
    def send() -> None:
        count = count + 1
        ac.log("info", "send", count)
        amount: ac.bits[8] = 1
        peer.accept(amount)

    send()

    @ac.autorule
    def observe() -> None:
        ac.log("info", "sender", count)
