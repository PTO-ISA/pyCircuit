# ruff: noqa: N802 -- pyCircuit Module name.
"""Native byte-memory admission probe for the payload consumer."""

import pycircuit as ac


@ac.module
def MemoryProbe(write: bool, address: ac.u16, data: ac.u64) -> ac.u64:
    memory = ac.byte_mem[ac.u64](depth=65536)

    @ac.rule
    def access() -> ac.u64:
        before = memory.read(address)
        if write:
            memory.write(address, data)
        return before

    return access
