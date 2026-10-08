"""Real depth-one queue systems; the original committed-host ISQ trace is separate."""

# ruff: noqa: F821 -- forward hardware queue/Core connections.
from pycircuit import log, queue, rule, struct, system, u1, u16
from history_resident.reusable_circular_rob import RobEvent, rob
from history_resident.reusable_oldest_ready_isq import IssueEntry, Readiness, isq


@struct
class RobFrame:
    left_flush_valid: u1
    left_flush_data: RobEvent
    left_allocate_valid: u1
    left_allocate_data: RobEvent
    left_completion_valid: u1
    left_completion_data: RobEvent
    right_flush_valid: u1
    right_flush_data: RobEvent
    right_allocate_valid: u1
    right_allocate_data: RobEvent
    right_completion_valid: u1
    right_completion_data: RobEvent
    left_allocated_take: u1
    left_retired_take: u1
    right_allocated_take: u1
    right_retired_take: u1


@rule
def rob_stimulus(epoch: u16) -> RobFrame:
    left_flush_valid: u1 = 0
    left_flush_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    left_allocate_valid: u1 = 0
    left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    left_completion_valid: u1 = 0
    left_completion_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    right_flush_valid: u1 = 0
    right_flush_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    right_allocate_valid: u1 = 0
    right_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    right_completion_valid: u1 = 0
    right_completion_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    left_allocated_take: u1 = 0
    left_retired_take: u1 = 0
    right_allocated_take: u1 = 0
    right_retired_take: u1 = 0
    if epoch == 0:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=10, done=0)
        right_allocate_valid = 1
        right_allocate_data = RobEvent(
            index=0, generation=0, epoch=0, value=100, done=0
        )
    if epoch == 2:
        right_allocated_take = 1
    if epoch == 3:
        right_completion_valid = 1
        right_completion_data = RobEvent(
            index=0, generation=1, epoch=0, value=0, done=0
        )
    if epoch == 6:
        right_retired_take = 1
    if epoch == 7:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=20, done=0)
    if epoch == 9:
        left_allocated_take = 1
    if epoch == 11:
        left_allocated_take = 1
    if epoch == 12:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=30, done=0)
    if epoch == 14:
        left_allocated_take = 1
    if epoch == 15:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=40, done=0)
    if epoch == 17:
        left_allocated_take = 1
    if epoch == 18:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=50, done=0)
    if epoch == 20:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=2, generation=1, epoch=0, value=0, done=0)
    if epoch == 22:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=0, generation=1, epoch=0, value=0, done=0)
    if epoch == 26:
        left_allocated_take = 1
    if epoch == 27:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=0, generation=1, epoch=0, value=0, done=0)
    if epoch == 29:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=3, generation=1, epoch=0, value=0, done=0)
    if epoch == 31:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=1, generation=1, epoch=0, value=0, done=0)
    if epoch == 33:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=0, generation=2, epoch=0, value=0, done=0)
    if epoch == 38:
        left_retired_take = 1
    if epoch == 40:
        left_retired_take = 1
    if epoch == 42:
        left_retired_take = 1
    if epoch == 44:
        left_retired_take = 1
    if epoch == 46:
        left_retired_take = 1
    if epoch == 47:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=60, done=0)
    if epoch == 49:
        left_allocated_take = 1
    if epoch == 50:
        left_flush_valid = 1
        left_flush_data = RobEvent(index=0, generation=0, epoch=0, value=0, done=0)
    if epoch == 52:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=1, generation=2, epoch=0, value=0, done=0)
    if epoch == 54:
        left_allocate_valid = 1
        left_allocate_data = RobEvent(index=0, generation=0, epoch=0, value=70, done=0)
    if epoch == 56:
        left_allocated_take = 1
    if epoch == 57:
        left_completion_valid = 1
        left_completion_data = RobEvent(index=2, generation=2, epoch=1, value=0, done=0)
    if epoch == 60:
        left_retired_take = 1
    return RobFrame(
        left_flush_valid=left_flush_valid,
        left_flush_data=left_flush_data,
        left_allocate_valid=left_allocate_valid,
        left_allocate_data=left_allocate_data,
        left_completion_valid=left_completion_valid,
        left_completion_data=left_completion_data,
        right_flush_valid=right_flush_valid,
        right_flush_data=right_flush_data,
        right_allocate_valid=right_allocate_valid,
        right_allocate_data=right_allocate_data,
        right_completion_valid=right_completion_valid,
        right_completion_data=right_completion_data,
        left_allocated_take=left_allocated_take,
        left_retired_take=left_retired_take,
        right_allocated_take=right_allocated_take,
        right_retired_take=right_retired_take,
    )


@rule
def rob_observe(
    epoch,
    frame,
    left_flush_ready,
    left_flush_available,
    left_flush_head,
    left_allocate_ready,
    left_allocate_available,
    left_allocate_head,
    left_completion_ready,
    left_completion_available,
    left_completion_head,
    right_flush_ready,
    right_flush_available,
    right_flush_head,
    right_allocate_ready,
    right_allocate_available,
    right_allocate_head,
    right_completion_ready,
    right_completion_available,
    right_completion_head,
    left_allocated_ready,
    left_allocated_available,
    left_allocated_head,
    left_retired_ready,
    left_retired_available,
    left_retired_head,
    right_allocated_ready,
    right_allocated_available,
    right_allocated_head,
    right_retired_ready,
    right_retired_available,
    right_retired_head,
    left,
    right,
):
    assert not frame.left_flush_valid or left_flush_ready, "input offer accepted"
    assert not frame.left_allocate_valid or left_allocate_ready, "input offer accepted"
    assert (
        not frame.left_completion_valid or left_completion_ready
    ), "input offer accepted"
    assert not frame.right_flush_valid or right_flush_ready, "input offer accepted"
    assert (
        not frame.right_allocate_valid or right_allocate_ready
    ), "input offer accepted"
    assert (
        not frame.right_completion_valid or right_completion_ready
    ), "input offer accepted"
    assert (
        not frame.left_allocated_take or left_allocated_available
    ), "output take available"
    assert (
        not frame.left_retired_take or left_retired_available
    ), "output take available"
    assert (
        not frame.right_allocated_take or right_allocated_available
    ), "output take available"
    assert (
        not frame.right_retired_take or right_retired_available
    ), "output take available"
    log("info", "epoch", epoch)
    log("info", "left_flush_available", left_flush_available)
    log("info", "left_flush_ready", left_flush_ready)
    log(
        "info",
        "left_flush_head_index",
        left_flush_head.index if left_flush_available else 0,
    )
    log(
        "info",
        "left_flush_head_generation",
        left_flush_head.generation if left_flush_available else 0,
    )
    log(
        "info",
        "left_flush_head_epoch",
        left_flush_head.epoch if left_flush_available else 0,
    )
    log(
        "info",
        "left_flush_head_value",
        left_flush_head.value if left_flush_available else 0,
    )
    log(
        "info",
        "left_flush_head_done",
        left_flush_head.done if left_flush_available else 0,
    )
    log("info", "left_allocate_available", left_allocate_available)
    log("info", "left_allocate_ready", left_allocate_ready)
    log(
        "info",
        "left_allocate_head_index",
        left_allocate_head.index if left_allocate_available else 0,
    )
    log(
        "info",
        "left_allocate_head_generation",
        left_allocate_head.generation if left_allocate_available else 0,
    )
    log(
        "info",
        "left_allocate_head_epoch",
        left_allocate_head.epoch if left_allocate_available else 0,
    )
    log(
        "info",
        "left_allocate_head_value",
        left_allocate_head.value if left_allocate_available else 0,
    )
    log(
        "info",
        "left_allocate_head_done",
        left_allocate_head.done if left_allocate_available else 0,
    )
    log("info", "left_completion_available", left_completion_available)
    log("info", "left_completion_ready", left_completion_ready)
    log(
        "info",
        "left_completion_head_index",
        left_completion_head.index if left_completion_available else 0,
    )
    log(
        "info",
        "left_completion_head_generation",
        left_completion_head.generation if left_completion_available else 0,
    )
    log(
        "info",
        "left_completion_head_epoch",
        left_completion_head.epoch if left_completion_available else 0,
    )
    log(
        "info",
        "left_completion_head_value",
        left_completion_head.value if left_completion_available else 0,
    )
    log(
        "info",
        "left_completion_head_done",
        left_completion_head.done if left_completion_available else 0,
    )
    log("info", "right_flush_available", right_flush_available)
    log("info", "right_flush_ready", right_flush_ready)
    log(
        "info",
        "right_flush_head_index",
        right_flush_head.index if right_flush_available else 0,
    )
    log(
        "info",
        "right_flush_head_generation",
        right_flush_head.generation if right_flush_available else 0,
    )
    log(
        "info",
        "right_flush_head_epoch",
        right_flush_head.epoch if right_flush_available else 0,
    )
    log(
        "info",
        "right_flush_head_value",
        right_flush_head.value if right_flush_available else 0,
    )
    log(
        "info",
        "right_flush_head_done",
        right_flush_head.done if right_flush_available else 0,
    )
    log("info", "right_allocate_available", right_allocate_available)
    log("info", "right_allocate_ready", right_allocate_ready)
    log(
        "info",
        "right_allocate_head_index",
        right_allocate_head.index if right_allocate_available else 0,
    )
    log(
        "info",
        "right_allocate_head_generation",
        right_allocate_head.generation if right_allocate_available else 0,
    )
    log(
        "info",
        "right_allocate_head_epoch",
        right_allocate_head.epoch if right_allocate_available else 0,
    )
    log(
        "info",
        "right_allocate_head_value",
        right_allocate_head.value if right_allocate_available else 0,
    )
    log(
        "info",
        "right_allocate_head_done",
        right_allocate_head.done if right_allocate_available else 0,
    )
    log("info", "right_completion_available", right_completion_available)
    log("info", "right_completion_ready", right_completion_ready)
    log(
        "info",
        "right_completion_head_index",
        right_completion_head.index if right_completion_available else 0,
    )
    log(
        "info",
        "right_completion_head_generation",
        right_completion_head.generation if right_completion_available else 0,
    )
    log(
        "info",
        "right_completion_head_epoch",
        right_completion_head.epoch if right_completion_available else 0,
    )
    log(
        "info",
        "right_completion_head_value",
        right_completion_head.value if right_completion_available else 0,
    )
    log(
        "info",
        "right_completion_head_done",
        right_completion_head.done if right_completion_available else 0,
    )
    log("info", "left_allocated_available", left_allocated_available)
    log("info", "left_allocated_ready", left_allocated_ready)
    log(
        "info",
        "left_allocated_head_index",
        left_allocated_head.index if left_allocated_available else 0,
    )
    log(
        "info",
        "left_allocated_head_generation",
        left_allocated_head.generation if left_allocated_available else 0,
    )
    log(
        "info",
        "left_allocated_head_epoch",
        left_allocated_head.epoch if left_allocated_available else 0,
    )
    log(
        "info",
        "left_allocated_head_value",
        left_allocated_head.value if left_allocated_available else 0,
    )
    log(
        "info",
        "left_allocated_head_done",
        left_allocated_head.done if left_allocated_available else 0,
    )
    log("info", "left_retired_available", left_retired_available)
    log("info", "left_retired_ready", left_retired_ready)
    log(
        "info",
        "left_retired_head_index",
        left_retired_head.index if left_retired_available else 0,
    )
    log(
        "info",
        "left_retired_head_generation",
        left_retired_head.generation if left_retired_available else 0,
    )
    log(
        "info",
        "left_retired_head_epoch",
        left_retired_head.epoch if left_retired_available else 0,
    )
    log(
        "info",
        "left_retired_head_value",
        left_retired_head.value if left_retired_available else 0,
    )
    log(
        "info",
        "left_retired_head_done",
        left_retired_head.done if left_retired_available else 0,
    )
    log("info", "right_allocated_available", right_allocated_available)
    log("info", "right_allocated_ready", right_allocated_ready)
    log(
        "info",
        "right_allocated_head_index",
        right_allocated_head.index if right_allocated_available else 0,
    )
    log(
        "info",
        "right_allocated_head_generation",
        right_allocated_head.generation if right_allocated_available else 0,
    )
    log(
        "info",
        "right_allocated_head_epoch",
        right_allocated_head.epoch if right_allocated_available else 0,
    )
    log(
        "info",
        "right_allocated_head_value",
        right_allocated_head.value if right_allocated_available else 0,
    )
    log(
        "info",
        "right_allocated_head_done",
        right_allocated_head.done if right_allocated_available else 0,
    )
    log("info", "right_retired_available", right_retired_available)
    log("info", "right_retired_ready", right_retired_ready)
    log(
        "info",
        "right_retired_head_index",
        right_retired_head.index if right_retired_available else 0,
    )
    log(
        "info",
        "right_retired_head_generation",
        right_retired_head.generation if right_retired_available else 0,
    )
    log(
        "info",
        "right_retired_head_epoch",
        right_retired_head.epoch if right_retired_available else 0,
    )
    log(
        "info",
        "right_retired_head_value",
        right_retired_head.value if right_retired_available else 0,
    )
    log(
        "info",
        "right_retired_head_done",
        right_retired_head.done if right_retired_available else 0,
    )
    log("info", "left_flush_take", left.flush_take)
    log("info", "left_allocate_take", left.allocate_take)
    log("info", "left_completion_take", left.completion_take)
    log("info", "left_allocated_valid", left.allocated_valid)
    log("info", "left_retired_valid", left.retired_valid)
    log("info", "right_flush_take", right.flush_take)
    log("info", "right_allocate_take", right.allocate_take)
    log("info", "right_completion_take", right.completion_take)
    log("info", "right_allocated_valid", right.allocated_valid)
    log("info", "right_retired_valid", right.retired_valid)
    log(
        "info",
        "left_allocated_publish_index",
        left.allocated_data.index if left.allocated_valid else 0,
    )
    log(
        "info",
        "left_allocated_publish_generation",
        left.allocated_data.generation if left.allocated_valid else 0,
    )
    log(
        "info",
        "left_allocated_publish_epoch",
        left.allocated_data.epoch if left.allocated_valid else 0,
    )
    log(
        "info",
        "left_allocated_publish_value",
        left.allocated_data.value if left.allocated_valid else 0,
    )
    log(
        "info",
        "left_allocated_publish_done",
        left.allocated_data.done if left.allocated_valid else 0,
    )
    log(
        "info",
        "left_retired_publish_index",
        left.retired_data.index if left.retired_valid else 0,
    )
    log(
        "info",
        "left_retired_publish_generation",
        left.retired_data.generation if left.retired_valid else 0,
    )
    log(
        "info",
        "left_retired_publish_epoch",
        left.retired_data.epoch if left.retired_valid else 0,
    )
    log(
        "info",
        "left_retired_publish_value",
        left.retired_data.value if left.retired_valid else 0,
    )
    log(
        "info",
        "left_retired_publish_done",
        left.retired_data.done if left.retired_valid else 0,
    )
    log(
        "info",
        "right_allocated_publish_index",
        right.allocated_data.index if right.allocated_valid else 0,
    )
    log(
        "info",
        "right_allocated_publish_generation",
        right.allocated_data.generation if right.allocated_valid else 0,
    )
    log(
        "info",
        "right_allocated_publish_epoch",
        right.allocated_data.epoch if right.allocated_valid else 0,
    )
    log(
        "info",
        "right_allocated_publish_value",
        right.allocated_data.value if right.allocated_valid else 0,
    )
    log(
        "info",
        "right_allocated_publish_done",
        right.allocated_data.done if right.allocated_valid else 0,
    )
    log(
        "info",
        "right_retired_publish_index",
        right.retired_data.index if right.retired_valid else 0,
    )
    log(
        "info",
        "right_retired_publish_generation",
        right.retired_data.generation if right.retired_valid else 0,
    )
    log(
        "info",
        "right_retired_publish_epoch",
        right.retired_data.epoch if right.retired_valid else 0,
    )
    log(
        "info",
        "right_retired_publish_value",
        right.retired_data.value if right.retired_valid else 0,
    )
    log(
        "info",
        "right_retired_publish_done",
        right.retired_data.done if right.retired_valid else 0,
    )
    epoch = epoch + 1


@system
def reusable_circular_rob():
    epoch: u16 = 0
    frame = rob_stimulus(epoch)
    left_flush_ready, left_flush_available, left_flush_head = queue[RobEvent](
        frame.left_flush_valid,
        frame.left_flush_data,
        left.flush_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_allocate_ready, left_allocate_available, left_allocate_head = queue[RobEvent](
        frame.left_allocate_valid,
        frame.left_allocate_data,
        left.allocate_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_completion_ready, left_completion_available, left_completion_head = queue[
        RobEvent
    ](
        frame.left_completion_valid,
        frame.left_completion_data,
        left.completion_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_flush_ready, right_flush_available, right_flush_head = queue[RobEvent](
        frame.right_flush_valid,
        frame.right_flush_data,
        right.flush_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_allocate_ready, right_allocate_available, right_allocate_head = queue[
        RobEvent
    ](
        frame.right_allocate_valid,
        frame.right_allocate_data,
        right.allocate_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_completion_ready, right_completion_available, right_completion_head = queue[
        RobEvent
    ](
        frame.right_completion_valid,
        frame.right_completion_data,
        right.completion_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_allocated_ready, left_allocated_available, left_allocated_head = queue[
        RobEvent
    ](
        left.allocated_valid,
        left.allocated_data,
        frame.left_allocated_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_retired_ready, left_retired_available, left_retired_head = queue[RobEvent](
        left.retired_valid,
        left.retired_data,
        frame.left_retired_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_allocated_ready, right_allocated_available, right_allocated_head = queue[
        RobEvent
    ](
        right.allocated_valid,
        right.allocated_data,
        frame.right_allocated_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_retired_ready, right_retired_available, right_retired_head = queue[RobEvent](
        right.retired_valid,
        right.retired_data,
        frame.right_retired_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left = rob(
        left_flush_available,
        left_flush_head,
        left_allocate_available,
        left_allocate_head,
        left_completion_available,
        left_completion_head,
        left_allocated_ready,
        left_retired_ready,
    )
    right = rob(
        right_flush_available,
        right_flush_head,
        right_allocate_available,
        right_allocate_head,
        right_completion_available,
        right_completion_head,
        right_allocated_ready,
        right_retired_ready,
    )
    rob_observe(
        epoch,
        frame,
        left_flush_ready,
        left_flush_available,
        left_flush_head,
        left_allocate_ready,
        left_allocate_available,
        left_allocate_head,
        left_completion_ready,
        left_completion_available,
        left_completion_head,
        right_flush_ready,
        right_flush_available,
        right_flush_head,
        right_allocate_ready,
        right_allocate_available,
        right_allocate_head,
        right_completion_ready,
        right_completion_available,
        right_completion_head,
        left_allocated_ready,
        left_allocated_available,
        left_allocated_head,
        left_retired_ready,
        left_retired_available,
        left_retired_head,
        right_allocated_ready,
        right_allocated_available,
        right_allocated_head,
        right_retired_ready,
        right_retired_available,
        right_retired_head,
        left,
        right,
    )


@struct
class IsqFrame:
    left_request_valid: u1
    left_request_data: IssueEntry
    left_readiness_valid: u1
    left_readiness_data: Readiness
    right_request_valid: u1
    right_request_data: IssueEntry
    right_readiness_valid: u1
    right_readiness_data: Readiness
    left_issued_take: u1
    right_issued_take: u1


@rule
def isq_stimulus(epoch: u16) -> IsqFrame:
    left_request_valid: u1 = 0
    left_request_data = IssueEntry(
        index=0, age=0, src0_tag=0, src1_tag=0, value=0, valid=0
    )
    left_readiness_valid: u1 = 0
    left_readiness_data = Readiness(tag=0, ready=0)
    right_request_valid: u1 = 0
    right_request_data = IssueEntry(
        index=0, age=0, src0_tag=0, src1_tag=0, value=0, valid=0
    )
    right_readiness_valid: u1 = 0
    right_readiness_data = Readiness(tag=0, ready=0)
    left_issued_take: u1 = 0
    right_issued_take: u1 = 0
    if epoch == 0:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=1, ready=1)
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=1, src0_tag=1, src1_tag=1, value=100, valid=0
        )
    if epoch == 3:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=5, ready=1)
    if epoch == 5:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=3, src0_tag=5, src1_tag=5, value=300, valid=0
        )
    if epoch == 7:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=9, ready=1)
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=4, src0_tag=9, src1_tag=9, value=400, valid=0
        )
    if epoch == 9:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=2, src0_tag=7, src1_tag=7, value=200, valid=0
        )
    if epoch == 13:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=7, ready=1)
    if epoch == 15:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=11, ready=1)
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=5, src0_tag=11, src1_tag=11, value=500, valid=0
        )
    if epoch == 20:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=6, src0_tag=1, src1_tag=1, value=600, valid=0
        )
    if epoch == 23:
        right_request_valid = 1
        right_request_data = IssueEntry(
            index=0, age=1, src0_tag=5, src1_tag=5, value=900, valid=0
        )
    if epoch == 27:
        right_readiness_valid = 1
        right_readiness_data = Readiness(tag=5, ready=1)
    if epoch == 30:
        right_issued_take = 1
    if epoch == 31:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=7, ready=0)
        left_issued_take = 1
    if epoch == 33:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=7, ready=1)
    if epoch == 37:
        left_issued_take = 1
    if epoch == 39:
        left_issued_take = 1
    if epoch == 41:
        left_issued_take = 1
    if epoch == 43:
        left_issued_take = 1
    if epoch == 45:
        left_issued_take = 1
    if epoch == 46:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=12, ready=1)
    if epoch == 48:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=12, ready=0)
    if epoch == 50:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=7, src0_tag=12, src1_tag=12, value=700, valid=0
        )
    if epoch == 54:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=12, ready=1)
    if epoch == 57:
        left_issued_take = 1
    if epoch == 58:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=20, ready=1)
    if epoch == 60:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=20, src0_tag=20, src1_tag=20, value=800, valid=0
        )
    if epoch == 62:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=21, ready=1)
    if epoch == 64:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=21, src0_tag=21, src1_tag=21, value=801, valid=0
        )
    if epoch == 66:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=22, ready=1)
    if epoch == 68:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=22, src0_tag=22, src1_tag=22, value=802, valid=0
        )
    if epoch == 70:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=63, ready=1)
    if epoch == 72:
        left_request_valid = 1
        left_request_data = IssueEntry(
            index=0, age=23, src0_tag=63, src1_tag=63, value=803, valid=0
        )
    if epoch == 74:
        left_issued_take = 1
    if epoch == 75:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=40, ready=0)
    if epoch == 76:
        left_issued_take = 1
    if epoch == 77:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=41, ready=1)
    if epoch == 78:
        left_issued_take = 1
    if epoch == 79:
        left_readiness_valid = 1
        left_readiness_data = Readiness(tag=42, ready=0)
    if epoch == 80:
        left_issued_take = 1
    return IsqFrame(
        left_request_valid=left_request_valid,
        left_request_data=left_request_data,
        left_readiness_valid=left_readiness_valid,
        left_readiness_data=left_readiness_data,
        right_request_valid=right_request_valid,
        right_request_data=right_request_data,
        right_readiness_valid=right_readiness_valid,
        right_readiness_data=right_readiness_data,
        left_issued_take=left_issued_take,
        right_issued_take=right_issued_take,
    )


@rule
def isq_observe(
    epoch,
    frame,
    left_request_ready,
    left_request_available,
    left_request_head,
    left_readiness_ready,
    left_readiness_available,
    left_readiness_head,
    right_request_ready,
    right_request_available,
    right_request_head,
    right_readiness_ready,
    right_readiness_available,
    right_readiness_head,
    left_issued_ready,
    left_issued_available,
    left_issued_head,
    right_issued_ready,
    right_issued_available,
    right_issued_head,
    left,
    right,
):
    assert not frame.left_request_valid or left_request_ready, "input offer accepted"
    assert (
        not frame.left_readiness_valid or left_readiness_ready
    ), "input offer accepted"
    assert not frame.right_request_valid or right_request_ready, "input offer accepted"
    assert (
        not frame.right_readiness_valid or right_readiness_ready
    ), "input offer accepted"
    assert not frame.left_issued_take or left_issued_available, "output take available"
    assert (
        not frame.right_issued_take or right_issued_available
    ), "output take available"
    log("info", "epoch", epoch)
    log("info", "left_request_available", left_request_available)
    log("info", "left_request_ready", left_request_ready)
    log(
        "info",
        "left_request_head_index",
        left_request_head.index if left_request_available else 0,
    )
    log(
        "info",
        "left_request_head_age",
        left_request_head.age if left_request_available else 0,
    )
    log(
        "info",
        "left_request_head_src0_tag",
        left_request_head.src0_tag if left_request_available else 0,
    )
    log(
        "info",
        "left_request_head_src1_tag",
        left_request_head.src1_tag if left_request_available else 0,
    )
    log(
        "info",
        "left_request_head_value",
        left_request_head.value if left_request_available else 0,
    )
    log(
        "info",
        "left_request_head_valid",
        left_request_head.valid if left_request_available else 0,
    )
    log("info", "left_readiness_available", left_readiness_available)
    log("info", "left_readiness_ready", left_readiness_ready)
    log(
        "info",
        "left_readiness_head_tag",
        left_readiness_head.tag if left_readiness_available else 0,
    )
    log(
        "info",
        "left_readiness_head_ready",
        left_readiness_head.ready if left_readiness_available else 0,
    )
    log("info", "right_request_available", right_request_available)
    log("info", "right_request_ready", right_request_ready)
    log(
        "info",
        "right_request_head_index",
        right_request_head.index if right_request_available else 0,
    )
    log(
        "info",
        "right_request_head_age",
        right_request_head.age if right_request_available else 0,
    )
    log(
        "info",
        "right_request_head_src0_tag",
        right_request_head.src0_tag if right_request_available else 0,
    )
    log(
        "info",
        "right_request_head_src1_tag",
        right_request_head.src1_tag if right_request_available else 0,
    )
    log(
        "info",
        "right_request_head_value",
        right_request_head.value if right_request_available else 0,
    )
    log(
        "info",
        "right_request_head_valid",
        right_request_head.valid if right_request_available else 0,
    )
    log("info", "right_readiness_available", right_readiness_available)
    log("info", "right_readiness_ready", right_readiness_ready)
    log(
        "info",
        "right_readiness_head_tag",
        right_readiness_head.tag if right_readiness_available else 0,
    )
    log(
        "info",
        "right_readiness_head_ready",
        right_readiness_head.ready if right_readiness_available else 0,
    )
    log("info", "left_issued_available", left_issued_available)
    log("info", "left_issued_ready", left_issued_ready)
    log(
        "info",
        "left_issued_head_index",
        left_issued_head.index if left_issued_available else 0,
    )
    log(
        "info",
        "left_issued_head_age",
        left_issued_head.age if left_issued_available else 0,
    )
    log(
        "info",
        "left_issued_head_src0_tag",
        left_issued_head.src0_tag if left_issued_available else 0,
    )
    log(
        "info",
        "left_issued_head_src1_tag",
        left_issued_head.src1_tag if left_issued_available else 0,
    )
    log(
        "info",
        "left_issued_head_value",
        left_issued_head.value if left_issued_available else 0,
    )
    log(
        "info",
        "left_issued_head_valid",
        left_issued_head.valid if left_issued_available else 0,
    )
    log("info", "right_issued_available", right_issued_available)
    log("info", "right_issued_ready", right_issued_ready)
    log(
        "info",
        "right_issued_head_index",
        right_issued_head.index if right_issued_available else 0,
    )
    log(
        "info",
        "right_issued_head_age",
        right_issued_head.age if right_issued_available else 0,
    )
    log(
        "info",
        "right_issued_head_src0_tag",
        right_issued_head.src0_tag if right_issued_available else 0,
    )
    log(
        "info",
        "right_issued_head_src1_tag",
        right_issued_head.src1_tag if right_issued_available else 0,
    )
    log(
        "info",
        "right_issued_head_value",
        right_issued_head.value if right_issued_available else 0,
    )
    log(
        "info",
        "right_issued_head_valid",
        right_issued_head.valid if right_issued_available else 0,
    )
    log("info", "left_request_take", left.request_take)
    log("info", "left_readiness_take", left.readiness_take)
    log("info", "left_issued_valid", left.issued_valid)
    log("info", "right_request_take", right.request_take)
    log("info", "right_readiness_take", right.readiness_take)
    log("info", "right_issued_valid", right.issued_valid)
    log(
        "info",
        "left_issued_publish_index",
        left.issued_data.index if left.issued_valid else 0,
    )
    log(
        "info",
        "left_issued_publish_age",
        left.issued_data.age if left.issued_valid else 0,
    )
    log(
        "info",
        "left_issued_publish_src0_tag",
        left.issued_data.src0_tag if left.issued_valid else 0,
    )
    log(
        "info",
        "left_issued_publish_src1_tag",
        left.issued_data.src1_tag if left.issued_valid else 0,
    )
    log(
        "info",
        "left_issued_publish_value",
        left.issued_data.value if left.issued_valid else 0,
    )
    log(
        "info",
        "left_issued_publish_valid",
        left.issued_data.valid if left.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_index",
        right.issued_data.index if right.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_age",
        right.issued_data.age if right.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_src0_tag",
        right.issued_data.src0_tag if right.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_src1_tag",
        right.issued_data.src1_tag if right.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_value",
        right.issued_data.value if right.issued_valid else 0,
    )
    log(
        "info",
        "right_issued_publish_valid",
        right.issued_data.valid if right.issued_valid else 0,
    )
    epoch = epoch + 1


@system
def reusable_oldest_ready_isq():
    epoch: u16 = 0
    frame = isq_stimulus(epoch)
    left_request_ready, left_request_available, left_request_head = queue[IssueEntry](
        frame.left_request_valid,
        frame.left_request_data,
        left.request_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_readiness_ready, left_readiness_available, left_readiness_head = queue[
        Readiness
    ](
        frame.left_readiness_valid,
        frame.left_readiness_data,
        left.readiness_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_request_ready, right_request_available, right_request_head = queue[
        IssueEntry
    ](
        frame.right_request_valid,
        frame.right_request_data,
        right.request_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_readiness_ready, right_readiness_available, right_readiness_head = queue[
        Readiness
    ](
        frame.right_readiness_valid,
        frame.right_readiness_data,
        right.readiness_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_issued_ready, left_issued_available, left_issued_head = queue[IssueEntry](
        left.issued_valid,
        left.issued_data,
        frame.left_issued_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_issued_ready, right_issued_available, right_issued_head = queue[IssueEntry](
        right.issued_valid,
        right.issued_data,
        frame.right_issued_take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    left = isq(
        left_request_available,
        left_request_head,
        left_readiness_available,
        left_readiness_head,
        left_issued_ready,
    )
    right = isq(
        right_request_available,
        right_request_head,
        right_readiness_available,
        right_readiness_head,
        right_issued_ready,
    )
    isq_observe(
        epoch,
        frame,
        left_request_ready,
        left_request_available,
        left_request_head,
        left_readiness_ready,
        left_readiness_available,
        left_readiness_head,
        right_request_ready,
        right_request_available,
        right_request_head,
        right_readiness_ready,
        right_readiness_available,
        right_readiness_head,
        left_issued_ready,
        left_issued_available,
        left_issued_head,
        right_issued_ready,
        right_issued_available,
        right_issued_head,
        left,
        right,
    )
