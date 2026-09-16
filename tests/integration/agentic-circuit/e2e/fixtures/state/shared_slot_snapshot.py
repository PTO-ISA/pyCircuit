"""Let one rule observe a slot while another atomically releases it."""

import agentic_circuit as ac


@ac.struct
class Event:
    value: ac.u8


@ac.rule
def observe(mailbox) -> Event:
    if mailbox.valid:
        return mailbox.value


@ac.rule
def consume(mailbox) -> Event:
    if mailbox.valid:
        event = mailbox.value
        mailbox.release()
        return event


@ac.system
def shared_slot_snapshot(incoming: Event) -> tuple[Event, Event]:
    mailbox = ac.slot(incoming)
    observed = observe(mailbox)
    consumed = consume(mailbox)
    return observed, consumed
