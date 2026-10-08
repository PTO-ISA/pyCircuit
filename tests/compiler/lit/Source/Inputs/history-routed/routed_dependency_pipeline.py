"""The full original fourteen-queue, four-route dependency graph."""

# ruff: noqa: F821 -- forward hardware queue and module-result connections.
from history_routed.records import WorkItem
from history_routed.scheduler import dependency
from history_routed.merge import merge
from history_routed.reorder import reorder
from pycircuit import module, queue, rule, struct, u1, u3, u64


@struct
class QueueView:
    ready: u1
    available: u1
    head: WorkItem


@struct
class GraphResult:
    ready: u1
    output_available: u1
    output_head: WorkItem
    scheduled_available: u1
    scheduled_head: WorkItem
    route_1_done_available: u1
    route_1_done_head: WorkItem
    route_3_done_available: u1
    route_3_done_head: WorkItem
    completed_available: u1
    completed_head: WorkItem


@rule
def add_value(item: WorkItem, delta) -> WorkItem:
    extended: u64 = delta
    return WorkItem(
        sequence_id=item.sequence_id,
        opcode=item.opcode,
        route=item.route,
        waits_for=item.waits_for,
        cycles=item.cycles,
        value=item.value + extended,
    )


@module
def frontend(valid: u1, item: WorkItem, take: u1) -> QueueView:
    ready, available, head = queue[WorkItem](
        valid,
        add_value(item, 1),
        take,
        depth=4,
        latency=1,
        ready_policy="local_occupancy",
    )
    return QueueView(ready=ready, available=available, head=head)


@module
def branch(valid: u1, item: WorkItem, take: u1, delta: u64) -> QueueView:
    ready, available, head = queue[WorkItem](
        valid,
        add_value(item, delta),
        take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    return QueueView(ready=ready, available=available, head=head)


@module
def output_stage(valid: u1, item: WorkItem, take: u1) -> QueueView:
    ready, available, head = queue[WorkItem](
        valid,
        add_value(item, 100),
        take,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    return QueueView(ready=ready, available=available, head=head)


@rule
def check_route(available, head: WorkItem):
    if available:
        selector: u3 = head.route
        assert selector < 4, "route_selector_out_of_range"


@module
def pipeline(valid: u1, data: WorkItem, take: u1) -> GraphResult:
    incoming_ready, incoming_available, incoming_head = queue[WorkItem](
        valid, data, prepared.ready, depth=16, latency=1, ready_policy="local_occupancy"
    )
    prepared = frontend(incoming_available, incoming_head, scheduling.take)
    scheduling = dependency(prepared.available, prepared.head, scheduled_ready)
    scheduled_ready, scheduled_available, scheduled_head = queue[WorkItem](
        scheduling.push,
        scheduling.item,
        (scheduled_head.route == 0 and route_0_ready)
        or (scheduled_head.route == 1 and route_1_ready)
        or (scheduled_head.route == 2 and route_2_ready)
        or (scheduled_head.route == 3 and route_3_ready),
        depth=16,
        latency=1,
        ready_policy="local_occupancy",
    )
    check_route(scheduled_available, scheduled_head)

    route_0_ready, route_0_available, route_0_head = queue[WorkItem](
        scheduled_available & (scheduled_head.route == 0),
        scheduled_head,
        route_0_done.ready,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    route_1_ready, route_1_available, route_1_head = queue[WorkItem](
        scheduled_available & (scheduled_head.route == 1),
        scheduled_head,
        route_1_done.ready,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    route_2_ready, route_2_available, route_2_head = queue[WorkItem](
        scheduled_available & (scheduled_head.route == 2),
        scheduled_head,
        route_2_done.ready,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    route_3_ready, route_3_available, route_3_head = queue[WorkItem](
        scheduled_available & (scheduled_head.route == 3),
        scheduled_head,
        route_3_done.ready,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    route_0_done = branch(route_0_available, route_0_head, merging.take0, 1)
    route_1_done = branch(route_1_available, route_1_head, merging.take1, 2)
    route_2_done = branch(route_2_available, route_2_head, merging.take2, 3)
    route_3_done = branch(route_3_available, route_3_head, merging.take3, 4)
    merging = merge(
        route_0_done.available,
        route_0_done.head,
        route_1_done.available,
        route_1_done.head,
        route_2_done.available,
        route_2_done.head,
        route_3_done.available,
        route_3_done.head,
        completed_ready,
    )
    completed_ready, completed_available, completed_head = queue[WorkItem](
        merging.push,
        merging.item,
        ordering.take,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    ordering = reorder(completed_available, completed_head, ordered_ready)
    ordered_ready, ordered_available, ordered_head = queue[WorkItem](
        ordering.push,
        ordering.item,
        output.ready,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    output = output_stage(ordered_available, ordered_head, take)
    return GraphResult(
        ready=incoming_ready,
        output_available=output.available,
        output_head=output.head,
        scheduled_available=scheduled_available,
        scheduled_head=scheduled_head,
        route_1_done_available=route_1_done.available,
        route_1_done_head=route_1_done.head,
        route_3_done_available=route_3_done.available,
        route_3_done_head=route_3_done.head,
        completed_available=completed_available,
        completed_head=completed_head,
    )
