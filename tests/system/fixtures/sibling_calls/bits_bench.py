# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac
from checks.bits import BitsOwner
from checks.bits_clients import BitsReader, BitsWriter


@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 0)))
def ExerciseBits():  # noqa: N802
    phase: ac.u4 = 0
    owner = BitsOwner()
    BitsWriter(owner, ac.four_state("10xz"), phase != 1)
    BitsReader(owner)

    @ac.rule
    def tick() -> None:
        ac.log("info", "bits old", owner.old())
        phase = phase + 1

    tick()
