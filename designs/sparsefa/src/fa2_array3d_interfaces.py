"""Typed interfaces for the registered 16x16x16 FA2 dot-product array.

The right operand stores column vectors: right[n].cK is B[k,n].
An advance is one enabled pipeline edge; omitting it holds every stage.
"""
import pycircuit as ac
from fa2.fa2_array_interfaces import Row16


@ac.struct
class Array3DBegin:
    tag: ac.u32
    length: ac.u7
    cell_mask: ac.bits[256]


@ac.struct
class Array3DAdvance:
    action: ac.u2
    tag: ac.u32
    index: ac.u3
    left: ac.table[16, Row16]
    right: ac.table[16, Row16]
    row_mask: ac.u16
    col_mask: ac.u16
    k_mask: ac.u16


@ac.struct
class Cell3DStep:
    left: Row16
    right: Row16
    k_mask: ac.u16
    enabled: ac.u1


@ac.struct
class Row3DStep:
    left: Row16
    right: ac.table[16, Row16]
    k_mask: ac.u16
    mask: ac.u16


@ac.struct
class Array3DResult:
    tag: ac.u32
    rows: ac.table[16, Row16]
    groups: ac.u3
    useful_macs: ac.u32
