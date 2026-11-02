# ruff: noqa: F823 -- captured module-owned state proposals.
"""A different payload, operation, state dependency and transport profile."""

import pycircuit as ac


@ac.struct
class DigestRequest:
    ticket: ac.u3
    live: ac.u1
    value: ac.u16
    mask: ac.u16


@ac.struct
class DigestReply:
    ticket: ac.u3
    ok: ac.u1
    fault: ac.u1
    payload: ac.u16


@ac.struct
class DigestState:
    digest: ac.u16
    completed: ac.u8
    rejected: ac.u8


@ac.module
def DigestWorker():  # noqa: N802
    digest: ac.u16 = 0
    completed: ac.u8 = 0
    rejected: ac.u8 = 0

    @ac.method
    def apply(request: DigestRequest) -> DigestReply:
        ok: ac.u1 = request.mask != 0
        value: ac.u16 = (request.value ^ request.mask) + digest
        if ok:
            digest = value
        else:
            rejected = rejected + 1
        completed = completed + 1
        return DigestReply(
            ticket=request.ticket, ok=ok, fault=~ok, payload=value if ok else 0
        )

    @ac.method
    def state() -> DigestState:
        return DigestState(digest=digest, completed=completed, rejected=rejected)


@ac.module(
    execution={
        "worker": "engine",
        "ingress": "pending",
        "egress": "results",
        "process": "apply",
        "snapshot": "state",
        "submit": "offer",
        "take": "read",
        "transfer": "advance",
        "enabled": "live",
        "valid": "ok",
        "accepted": "sent",
        "interval": 1,
        "wait_state": "",
    }
)
def TimedDigestPipeline():  # noqa: N802
    pending = ac.queue[DigestRequest](depth=1, latency=2)
    results = ac.queue[DigestReply](depth=2, latency=1)
    engine = DigestWorker()
    sent: ac.u8 = 0

    @ac.method
    def offer(request: DigestRequest) -> ac.u1:
        if request.live:
            pending.push(request)
            sent = sent + 1
        return 1

    @ac.method
    def read() -> DigestReply:
        return results.pop()

    @ac.method
    def state() -> tuple[ac.u8, ac.u16, ac.u8, ac.u8]:
        snapshot = engine.state()
        return sent, snapshot.digest, snapshot.completed, snapshot.rejected

    @ac.rule
    def advance() -> None:
        request = pending.pop()
        result = engine.apply(request)
        if result.ok:
            results.push(result)
