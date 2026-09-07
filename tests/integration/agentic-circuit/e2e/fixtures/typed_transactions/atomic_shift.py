"""Typed state transaction with one output and four owner-local writes."""

import agentic_circuit as ac


@ac.struct
class Command:
    push: bool
    value: ac.u8


@ac.rule
def shift(entries, command):
    entry0 = entries[0]
    entry1 = entries[1]
    entry2 = entries[2]
    if command.push:
        entries[3] = entry2
        entries[2] = entry1
        entries[1] = entry0
        entries[0] = command.value
    return command


@ac.system
def atomic_shift(command: Command) -> Command:
    entries: list[ac.u8] = [0] * 4
    snapshot = shift(entries, command)
    return snapshot
