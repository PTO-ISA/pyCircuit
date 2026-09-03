from __future__ import annotations

from agentic_circuit import module, system


@module
def top() -> None:
    return


@system(root="top")
def main() -> None:
    return
