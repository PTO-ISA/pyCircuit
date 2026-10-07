"""A table update fixture with independent shape, payload and scalar state."""
import pycircuit as ac


@ac.struct
class Parcel:
    active: ac.u1
    payload: ac.bits[7]


@ac.rule
def revise(mailbox, cursor, audit, shadow, enable, index, value):
    snapshot = mailbox[index]
    prior_payload: ac.u7 = snapshot.payload
    active: ac.u1 = snapshot.active
    shadow = shadow + 2
    if enable:
        mailbox[index] = Parcel(active=1, payload=value)
        cursor = cursor + 1
        audit = audit + 5
    return {"prior_payload": prior_payload, "active": active, "cursor": cursor,
            "audit": audit, "snapshot": shadow}


@ac.module
def Scatter(enable: ac.u1, index: ac.u3, value: ac.u7) -> {  # noqa: N802
        "prior_payload": ac.u7, "active": ac.u1, "cursor": ac.u3,  # noqa: F821
        "audit": ac.u9, "snapshot": ac.u3}:  # noqa: F821
    mailbox = ac.table[8, Parcel](init=0)
    cursor: ac.u3 = 3
    audit: ac.u9 = 341
    saved = cursor
    return revise(mailbox, cursor, audit, saved, enable, index, value)
