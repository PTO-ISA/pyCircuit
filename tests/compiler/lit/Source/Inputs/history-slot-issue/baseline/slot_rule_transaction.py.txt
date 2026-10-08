"""Atomic rule consumption of a committed one-entry slot."""

import agentic_circuit as ac


@ac.struct
class Event:
    value: ac.u8


@ac.rule
def consume(mailbox) -> Event:
    if mailbox.valid:
        event = mailbox.value
        mailbox.release()
        return event


@ac.system
def slot_rule_transaction(incoming: Event) -> Event:
    mailbox = ac.slot(incoming)
    outgoing = consume(mailbox)
    return outgoing
