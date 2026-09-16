"""Mandatory inline helpers remain closed inside Table predicate regions."""

import agentic_circuit as ac


@ac.struct
class Entry:
    tag: ac.u8
    valid: bool


@ac.struct
class Request:
    tag: ac.u8


@ac.struct
class Result:
    index: ac.u2
    valid: bool


@ac.inline
def matches(entry: Entry, request: Request) -> bool:
    return entry.valid & (entry.tag == request.tag)


@ac.rule
def lookup(entries, request: Request) -> Result:
    selected = entries.find(where=lambda entry: matches(entry, request))
    return Result(index=selected.index, valid=selected.valid)


@ac.module
def tag_array(request: Request) -> Result:
    entries = ac.table[4, Entry](init=0)
    result = lookup(entries, request)
    return result


@ac.system
def helper_table_find(request: Request) -> Result:
    result = tag_array(request)
    return result
