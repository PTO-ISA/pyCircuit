# ruff: noqa: F823, F841, N802 -- Captured Module state.
from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    LEFT = 1
    RIGHT = 2


@ac.struct
class Packet:
    data: ac.bits[8]
    mode: Mode


@ac.system
def Top():
    phase: ac.bits[8] = 0
    stored: Packet = Packet(ac.bits[8](0), Mode.RIGHT)
    tag: ac.bits[8] = 0

    if phase == 0:
        unknown = ac.four_state("10xz")
        probe = Packet(ac.four_state("10xz0101"), Mode.RIGHT)
    else:
        unknown = ac.four_state("z01x")
        probe = Packet(ac.four_state("z01x1001"), Mode.LEFT)

    @ac.method
    def choose(value: ac.bits[8]) -> tuple[Packet, Mode]:
        packet = Packet(value + 3, Mode.LEFT)
        mode = Mode.RIGHT
        if phase == 0:
            packet = Packet(value + 1, Mode.LEFT)
            mode = Mode.LEFT
        elif phase == 1:
            packet = Packet(value + 2, Mode.RIGHT)
        return packet, mode

    @ac.rule
    def produce() -> tuple[Packet, ac.bits[8]]:
        packet, mode = choose(ac.bits[8](10))
        if mode == Mode.LEFT:
            return packet, ac.bits[8](1)
        if phase == 1:
            return Packet(packet.data + 10, Mode.LEFT), ac.bits[8](2)
        return packet, ac.bits[8](3)

    stored, tag = produce()

    @ac.autorule
    def tick() -> None:
        if probe.mode == Mode.RIGHT:
            selected = ac.bits[8](1)
        else:
            selected = ac.bits[8](2)
        ac.log("info", "four_state", unknown, probe.data, selected)
        ac.log("info", "state", phase, stored.data, stored.mode == Mode.RIGHT, tag)
        phase = phase + 1
