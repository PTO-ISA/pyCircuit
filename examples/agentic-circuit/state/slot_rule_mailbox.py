"""Two equivalent ways to consume a module-owned slot from a rule."""

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


@ac.module
def explicit_mailbox(incoming: Event) -> Event:
    mailbox = ac.slot(incoming)
    outgoing = consume(mailbox)
    return outgoing


@ac.module
def nested_mailbox(incoming: Event) -> Event:
    mailbox = ac.slot(incoming)

    @ac.rule
    def consume_captured() -> Event:
        if mailbox.valid:
            event = mailbox.value
            mailbox.release()
            return event

    outgoing = consume_captured()
    return outgoing


@ac.system
def slot_rule_mailbox(left: Event, right: Event) -> tuple[Event, Event]:
    explicit_result = explicit_mailbox(left)
    nested_result = nested_mailbox(right)
    return explicit_result, nested_result
