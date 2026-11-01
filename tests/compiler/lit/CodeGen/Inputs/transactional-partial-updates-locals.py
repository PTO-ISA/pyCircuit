# ruff: noqa: F823, F841, N802 -- Captured System state.
"""Local reconstruction preserves snapshots, nominal fields and X/Z siblings."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    LEFT = 1
    RIGHT = 2


@ac.struct
class Inner:
    tag: ac.u8
    mode: Mode


@ac.struct
class Packet:
    inner: Inner
    word: ac.u8
    data: ac.u8


def transform(packet: Packet, enabled: ac.u1) -> tuple[Packet, Packet]:
    saved = packet
    packet.inner.tag += 1
    packet.word[:4] = ac.u4(5)
    packet.word[4:8] += ac.u4(2)
    packet.word[1] = ac.u1(1)
    if enabled:
        packet.inner.tag += 10
        packet.inner.mode = Mode.RIGHT
    else:
        packet.inner.tag += 20
    packet.inner.tag[:] = packet.inner.tag + 2
    control = ac.four_state("xz000000")
    control[:1] = ac.u1(1)
    if control[:1]:
        packet.inner.tag += 2
    return packet, saved


@ac.module
def Sample() -> Packet:
    value: Packet = Packet(Inner(ac.u8(3), Mode.LEFT), ac.u8(160), ac.u8(0))
    return value


@ac.system
def Top():
    phase: ac.u8 = 0
    source = Sample()
    updated: Packet = Packet(Inner(ac.u8(0), Mode.LEFT), ac.u8(0), ac.u8(0))

    @ac.rule
    def transform_input(packet: Packet) -> Packet:
        saved = packet
        packet.inner.tag += 4
        packet.word[:4] = ac.u4(9)
        ac.log(
            "info", "formal", packet.inner.tag, packet.word, saved.inner.tag, saved.word
        )
        return packet

    updated = transform_input(source)

    @ac.autorule
    def observe() -> None:
        original = Packet(
            Inner(ac.u8(3), Mode.LEFT), ac.u8(160), ac.four_state("10xz01zx")
        )
        changed, saved = transform(original, phase == 0)
        ac.log(
            "info",
            "local",
            changed.inner.tag,
            changed.word,
            changed.inner.mode == Mode.RIGHT,
            saved.inner.tag,
            saved.word,
            original.inner.tag,
            changed.data,
        )
        ac.log("info", "formal-output", updated.inner.tag, updated.word)
        phase += 1
