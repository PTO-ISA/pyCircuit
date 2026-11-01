# ruff: noqa: F821, F823, F841, N802 -- Captured Module state and instances.
"""Nested Methods compose disjoint writes but all reads retain committed Q."""

import pycircuit as ac


@ac.struct
class Packet:
    tag: ac.u8
    word: ac.u8


@ac.module
def Counter(*, LO: int = 0, HI: int = 4):  # noqa: N803 -- Static Integer parameters.
    packet: Packet = Packet(ac.u8(2), ac.u8(165))
    alias = packet

    @ac.method(guard=lambda phase: phase != 1)
    def finish(phase: ac.u8) -> None:
        alias.word[HI:] += ac.u4(1)

    @ac.method
    def update(phase: ac.u8) -> None:
        packet.tag += 1
        packet.word[LO:HI] = packet.tag[LO:HI]
        finish(phase)
        ac.log("info", "old", packet.tag, packet.word)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "state", packet.tag, packet.word)


@ac.module
def Stream():
    counter: Counter
    alias = counter
    phase: ac.u8 = 0

    @ac.autorule
    def advance() -> None:
        alias.update(phase)

    @ac.autorule
    def tick() -> None:
        assert phase < 4
        ac.log("info", "phase", phase)
        phase += 1


@ac.system(
    samples=(
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 1),
        (1, 1),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
    )
)
def Top():
    Stream()


@ac.system
def Failure():
    Stream()
