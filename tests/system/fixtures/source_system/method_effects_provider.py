# ruff: noqa: B011, F823, F841 -- hardware owner writes and unreachable assertion probe.
import pycircuit as ac


@ac.module
def MethodCounter():
    count: ac.u8 = 0

    @ac.method
    def bump(delta: ac.u8) -> ac.u8:
        before = count
        count = count + delta
        ac.log("info", "bump", "before", before, "delta", delta, "after", count)
        return before

    @ac.method
    def verify(limit: ac.u8) -> None:
        assert count < limit, "candidate count must remain below limit"

    @ac.method
    def snapshot() -> None:
        ac.report("method count", count)

    @ac.method(guard=lambda allow: allow)
    def stop(allow: bool) -> None:
        count = count

    @ac.method
    def suppressed() -> None:
        assert False, "later false guard suppresses this assertion"
        ac.log("warning", "suppressed", count)
        stop(False)
