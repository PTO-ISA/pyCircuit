# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 5

    @ac.rule(guard=lambda: count < 7)
    def increment() -> None:
        count = count + 1

    @ac.rule(after=(increment,))
    def read() -> ac.u8:
        return count

    @ac.method(after=(increment,), guard=lambda: count == 6)
    def at_six() -> ac.u8:
        return count

    @ac.method
    def old() -> ac.u8:
        return count

    @ac.method(guard=lambda allow: allow)
    def require(allow: ac.u1) -> None:
        pass


@ac.module
def Ledger():  # noqa: N802
    balance: ac.u8 = 40
    marker: ac.u8 = 90

    @ac.method
    def deposit(change_marker: ac.u1) -> None:
        balance = balance + 10
        if change_marker:
            marker = marker + 1

    @ac.method
    def read() -> tuple[ac.u8, ac.u8]:
        return balance, marker

    @ac.method(guard=lambda: balance == 50)
    def at_fifty() -> ac.u8:
        return marker
