"""Two independent instances of each resident core, with borrowed typed channels."""

from history_resident.reusable_circular_rob import RobEvent, RobResult, rob
from history_resident.reusable_oldest_ready_isq import (
    IsqResult,
    IssueEntry,
    Readiness,
    isq,
)
from pycircuit import module, struct, u1


@struct
class DualRobResult:
    left: RobResult
    right: RobResult


@struct
class DualIsqResult:
    left: IsqResult
    right: IsqResult


@module
def dual_rob(
    left_flush_available: u1,
    left_flush_request: RobEvent,
    left_allocate_available: u1,
    left_allocate_request: RobEvent,
    left_completion_available: u1,
    left_completion: RobEvent,
    left_allocated_space: u1,
    left_retired_space: u1,
    right_flush_available: u1,
    right_flush_request: RobEvent,
    right_allocate_available: u1,
    right_allocate_request: RobEvent,
    right_completion_available: u1,
    right_completion: RobEvent,
    right_allocated_space: u1,
    right_retired_space: u1,
) -> DualRobResult:
    left = rob(
        left_flush_available,
        left_flush_request,
        left_allocate_available,
        left_allocate_request,
        left_completion_available,
        left_completion,
        left_allocated_space,
        left_retired_space,
    )
    right = rob(
        right_flush_available,
        right_flush_request,
        right_allocate_available,
        right_allocate_request,
        right_completion_available,
        right_completion,
        right_allocated_space,
        right_retired_space,
    )
    return DualRobResult(left=left, right=right)


@module
def dual_isq(
    left_request_available: u1,
    left_request: IssueEntry,
    left_readiness_available: u1,
    left_readiness: Readiness,
    left_issued_space: u1,
    right_request_available: u1,
    right_request: IssueEntry,
    right_readiness_available: u1,
    right_readiness: Readiness,
    right_issued_space: u1,
) -> DualIsqResult:
    left = isq(
        left_request_available,
        left_request,
        left_readiness_available,
        left_readiness,
        left_issued_space,
    )
    right = isq(
        right_request_available,
        right_request,
        right_readiness_available,
        right_readiness,
        right_issued_space,
    )
    return DualIsqResult(left=left, right=right)
