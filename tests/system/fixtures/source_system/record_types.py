from pycircuit import struct, u1, u8


@struct
class Inner:
    flag: u1 = 1
    data: u8 = 7


@struct
class Packet:
    inner: Inner
    sequence: u8 = 19


@struct
class ForeignPacket:
    inner: Inner
    sequence: u8 = 19
