"""Canonical payload declarations shared by independently compiled owners."""

from pycircuit import struct, u1, u8


@struct
class Event:
    value: u8


@struct
class Entry:
    valid: u1
    age: u8
    src0_tag: u8
    src0_ready: u1
    src1_tag: u8
    src1_ready: u1


@struct
class Wakeup:
    tag: u8
    valid: u1
