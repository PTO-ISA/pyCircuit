"""Combine Queue input, persistent state, output, and Slot release."""

import agentic_circuit as ac


@ac.struct
class Event:
    value: ac.u8


@ac.struct
class Request:
    increment: ac.u8


@ac.rule
def apply(count, mailbox, request) -> Event:
    if mailbox.valid:
        event = mailbox.value
        next_count = count + request.increment
        count = next_count
        mailbox.release()
        return event.with_fields(value=event.value + next_count)


@ac.system
def slot_rule_atomic_effects(incoming: Event, request: Request) -> Event:
    count: ac.u8 = 0
    mailbox = ac.slot(incoming)
    outgoing = apply(count, mailbox, request)
    return outgoing
