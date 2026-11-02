# ruff: noqa: F823 -- captured module-owned state proposals.
"""Shared word arithmetic with explicit queue timing and atomic commit."""

import pycircuit as ac

from functional_float.word_arithmetic import WordResult, word_arithmetic


@ac.struct
class Request:
    tag: ac.u4
    enable: ac.u1
    lhs: ac.u64
    rhs: ac.u64
    operation: ac.u4


@ac.struct
class Response:
    tag: ac.u4
    valid: ac.u1
    invalid: ac.u1
    value: ac.u64


@ac.struct
class WorkerState:
    total: ac.u64
    completed: ac.u8
    rejected: ac.u8


@ac.module
def WordWorker():  # noqa: N802
    total: ac.u64 = 0
    completed: ac.u8 = 0
    rejected: ac.u8 = 0

    @ac.method
    def process(request: Request) -> Response:
        result = word_arithmetic(request.lhs, request.rhs, request.operation, 0, 1)
        if result.valid:
            total = total + result.result
        else:
            rejected = rejected + 1
        completed = completed + 1
        return Response(
            tag=request.tag,
            valid=result.valid,
            invalid=~result.valid,
            value=result.result if result.valid else 0,
        )

    @ac.method
    def snapshot() -> WorkerState:
        return WorkerState(total=total, completed=completed, rejected=rejected)


@ac.module(
    execution={
        "worker": "worker",
        "ingress": "ingress",
        "egress": "egress",
        "process": "process",
        "snapshot": "snapshot",
        "submit": "submit",
        "take": "take",
        "transfer": "transfer",
        "enabled": "enable",
        "valid": "valid",
        "accepted": "accepted",
        "interval": 1,
        "wait_state": "",
    }
)
def TimedWordPipeline():  # noqa: N802
    """Depth-2 ingress, latency-1 admission, latency-3 output, II=1."""

    ingress = ac.queue[Request](depth=2, latency=1)
    egress = ac.queue[Response](depth=2, latency=3)
    worker = WordWorker()
    accepted: ac.u8 = 0

    @ac.method
    def submit(request: Request) -> ac.u1:
        if request.enable:
            ingress.push(request)
            accepted = accepted + 1
        return 1

    @ac.method
    def take() -> Response:
        return egress.pop()

    @ac.method
    def snapshot() -> tuple[ac.u8, ac.u64, ac.u8, ac.u8]:
        state = worker.snapshot()
        return accepted, state.total, state.completed, state.rejected

    @ac.rule
    def transfer() -> None:
        request = ingress.pop()
        response = worker.process(request)
        if response.valid:
            egress.push(response)


@ac.module(
    execution={
        "worker": "worker",
        "ingress": "ingress",
        "egress": "egress",
        "process": "process",
        "snapshot": "snapshot",
        "submit": "submit",
        "take": "take",
        "transfer": "transfer",
        "enabled": "enable",
        "valid": "valid",
        "accepted": "accepted",
        "interval": 2,
        "wait_state": "issue_gap",
    }
)
def TimedWordPipelineAlt():  # noqa: N802
    """Depth-1/latency-1 profile with an explicit II=2 issue gap."""

    ingress = ac.queue[Request](depth=1, latency=1)
    egress = ac.queue[Response](depth=1, latency=1)
    worker = WordWorker()
    accepted: ac.u8 = 0
    issue_gap: ac.u1 = 0

    @ac.method
    def submit(request: Request) -> ac.u1:
        if request.enable:
            ingress.push(request)
            accepted = accepted + 1
        return 1

    @ac.method
    def take() -> Response:
        return egress.pop()

    @ac.method
    def snapshot() -> tuple[ac.u8, ac.u64, ac.u8, ac.u8]:
        state = worker.snapshot()
        return accepted, state.total, state.completed, state.rejected

    @ac.rule
    def transfer() -> None:
        if issue_gap:
            issue_gap = 0
        else:
            request = ingress.pop()
            response = worker.process(request)
            if response.valid:
                egress.push(response)
            issue_gap = 1
