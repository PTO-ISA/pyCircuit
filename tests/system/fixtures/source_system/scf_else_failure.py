# ruff: noqa: B007, F823, F841 -- captured module-owned state proposals.
import pycircuit as ac


@ac.system
def LoopElseFailure():
    stop: ac.u1 = 0
    allow: ac.u1 = 1
    total: ac.u8 = 5
    peer: ac.u8 = 7

    @ac.rule
    def update() -> None:
        before = total
        for index in range(3):
            total = total + 1
            ac.log("info", "loop body", before, total)
            if stop:
                break
        else:
            total = total + 20
            ac.log("info", "loop else", before, total)
        assert allow, "loop_else_late_check"
        ac.report("loop total", total)

    @ac.rule
    def sibling() -> None:
        peer = peer + 1
        ac.log("info", "loop sibling", peer)

    update()
    sibling()
