# ruff: noqa: B011, F823, F841 -- hardware state and assertion probes.
import pycircuit as ac


@ac.module
def ConditionalCounter():
    count: ac.u8 = 5

    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        count = count + delta
        ac.log("info", "conditional add", delta, count)
        return count

    @ac.method(guard=lambda allow, delta: allow)
    def guarded_add(allow: ac.u1, delta: ac.u8) -> ac.u8:
        return add(delta)

    @ac.method(guard=lambda allow: allow)
    def stop(allow: ac.u1) -> None:
        pass

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def snapshot() -> None:
        ac.report("conditional count", count)

    @ac.method
    def sequence(choose: ac.u1) -> ac.u8:
        first = add(1)
        if choose:
            if choose:
                selected = add(2)
            else:
                selected = add(7)
        elif choose:
            selected = add(9)
        else:
            selected = add(4)
        after = add(3)
        ac.log("info", "conditional snapshots", first, selected, after)
        assert count < 24, "conditional candidate limit"
        snapshot()
        return after
