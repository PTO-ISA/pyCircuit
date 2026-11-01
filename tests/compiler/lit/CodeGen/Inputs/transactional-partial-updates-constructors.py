# ruff: noqa: F821, N802 -- Captured System state.
"""Explicit values, declared defaults and recursive zero stay distinct."""

import pycircuit as ac
from partial_case.types import Inner, Mode, Packet


@ac.system
def Top():
    initial: Packet()

    @ac.autorule
    def observe() -> None:
        explicit = Packet(mode=Mode.WRITE, preset=Inner(tag=11), raw=Inner(flags=4))
        ac.log(
            "info",
            "defaults",
            initial.raw.tag,
            initial.raw.flags,
            initial.preset.tag,
            initial.preset.flags,
            initial.mode == Mode.READ,
            initial.raw.active,
            initial.preset.active,
        )
        ac.log(
            "info",
            "explicit",
            explicit.raw.tag,
            explicit.raw.flags,
            explicit.preset.tag,
            explicit.preset.flags,
            explicit.mode == Mode.WRITE,
            explicit.raw.active,
            explicit.preset.active,
        )
