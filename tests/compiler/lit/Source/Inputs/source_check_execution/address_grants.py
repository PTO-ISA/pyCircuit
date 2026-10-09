"""Private address-mask transport beside user/owner/check result slots."""
# ruff: noqa: N802 -- hardware definition names are stable fixture identities.
import pycircuit as ac


@ac.struct
class Entry:
    a: ac.u8
    b: ac.u8


@ac.struct
class Ack:
    value: ac.u8


@ac.struct
class Result:
    a0: ac.u8
    b0: ac.u8
    a1: ac.u8
    b1: ac.u8
    left: ac.u8
    right: ac.u8
    user_a: ac.u8
    user_b: ac.u8


@ac.rule
def Put(entries, side, index, value, enable, allow) -> Ack:
    if enable:
        entries[index].a = value
        entries[index].b = value ^ 255
        side = value  # noqa: F841 - persistent owner proposal observed by Top
        assert allow, "address grant suffix"
    return Ack(value=value)


@ac.module
def Top(req_a: ac.u1, req_b: ac.u1, i: ac.u1, j: ac.u1,
        write_a: ac.u8, write_b: ac.u8, allow: ac.u1) -> Result:
    entries = ac.table[2, Entry](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    grant_a = req_a
    grant_b = req_b & (~grant_a | (i != j))
    a = Put(entries, left, i, write_a, grant_a, allow)
    b = Put(entries, right, j, write_b, grant_b, allow)
    return Result(a0=entries[0].a, b0=entries[0].b,
                  a1=entries[1].a, b1=entries[1].b,
                  left=left, right=right, user_a=a.value, user_b=b.value)
