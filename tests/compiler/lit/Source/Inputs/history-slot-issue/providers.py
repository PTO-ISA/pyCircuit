"""Resident mailboxes and four-entry Issue Table with the historical old-Q rules."""

# ruff: noqa: F821, F841 -- forward queue connections and hardware proposals.
from history_slot_issue.payload_types import Entry, Event, Wakeup
from history_slot_issue.valid_cell import valid_cell
from history_slot_issue.payload_cell import payload_cell
from pycircuit import module, queue, rule, struct, table, u1


@struct
class MailboxResult:
    incoming_take: u1
    outgoing_valid: u1
    outgoing_data: Event


@rule
def consume(
    slot_valid, slot_payload, incoming_available, incoming_head: Event, outgoing_space
) -> MailboxResult:
    capture = ~slot_valid & incoming_available
    publish = slot_valid & outgoing_space
    event = slot_payload
    if capture:
        slot_valid = 1
        slot_payload = incoming_head
    if publish:
        slot_valid = 0
    return MailboxResult(
        incoming_take=capture, outgoing_valid=publish, outgoing_data=event
    )


@module
def explicit_mailbox(
    incoming_available: u1, incoming_head: Event, outgoing_space: u1
) -> MailboxResult:
    slot_valid: u1 = 0
    slot_payload: Event = 0
    result = consume(
        slot_valid, slot_payload, incoming_available, incoming_head, outgoing_space
    )
    return result


@module
def nested_mailbox(
    incoming_available: u1,
    incoming_head: Event,
    outgoing_space: u1,
) -> {"incoming_take": u1, "outgoing_valid": u1, "outgoing_data": Event}:
    slot_valid = valid_cell()
    slot_payload = payload_cell()
    capture = (slot_valid.q == 0) & (incoming_available != 0)
    release = (slot_valid.q != 0) & (outgoing_space != 0)
    saved_incoming = incoming_head
    saved_payload = slot_payload.q

    @rule
    def consume_captured():
        slot_valid(en=capture | release, d=capture)

    @rule
    def capture_payload():
        slot_payload(en=capture, d=saved_incoming)

    consume_captured()
    capture_payload()
    return {
        "incoming_take": capture,
        "outgoing_valid": release,
        "outgoing_data": saved_payload,
    }


@struct
class IssueResult:
    allocation_take: u1
    wakeup_take: u1
    issued_valid: u1
    issued_data: Entry
    install: u1
    wakeup_release: u1
    selected: u1


@rule
def update_issue(
    entries,
    allocation_valid,
    allocation_payload,
    wakeup_valid,
    wakeup_payload,
    allocation_available,
    allocation_head: Entry,
    wakeup_available,
    wakeup_head: Wakeup,
    free_index,
    install,
    issue_index,
    selected,
):
    # Every mask is formed from the committed image before any field proposal.
    hit00 = (
        wakeup_valid
        & entries[0].valid
        & ~entries[0].src0_ready
        & (entries[0].src0_tag == wakeup_payload.tag)
    )
    hit01 = (
        wakeup_valid
        & entries[0].valid
        & ~entries[0].src1_ready
        & (entries[0].src1_tag == wakeup_payload.tag)
    )
    hit10 = (
        wakeup_valid
        & entries[1].valid
        & ~entries[1].src0_ready
        & (entries[1].src0_tag == wakeup_payload.tag)
    )
    hit11 = (
        wakeup_valid
        & entries[1].valid
        & ~entries[1].src1_ready
        & (entries[1].src1_tag == wakeup_payload.tag)
    )
    hit20 = (
        wakeup_valid
        & entries[2].valid
        & ~entries[2].src0_ready
        & (entries[2].src0_tag == wakeup_payload.tag)
    )
    hit21 = (
        wakeup_valid
        & entries[2].valid
        & ~entries[2].src1_ready
        & (entries[2].src1_tag == wakeup_payload.tag)
    )
    hit30 = (
        wakeup_valid
        & entries[3].valid
        & ~entries[3].src0_ready
        & (entries[3].src0_tag == wakeup_payload.tag)
    )
    hit31 = (
        wakeup_valid
        & entries[3].valid
        & ~entries[3].src1_ready
        & (entries[3].src1_tag == wakeup_payload.tag)
    )
    capture_allocation = ~allocation_valid & allocation_available
    capture_wakeup = ~wakeup_valid & wakeup_available
    saved_allocation = allocation_payload
    if hit00:
        entries[0].src0_ready = 1
    if hit01:
        entries[0].src1_ready = 1
    if hit10:
        entries[1].src0_ready = 1
    if hit11:
        entries[1].src1_ready = 1
    if hit20:
        entries[2].src0_ready = 1
    if hit21:
        entries[2].src1_ready = 1
    if hit30:
        entries[3].src0_ready = 1
    if hit31:
        entries[3].src1_ready = 1
    if selected:
        entries[issue_index].valid = 0
    if install:
        entries[free_index] = saved_allocation
        allocation_valid = 0
    if wakeup_valid:
        wakeup_valid = 0
    if capture_allocation:
        allocation_valid = 1
        allocation_payload = allocation_head
    if capture_wakeup:
        wakeup_valid = 1
        wakeup_payload = wakeup_head


@module
def issue_core(
    allocation_available: u1,
    allocation_head: Entry,
    wakeup_available: u1,
    wakeup_head: Wakeup,
    issued_space: u1,
) -> IssueResult:
    allocation_valid: u1 = 0
    allocation_payload: Entry = 0
    wakeup_valid: u1 = 0
    wakeup_payload: Wakeup = 0
    entries = table[4, Entry](init=0)
    free_index, free_valid = entries.first(where=lambda entry: not entry.valid)
    issue_index, selected = entries.argmin(
        where=lambda entry: entry.valid and entry.src0_ready and entry.src1_ready,
        key=lambda entry: entry.age,
    )
    issued = entries[issue_index]
    install = allocation_valid & free_valid
    capture_allocation = ~allocation_valid & allocation_available
    capture_wakeup = ~wakeup_valid & wakeup_available
    old_wakeup_valid = wakeup_valid
    update_issue(
        entries,
        allocation_valid,
        allocation_payload,
        wakeup_valid,
        wakeup_payload,
        allocation_available,
        allocation_head,
        wakeup_available,
        wakeup_head,
        free_index,
        install,
        issue_index,
        selected,
    )
    return IssueResult(
        allocation_take=capture_allocation,
        wakeup_take=capture_wakeup,
        issued_valid=selected,
        issued_data=issued,
        install=install,
        wakeup_release=old_wakeup_valid,
        selected=selected,
    )


@module
def issue(
    allocation_valid: u1,
    allocation_data: Entry,
    wakeup_valid: u1,
    wakeup_data: Wakeup,
    issued_take: u1,
) -> {
    "allocation_ready": u1,
    "allocation_available": u1,
    "allocation_head": Entry,
    "wakeup_ready": u1,
    "wakeup_available": u1,
    "wakeup_head": Wakeup,
    "issued_ready": u1,
    "issued_available": u1,
    "issued_head": Entry,
    "allocation_capture": u1,
    "wakeup_capture": u1,
    "allocation_install": u1,
    "wakeup_release": u1,
    "selected": u1,
    "publish": u1,
    "publish_data": Entry,
}:
    allocation_ready, allocation_available, allocation_head = queue[Entry](
        allocation_valid,
        allocation_data,
        core.allocation_take,
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    wakeup_ready, wakeup_available, wakeup_head = queue[Wakeup](
        wakeup_valid,
        wakeup_data,
        core.wakeup_take,
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    issued_ready, issued_available, issued_head = queue[Entry](
        core.issued_valid,
        core.issued_data,
        issued_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    core = issue_core(
        allocation_available,
        allocation_head,
        wakeup_available,
        wakeup_head,
        issued_ready,
    )
    return {
        "allocation_ready": allocation_ready,
        "allocation_available": allocation_available,
        "allocation_head": allocation_head,
        "wakeup_ready": wakeup_ready,
        "wakeup_available": wakeup_available,
        "wakeup_head": wakeup_head,
        "issued_ready": issued_ready,
        "issued_available": issued_available,
        "issued_head": issued_head,
        "allocation_capture": core.allocation_take,
        "wakeup_capture": core.wakeup_take,
        "allocation_install": core.install,
        "wakeup_release": core.wakeup_release,
        "selected": core.selected,
        "publish": core.issued_valid & issued_ready,
        "publish_data": core.issued_data,
    }
