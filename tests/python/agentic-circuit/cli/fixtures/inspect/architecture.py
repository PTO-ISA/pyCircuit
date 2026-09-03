from __future__ import annotations

from agentic_circuit import module, process, system


@module
def chip() -> None:
    return


@process(kind="workload")
def workload() -> None:
    yield_sim()


@system(root="chip")
def main() -> None:
    return
