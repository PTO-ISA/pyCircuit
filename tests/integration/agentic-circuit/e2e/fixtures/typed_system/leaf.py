import agentic_circuit as ac

from contracts import (
    AccessKind,
    Entry,
    ReadRequest,
    ReadResult,
    WriteAck,
    WriteRequest,
)


@ac.rule
def read_entry(entries, request, owner_generation):
    selected = ac.find(
        entries,
        where=lambda entry: entry.valid
        & (entry.generation == request.generation),
    )
    return ReadResult(
        request=request,
        value=selected.value.value if selected.valid else 0,
        found=selected.valid,
    )


@ac.rule
def write_entry(entries, request, owner_generation):
    old = entries[request.index]
    accepted = (
        request.valid
        & (request.kind == AccessKind.WRITE)
        & (request.generation == owner_generation)
        & (not old.valid)
    )
    if accepted:
        entries[request.index] = Entry(
            generation=request.generation,
            value=request.value,
            valid=True,
        )
    return WriteAck(request=request, accepted=accepted)


@ac.module
def typed_state_leaf(
    read_request: ReadRequest,
    write_request: WriteRequest,
    *,
    owner_generation: ac.const[int],
) -> tuple[ReadResult, WriteAck]:
    entries = ac.table[128, Entry](init=0)
    read_result = read_entry(entries, read_request, owner_generation)
    write_ack = write_entry(entries, write_request, owner_generation)
    return read_result, write_ack
