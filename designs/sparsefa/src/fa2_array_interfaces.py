"""Typed payloads for the standalone P0T outer-product array."""

import pycircuit as ac


@ac.struct
class Row16:
    c0: ac.u32
    c1: ac.u32
    c2: ac.u32
    c3: ac.u32
    c4: ac.u32
    c5: ac.u32
    c6: ac.u32
    c7: ac.u32
    c8: ac.u32
    c9: ac.u32
    c10: ac.u32
    c11: ac.u32
    c12: ac.u32
    c13: ac.u32
    c14: ac.u32
    c15: ac.u32


@ac.struct
class ArrayBegin:
    tag: ac.u32
    length: ac.u7
    cell_mask: ac.bits[256]


@ac.struct
class ArrayStep:
    tag: ac.u32
    index: ac.u7
    left: ac.table[16, ac.u32]
    right: ac.table[16, ac.u32]
    row_mask: ac.u16
    col_mask: ac.u16


@ac.struct
class RowStep:
    left: ac.u32
    right: ac.table[16, ac.u32]
    mask: ac.u16
    enabled: ac.u1


@ac.struct
class ArrayResult:
    tag: ac.u32
    rows: ac.table[16, Row16]
    steps: ac.u7
    useful_macs: ac.u32
