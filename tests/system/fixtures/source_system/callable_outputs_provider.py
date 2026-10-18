# ruff: noqa: F823 -- hardware owner writes are captured without Python execution.
import pycircuit as ac


@ac.struct
class Packet:
    value: ac.u8


@ac.module
def TupleOwner():
    count: ac.u8 = 5

    @ac.method
    def advance(delta: ac.u8, enabled: bool) -> tuple[ac.u8, Packet, bool]:
        before = count
        count = count + delta
        return before, Packet(value=count), enabled

    @ac.method
    def singleton(packet: Packet) -> tuple[Packet]:
        return (packet,)
