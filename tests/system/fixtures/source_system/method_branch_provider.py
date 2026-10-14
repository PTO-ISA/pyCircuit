# ruff: noqa: B011, F823, F841 -- hardware owner writes and source assertions.
import pycircuit as ac


@ac.module
def BranchCounter():
    count: ac.u8 = 5

    @ac.method
    def advance(flag: ac.u1) -> ac.u8:
        before = count
        count = count + 1
        if flag:
            saved = count
            count = count + 2
        else:
            saved = count + 10
        after = count
        count = count + 4
        ac.log(
            "info",
            "branch advance",
            "before",
            before,
            "saved",
            saved,
            "after",
            after,
            "final",
            count,
        )
        assert count < 35, "branch candidate must remain below limit"
        return saved

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def snapshot() -> None:
        ac.report("branch count", count)


@ac.module
def BranchProbeOwner():
    value: ac.u8 = 5

    @ac.method
    def one_arm(flag: ac.u1, data: ac.u8) -> None:
        if flag:
            value = data

    @ac.method
    def both_arms(flag: ac.u1, data: ac.u8) -> None:
        if flag:
            value = data
        else:
            value = data

    @ac.method
    def prior_write(flag: ac.u1, data: ac.u8) -> None:
        value = value + 1
        if flag:
            value = data

    @ac.method
    def identity(flag: ac.u1) -> None:
        if flag:
            value = value

    @ac.method(guard=lambda allow, flag, data: allow)
    def guarded_one_arm(allow: ac.u1, flag: ac.u1, data: ac.u8) -> None:
        if flag:
            value = data

    @ac.method(guard=lambda allow: allow)
    def guarded_identity(allow: ac.u1) -> None:
        value = value

    @ac.method
    def snapshot() -> None:
        ac.log("info", "probe value", value)
        ac.report("probe value", value)
