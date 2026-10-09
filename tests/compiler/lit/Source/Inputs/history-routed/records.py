"""Canonical packed payload of the original four-route dependency graph."""

from pycircuit import struct, u2, u8, u16, u64


@struct
class WorkItem:
    sequence_id: u8
    opcode: u8
    route: u2
    waits_for: u8
    cycles: u16
    value: u64
