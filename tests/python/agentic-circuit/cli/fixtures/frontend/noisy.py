from __future__ import annotations

print("project noise on stdout")

from agentic_circuit import module, system


@module
def top() -> None:
    return


@system(root="top")
def main() -> None:
    return
