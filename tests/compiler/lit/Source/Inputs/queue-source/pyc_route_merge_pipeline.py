"""pyc_route_merge_pipeline: hardware fixture; migration notes in tests/compiler/oracles/queue_source/MIGRATION-NOTES.md."""

# ruff: noqa: F821, N802 -- ordinary forward hardware queue-result wires.
import pycircuit as ac


@ac.struct
class RouteMergeResult:
    ready: ac.u1
    available: ac.u1
    head: ac.u64


@ac.rule
def CheckRoute(available, head):  # noqa: N802
    if available:
        assert head == 0 or head == 1, "route_selector_out_of_range"


@ac.module
def RouteMergePipeline(  # noqa: N802
    valid: ac.u1, data: ac.u64, take: ac.u1
) -> RouteMergeResult:
    CheckRoute(available, head)
    ready, available, head = ac.queue[ac.u64](
        valid,
        data,
        available and ((head == 0 and left_ready) or (head == 1 and right_ready)),
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    is_left = head == 0
    is_right = head == 1
    left_ready, left_valid, left_head = ac.queue[ac.u64](
        available and is_left,
        head,
        left_done_ready,
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_ready, right_valid, right_head = ac.queue[ac.u64](
        available and is_right,
        head,
        right_done_ready,
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    left_done_ready, left_done_valid, left_done_head = ac.queue[ac.u64](
        left_valid,
        left_head + 10,
        merged_ready,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    right_done_ready, right_done_valid, right_done_head = ac.queue[ac.u64](
        right_valid,
        right_head + 20,
        merged_ready and not left_done_valid,
        depth=1,
        latency=1,
        ready_policy="local_occupancy",
    )
    merged_ready, merged_valid, merged_head = ac.queue[ac.u64](
        left_done_valid or right_done_valid,
        left_done_head if left_done_valid else right_done_head,
        take,
        depth=2,
        latency=1,
        ready_policy="local_occupancy",
    )
    return RouteMergeResult(ready=ready, available=merged_valid, head=merged_head)
