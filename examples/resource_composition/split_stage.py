# ruff: noqa: F823 -- Module state is compiler-owned.
import pycircuit as ac


@ac.module
def SplitStage(source: ac.u8, peer: ac.module) -> tuple[ac.u8, ac.u8]:  # noqa: N802
    accepted: ac.u8 = 0

    @ac.rule
    def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
        total: ac.u8 = peer.add(value)
        accepted = accepted + 1
        ac.log("info", "split", value, total)
        return value + 10, value + 20

    left, right = split(source)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "accepted", accepted)

    return left, right
