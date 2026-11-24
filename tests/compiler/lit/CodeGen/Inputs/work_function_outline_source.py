"""Exercise aggregate transport, byte arithmetic and checked state across Work calls."""

import pycircuit as ac


@ac.struct
class Payload:
    lanes: ac.table[2, ac.u8]
    raw: ac.u8


@ac.module
def MediumMix(x: ac.u8) -> ac.u8:  # noqa: N802
    mixed = x
    for _round in range(24):
        mixed = (mixed + x) ^ 0x5A
    return mixed


@ac.module
def MixingLeaf(  # noqa: N802
    packet: Payload, row: ac.bits[73], shift: ac.bits[73]
) -> tuple[ac.u8, ac.u8, ac.u9]:
    mixed = packet.lanes[0]
    for _round in range(448):
        mixed = (mixed + ~packet.raw) ^ 0x5A
    shifted = row >> shift
    return mixed + packet.lanes[1], packet.raw, shifted[3:12]


@ac.module
def MixingBranch(  # noqa: N802
    x: ac.u8, row: ac.bits[73], shift: ac.bits[73]
) -> tuple[ac.u8, ac.u8, ac.u9]:
    packet = Payload(lanes=(x, x ^ 0x33), raw=x)
    mixed, raw, window = MixingLeaf(packet, row, shift)
    return mixed, raw, window


@ac.module
def CheckedHistory(mixed: ac.u8, allow: bool) -> ac.u8:  # noqa: N802
    saved: ac.u8 = 0

    @ac.rule
    def record() -> None:
        saved = mixed  # noqa: F841
        assert allow, "history update rejected"

    return saved


@ac.module
def OutlineTop(  # noqa: N802
    x: ac.u8, allow: bool, row: ac.bits[73], shift: ac.bits[73]
) -> tuple[ac.u8, ac.u8, ac.u8, ac.u8, ac.u9]:
    mixed, raw, window = MixingBranch(x, row, shift)
    saved = CheckedHistory(mixed, allow)
    return mixed, saved, raw, MediumMix(x), window
