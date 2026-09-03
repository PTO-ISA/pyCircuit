from __future__ import annotations

from agentic_circuit import module, process, scope, system


@module
def top() -> None:
    with scope("controller"):
        Showcase(scenario=5, name="runtime")


@process(kind="workload")
def workload() -> None:
    yield_sim()


@system(root="top")
def main() -> None:
    return
