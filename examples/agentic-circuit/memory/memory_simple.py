"""Shared explicit-memory Queue frontend example for contract epoch 0.4."""

import agentic_circuit as ac


@ac.struct
class MemoryRequest:
    address: ac.u4
    data: ac.u16
    tag: ac.u8


@ac.system
def memory_simple() -> None:
    # One physical SRAM is owned by the root scope. Both child scopes borrow it.
    sram = ac.memory(ac.u16, entries=16, init=0, latency=1)

    # Canonical endpoint ordinal 0: fixed-priority writer.
    with ac.scope("a_writer"):
        writes = ac.source(MemoryRequest, depth=4, latency=1)
        write_responses = sram.request(
            writes,
            address=lambda request: request.address,
            write=lambda request: True,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
        )
        ac.sink(write_responses)

    # Canonical endpoint ordinal 1: reader, accepted only when the SRAM is idle
    # and no writer request is valid.
    with ac.scope("b_reader"):
        reads = ac.source(MemoryRequest, depth=4, latency=1)
        read_responses = sram.request(
            reads,
            address=lambda request: request.address,
            write=lambda request: False,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
        )
        ac.sink(read_responses)
