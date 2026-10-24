"""An available first write cannot hide demand for a later selected index."""

import pycircuit as ac


@ac.module
def IndexProvider(valid: bool) -> ac.u2:
    @ac.rule(guard=lambda: valid)
    def produce() -> ac.u2:
        return 3

    return produce()


@ac.module
def Bank(index: ac.u2, choose: bool) -> tuple[ac.u8, ac.u8]:
    entries = ac.table[4, ac.u8](init=0)

    @ac.rule
    def write() -> None:
        entries[0] = 7
        if choose:
            entries[index] = 9

    write()
    return entries[0], entries[3]


@ac.module
def Top(valid: bool, choose: bool) -> tuple[ac.u8, ac.u8]:
    index = IndexProvider(valid)
    first, last = Bank(index, choose)
    return first, last


@ac.system(samples=((0, 0), (1, 0), (0, 0), (1, 0)))
def Exercise():
    masked_first, masked_last = Top(False, False)
    denied_first, denied_last = Top(False, True)
    first_only, untouched_last = Top(True, False)
    both_first, both_last = Top(True, True)

    @ac.rule
    def observe() -> None:
        ac.log("info", "indexed-demand", masked_first, masked_last,
               denied_first, denied_last, first_only, untouched_last,
               both_first, both_last)

    observe()
