import pycircuit as ac


@ac.struct
class Header:
    sequence: ac.u8


@ac.struct
class Payload:
    data: ac.u8


@ac.rule
def advance(count, enable):
    if enable:
        count = count + 1


@ac.module
def StatefulTuple(payload: ac.u8, enable: ac.u1) -> tuple[Header, Payload, ac.u1]:
    count: ac.u8 = 5
    header = Header(sequence=count)
    advance(count, enable)
    return header, Payload(data=payload), enable
