"""Agentic Circuit architecture entry point."""

from agentic_circuit import module, system


@module
def Top() -> None:
    pass


@system
def main() -> None:
    Top()
