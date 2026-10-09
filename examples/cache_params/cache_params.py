"""Fixed cache geometry with a combinational address-tag projection."""

import pycircuit as ac


@ac.struct
class CacheResult:
    tag: ac.u28
    line_words: ac.u9 = 8
    tag_bits: ac.u9 = 28


@ac.module
def CacheParams(addr: ac.u40) -> CacheResult:  # noqa: N802
    return CacheResult(tag=addr[12:40])
