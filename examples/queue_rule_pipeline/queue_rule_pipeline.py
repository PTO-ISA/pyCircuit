# ruff: noqa: F823 -- captured module-owned state proposals.
"""Typed Queue calls and an atomic stateful processing stage."""

import pycircuit as ac


@ac.struct
class Packet:
    tag: ac.u4
    value: ac.u8


@ac.module
def Accumulator():  # noqa: N802
    total: ac.u8 = 0

    @ac.method
    def add(packet: Packet) -> Packet:
        value = packet.value
        for _step in range(2):
            value = value + 1
        total = total + value
        return Packet(tag=packet.tag, value=total)

    @ac.method
    def snapshot() -> ac.u8:
        return total


@ac.module
def QueueRulePipeline():  # noqa: N802
    ingress = ac.queue[Packet](depth=1, latency=1)
    egress = ac.queue[Packet](depth=1, latency=1)
    accumulator = Accumulator()
    transferred: ac.u8 = 0

    @ac.method
    def put(packet: Packet) -> None:
        ingress.push(packet)

    @ac.method
    def get() -> Packet:
        return egress.pop()

    @ac.method
    def snapshot() -> tuple[ac.u8, ac.u8]:
        return accumulator.snapshot(), transferred

    @ac.rule
    def transfer() -> None:
        packet = ingress.pop()
        result = accumulator.add(packet)
        transferred = transferred + 1
        egress.push(result)

    @ac.rule
    def progress() -> None:
        ac.log("info", "queue progress", transferred)
