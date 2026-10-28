"""Generic Method bodies retain the selected instance's internal width."""

import pycircuit as ac


@ac.module
def Provider(*, WIDTH: int = 16):
    @ac.method
    def apply(value: ac.u8) -> ac.u64:
        widened: ac.bits[WIDTH] = value
        configured: ac.bits[WIDTH] = 0x100015
        combined = widened + configured
        rows: ac.table[2, ac.bits[WIDTH]] = (widened, ~combined)
        fragment: ac.bits[WIDTH] = rows[1][14:22]
        return rows[1] ^ fragment


@ac.system(samples=((0, 0), (1, 0)))
def Top():
    provider = Provider(WIDTH=24)

    @ac.rule
    def run() -> None:
        known = provider.apply(173)
        unknown = provider.apply(ac.four_state("xxxxxxxx"))
        floating = provider.apply(ac.four_state("zzzzzzzz"))
        ac.log("info", "bound known", known)
        ac.log("info", "bound x", unknown)
        ac.log("info", "bound z", floating)
