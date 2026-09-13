"""One design owns two module instances with distinct dependent state types."""

import agentic_circuit as ac

ENTRY_COUNT = ac.param[int]("entry_count")


@ac.struct
class Entry:
    index: ac.bits[ac.index_width(ENTRY_COUNT)]


@ac.struct
class Packet:
    value: ac.u8


@ac.rule
def keep(entries, value: Packet) -> Packet:
    entries[0] = Entry(index=0)
    return value


@ac.module
def stage(value: Packet, *, entry_count: ac.const[int]) -> Packet:
    entries: list[Entry] = [0] * entry_count
    result = keep(entries, value)
    return result


@ac.system
def multi_specialization_types(value: Packet) -> Packet:
    first = stage(value, entry_count=5)
    second = stage(first, entry_count=6)
    return second
