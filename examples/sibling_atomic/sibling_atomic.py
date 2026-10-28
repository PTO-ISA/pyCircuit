# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 10

    @ac.method
    def increment() -> None:
        count = count + 3

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def committed() -> ac.u8:
        return count


@ac.module
def Ledger():  # noqa: N802
    total: ac.u8 = 100

    @ac.method(guard=lambda allow: allow)
    def record(allow: ac.u1) -> None:
        total = total + 10

    @ac.method
    def read() -> ac.u8:
        return total


@ac.module
def Writer(counter: Counter, ledger: Ledger, allow: ac.u1):  # noqa: N802
    @ac.rule
    def transfer() -> None:
        counter.increment()
        ledger.record(allow)


@ac.module
def Reader(counter: Counter, ledger: Ledger):  # noqa: N802
    @ac.rule
    def observe() -> None:
        count = counter.read()
        total = ledger.read()
        ac.log("info", "atomic visible", count, total)
