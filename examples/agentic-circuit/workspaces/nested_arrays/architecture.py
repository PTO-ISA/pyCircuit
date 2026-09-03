from __future__ import annotations

from agentic_circuit import module, process, scope, system


@module
def top() -> None:
    with scope("lanes"):
        Showcase(scenario=3, name="runtime")


@process(kind="workload")
def workload() -> None:
    yield_sim()


@system(root="top")
def main() -> None:
    return
