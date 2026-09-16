"""Nested-rule form of atomic slot consumption."""

import agentic_circuit as ac


@ac.struct
class Event:
    value: ac.u8


@ac.module
def mailbox_module(incoming: Event) -> Event:
    mailbox = ac.slot(incoming)

    @ac.rule
    def consume() -> Event:
        if mailbox.valid:
            event = mailbox.value
            mailbox.release()
            return event

    outgoing = consume()
    return outgoing


@ac.system
def slot_rule_transaction_nested(incoming: Event) -> Event:
    return mailbox_module(incoming)
