"""Ordered mask/equality selections with four-state mux merging."""

import pycircuit as ac


@ac.struct
class DecodeResult:
    op: ac.u4
    len: ac.u3


@ac.module
def DecodeRules(insn: ac.u8) -> DecodeResult:  # noqa: N802
    masked = insn & 240
    hit_10 = masked == 16
    hit_20 = masked == 32
    hit_30 = masked == 48

    # Later matching entries select over the preceding result.
    op_10 = 1 if hit_10 else 0
    op_20 = 2 if hit_20 else op_10
    opcode = 3 if hit_30 else op_20
    length_10 = 4 if hit_10 else 0
    length_20 = 4 if hit_20 else length_10
    length = 4 if hit_30 else length_20
    return DecodeResult(op=opcode, len=length)
