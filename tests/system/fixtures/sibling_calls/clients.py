import pycircuit as ac
from checks.counter import Counter, Ledger


@ac.module
def Writer(  # noqa: N802
    counter: Counter, ledger: Ledger, allow: ac.u1, change_marker: ac.u1
):
    alias = counter

    @ac.rule
    def write() -> None:
        alias.increment()
        ledger.deposit(change_marker)
        alias.require(allow)
        ac.log("info", "sibling written", alias.old())


@ac.module
def Reader(counter: Counter, ledger: Ledger):  # noqa: N802
    alias = counter

    @ac.rule
    def observe() -> None:
        value = alias.read()
        balance, marker = ledger.read()
        ac.log("info", "sibling visible", value, balance, marker)

    @ac.rule
    def observe_six() -> None:
        value = alias.at_six()
        balance, marker = ledger.read()
        ac.log("info", "sibling six", value, balance, marker)

    @ac.rule
    def observe_balance() -> None:
        value = alias.read()
        marker = ledger.at_fifty()
        ac.log("info", "sibling fifty", value, marker)


@ac.module
def Relay(counter: Counter, ledger: Ledger):  # noqa: N802
    alias = counter
    Reader(alias, ledger)
