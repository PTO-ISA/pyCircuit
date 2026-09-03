from __future__ import annotations

from agentic_circuit import module, process, system


@module
def top() -> None:
    return


@process(kind="workload")
def workload() -> None:
    yield_sim()


@system(root="top")
def main() -> None:
    return None
