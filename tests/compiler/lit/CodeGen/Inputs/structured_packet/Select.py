"""Pure nominal selection merges data leaves for unknown selectors."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    LEFT = 1
    RIGHT = 3


@ac.struct
class Header:
    data: ac.u4
    mode: Mode


@ac.struct
class Packet:
    header: Header
    tag: ac.u4


def choose(selector: ac.u1, left: Packet, right: Packet) -> Packet:
    return left if selector else right


@ac.system
def Top():  # noqa: N802
    @ac.autorule
    def observe() -> None:
        left = Packet(Header(ac.four_state("10xz"), Mode.LEFT), ac.u4(10))
        right = Packet(Header(ac.four_state("11zz"), Mode.RIGHT), ac.u4(9))
        for case in range(4):
            selector = ac.u1(0)
            if case == 1:
                selector = ac.u1(1)
            if case == 2:
                selector = ac.four_state("x")
            if case == 3:
                selector = ac.four_state("z")
            packet = choose(selector, left, right)
            mode = Mode.LEFT if selector else Mode.RIGHT
            ac.log(
                "info",
                "selected",
                ac.u2(case),
                packet.header.data,
                packet.header.mode,
                packet.tag,
                mode,
            )
