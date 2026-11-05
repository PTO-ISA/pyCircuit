# ruff: noqa: N802, N803 -- pyCircuit module and static parameter names.
"""Decode the Calendar baseline's 16-byte register-pair entry.

The 40-node layout is fixed by Calendar §1.7. The 48-node layout is still Q1;
this module does not emit or reinterpret an entry for that configuration.
"""

import pycircuit as ac


@ac.struct
class RouteRegisters:
    lo: ac.u64
    hi: ac.u64


@ac.struct
class DecodedEntry:
    pair: ac.u2
    land_count: ac.u8
    flags: ac.u8
    expected_bytes: ac.u32
    fault: bool


# ndf: implements=WSEMIG-ROUTE-001
@ac.module
def DecodeEntry(entry: RouteRegisters, *, NODE: int = 0) -> DecodedEntry:
    @ac.rule
    def decode() -> DecodedEntry:
        # The largest low shift is 78, so seven bits carry every shift exactly.
        node: ac.u7 = NODE
        assert node < 40, "Calendar entry requires node 0..39"
        low = entry.lo >> (2 * node)
        high = entry.hi >> (2 * node - 64 if node >= 32 else 64)
        pair = low[0:2] | high[0:2]
        return DecodedEntry(
            pair=pair,
            land_count=entry.hi[16:24],
            flags=entry.hi[24:32],
            expected_bytes=entry.hi[32:64],
            fault=pair == 1,
        )

    return decode
