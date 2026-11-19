# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac
from checks.counters import Counter


@ac.module
def RemoteCounter(rst_n: ac.u1, *, selected: ac.Domain) -> None:
    remote_count = Counter(rst_n, start=10, domain=selected)


@ac.module
def Pair(rst_n: ac.u1, *, domains: dict[str, ac.Domain]) -> None:
    slow = domains["io"]
    local_count = Counter(rst_n, start=1)
    RemoteCounter(rst_n, selected=slow)


@ac.module
def Hierarchy(rst_n: ac.u1, *, domains: list[ac.Domain], slot: int = 0) -> None:
    selected = domains[slot]
    Pair(rst_n, domains={"io": selected})
