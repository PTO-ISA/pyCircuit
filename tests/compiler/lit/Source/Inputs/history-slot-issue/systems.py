"""Actual queue/slot environments; independent schedules enter through literal frames."""

# ruff: noqa: F821 -- forward hardware connections.
from history_slot_issue.providers import (
    Entry,
    Event,
    Wakeup,
    explicit_mailbox,
    issue,
    nested_mailbox,
)
from pycircuit import log, queue, rule, struct, system, u1, u16


@struct
class MailboxFrame:
    left_valid: u1
    left_data: Event
    left_take: u1
    right_valid: u1
    right_data: Event
    right_take: u1


@struct
class IssueFrame:
    allocation_valid: u1
    allocation_data: Entry
    wakeup_valid: u1
    wakeup_data: Wakeup
    issued_take: u1


@rule
def mailbox_stimulus(epoch: u16) -> MailboxFrame:
    return MailboxFrame(
        left_valid=epoch == 0,
        left_data=Event(value=7),
        left_take=1,
        right_valid=epoch == 0,
        right_data=Event(value=7),
        right_take=1,
    )


@rule
def issue_stimulus(epoch: u16) -> IssueFrame:
    return IssueFrame(
        allocation_valid=epoch == 0,
        allocation_data=Entry(
            valid=1, age=0, src0_tag=7, src0_ready=0, src1_tag=7, src1_ready=0
        ),
        wakeup_valid=epoch == 4,
        wakeup_data=Wakeup(tag=7, valid=1),
        issued_take=1,
    )


@rule
def advance(epoch):
    epoch = epoch + 1


@system
def slot_rule_mailbox():
    epoch: u16 = 0
    frame = mailbox_stimulus(epoch)
    right = nested_mailbox()
    left_in_ready, left_in_available, left_in_head = queue[Event](
        frame.left_valid,
        frame.left_data,
        left.incoming_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_in_ready, right_in_available, right_in_head = queue[Event](
        frame.right_valid,
        frame.right_data,
        right.incoming_take != 0,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_out_ready, left_out_available, left_out_head = queue[Event](
        left.outgoing_valid,
        left.outgoing_data,
        frame.left_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_out_ready, right_out_available, right_out_head = queue[Event](
        right.outgoing_valid != 0,
        right.outgoing_data,
        frame.right_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left = explicit_mailbox(left_in_available, left_in_head, left_out_ready)

    @rule
    def drive_nested():
        right(
            incoming_available=right_in_available,
            incoming_head=right_in_head,
            outgoing_space=right_out_ready,
        )
        assert (right.incoming_take == 0) or (
            right_in_available != 0
        ), "mailbox captures an available input"
        assert (right.outgoing_valid == 0) or (
            right_out_ready != 0
        ), "mailbox releases with accepted output"
        log("info", "right_capture", right.incoming_take)
        log("info", "right_publish", right.outgoing_valid)
        log("info", "right_publish_data", right.outgoing_data.value)
        log("info", "right_in_ready", right_in_ready)
        log("info", "right_in_available", right_in_available)
        log("info", "right_in_head", right_in_head.value)
        log("info", "right_out_ready", right_out_ready)
        log("info", "right_out_available", right_out_available)
        log("info", "right_out_head", right_out_head.value)

    @rule
    def observe_left():
        assert (left.incoming_take == 0) or (
            left_in_available != 0
        ), "mailbox captures an available input"
        assert (left.outgoing_valid == 0) or (
            left_out_ready != 0
        ), "mailbox releases with accepted output"
        log("info", "epoch", epoch)
        log("info", "left_capture", left.incoming_take)
        log("info", "left_publish", left.outgoing_valid)
        log("info", "left_publish_data", left.outgoing_data.value)
        log("info", "left_in_ready", left_in_ready)
        log("info", "left_in_available", left_in_available)
        log("info", "left_in_head", left_in_head.value)
        log("info", "left_out_ready", left_out_ready)
        log("info", "left_out_available", left_out_available)
        log("info", "left_out_head", left_out_head.value)

    drive_nested()
    observe_left()
    advance(epoch)


@system
def resident_issue():
    epoch: u16 = 0
    frame = issue_stimulus(epoch)
    dut = issue()

    @rule
    def drive():
        dut(
            allocation_valid=frame.allocation_valid,
            allocation_data=frame.allocation_data,
            wakeup_valid=frame.wakeup_valid,
            wakeup_data=frame.wakeup_data,
            issued_take=frame.issued_take,
        )
        assert (dut.allocation_capture == 0) or (
            dut.allocation_available != 0
        ), "allocation captures an available input"
        assert (dut.wakeup_capture == 0) or (
            dut.wakeup_available != 0
        ), "wakeup captures an available input"
        assert (dut.publish == 0) or (
            dut.issued_ready != 0
        ), "issued publication is accepted"
        log("info", "epoch", epoch)
        log("info", "allocation_ready", dut.allocation_ready)
        log("info", "allocation_available", dut.allocation_available)
        log("info", "allocation_head", dut.allocation_head)
        log("info", "wakeup_ready", dut.wakeup_ready)
        log("info", "wakeup_available", dut.wakeup_available)
        log("info", "wakeup_head", dut.wakeup_head)
        log("info", "issued_ready", dut.issued_ready)
        log("info", "issued_available", dut.issued_available)
        log("info", "issued_head", dut.issued_head)
        log("info", "allocation_capture", dut.allocation_capture)
        log("info", "wakeup_capture", dut.wakeup_capture)
        log("info", "allocation_install", dut.allocation_install)
        log("info", "wakeup_release", dut.wakeup_release)
        log("info", "selected", dut.selected)
        log("info", "publish", dut.publish)
        log("info", "publish_data", dut.publish_data)

    drive()
    advance(epoch)
