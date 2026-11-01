# ruff: noqa: F823, F841, N802 -- Captured System state.
"""Pure calls retain natural widths, independent invocations and return paths."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    LEFT = 1
    RIGHT = 2


@ac.struct
class Packet:
    value: ac.u8
    mode: Mode


def transform(packet: Packet, mode: Mode, enabled: bool) -> tuple[Packet, Mode, bool]:
    if enabled:
        return Packet(packet.value + 1, mode), packet.mode, enabled
    return packet, packet.mode, enabled


def relay(packet: Packet, mode: Mode, enabled: bool) -> tuple[Packet, Mode, bool]:
    transformed, original_mode, selected = transform(packet, mode, enabled)
    return transformed, original_mode, selected


def increment(value: ac.u8) -> ac.u8:
    return value + 10


def twice(value: ac.u8) -> ac.u8:
    return increment(increment(value))


def widen(value: ac.u8) -> tuple[ac.u16, ac.u16]:
    return ac.u16(value + ac.u8(10)), ac.u16(value << ac.u8(1))


def select(value: ac.u8, phase: ac.u8) -> tuple[ac.u8, ac.u8]:
    if phase == 0:
        return value, ac.u8(30)
    if phase < 2:
        adjusted = value + 1
        tag = ac.u8(40)
    else:
        adjusted = value + 2
        tag = ac.u8(50)
    return adjusted, tag


@ac.system
def Top():
    phase: ac.u8 = 0

    @ac.autorule
    def observe() -> None:
        value = ac.u8(250) + phase
        shared = increment(value)
        separate = increment(value + 1)
        added, shifted = widen(value)
        chosen, tag = select(shared, phase)
        packet, mode, enabled = relay(Packet(shared, Mode.LEFT), Mode.RIGHT, phase < 2)
        ac.log(
            "info",
            "nominal",
            packet.value,
            packet.mode == Mode.RIGHT,
            mode == Mode.LEFT,
            enabled,
        )
        ac.log(
            "info",
            "data",
            shared,
            shared + 1,
            separate,
            twice(value),
            added,
            shifted,
            chosen,
            tag,
        )
        phase = phase + 1
