# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""Selected element updates preserve old reads and untouched record fields."""

import pycircuit as ac


@ac.struct
class Entry:
    tag: ac.u8
    count: ac.u8


@ac.module
def RegisterBank():
    regs: ac.table[4, ac.u8] = (1, 2, 3, 4)
    entries: ac.table[4, Entry] = (
        Entry(10, 100),
        Entry(20, 110),
        Entry(30, 120),
        Entry(40, 130),
    )
    index: ac.u2 = 0

    @ac.autorule
    def update() -> None:
        before = regs[index]
        regs[index] += 10
        entries[index].count += 1
        ac.log("info", "selected", index, before, regs[index], entries[index].count)
        index += 1

    @ac.autorule
    def observe() -> None:
        ac.log("info", "regs", regs[0], regs[1], regs[2], regs[3])
        ac.log(
            "info",
            "entries",
            entries[0].tag,
            entries[0].count,
            entries[1].tag,
            entries[1].count,
            entries[2].tag,
            entries[2].count,
            entries[3].tag,
            entries[3].count,
        )


@ac.system(
    samples=(
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
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
    )
)
def Top():
    RegisterBank()
