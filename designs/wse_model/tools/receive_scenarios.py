"""External receive stimuli and a test-only finite-resource scheduling adapter.

ReceiveAccount remains the authority for byte accounting, range, commit and
identity semantics. This adapter adds the declared single-context test profile:
slot-ordered early adoption (one per cycle), backpressure and sticky faults.
It is never imported by the generated circuit or its input driver.
"""

from __future__ import annotations

from copy import deepcopy

from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.errors import WseModelError


class ModelCapacityError(RuntimeError):
    """Explicit test configuration cannot hold another counted identity."""


def request(kind: int = 0, **fields) -> dict:
    segment = {
        "opcode": 1,
        "epoch": 1,
        "address": 100,
        "nbytes": 4,
        "source": 1,
        "seq": 0,
        "committed": True,
        "self_sourced": False,
    }
    result = {"kind": kind, "capacity": 16, "expected": 8, "compiled_expected": 8}
    for name, value in fields.items():
        (segment if name in segment else result)[name] = value
    result["segment"] = segment
    return result


def scenario(name: str, requests: list[dict], *, early: int = 2, seen: int = 2) -> dict:
    return {"name": name, "early_slots": early, "seen_slots": seen, "requests": requests}


class ReceiveReference:
    """Timing adapter around the ordinary Python account, not a second counter."""

    def __init__(self, early_slots: int, seen_slots: int):
        self.pending: list[Segment | None] = [None] * early_slots
        self.seen_slots = seen_slots
        self.account: ReceiveAccount | None = None
        self.active = False
        self.fault = False

    def _deliver(self, segment: Segment) -> int:
        assert self.account is not None
        # Validate with the semantic core on an unpublished candidate. Capacity
        # checks can then reject it without copying its arithmetic or priorities.
        candidate = deepcopy(self.account)
        outcome = candidate.deliver(segment)
        if len(candidate.seen) > self.seen_slots:
            raise ModelCapacityError("WSE_MODEL_CAPACITY: dedup identity slots exhausted")
        self.account = candidate
        return {
            DeliveryOutcome.COUNTED: 2,
            DeliveryOutcome.DUPLICATE: 3,
            DeliveryOutcome.SELF_SOURCED: 4,
            DeliveryOutcome.UNCOMMITTED: 5,
        }[outcome]

    def step(self, row: dict) -> dict:
        accepted, status = False, 0
        matching = next(
            (
                i
                for i, s in enumerate(self.pending)
                if s is not None and self.active and s.key == self.account.key
            ),
            None,
        )
        if self.fault:
            status = 9
        elif matching is not None:
            try:
                status = self._deliver(self.pending[matching])
            except WseModelError:
                self.fault, status = True, 9
            # ModelCapacityError escapes before removing the pending entry.
            self.pending[matching] = None
        else:
            fields = row["segment"]
            try:
                if row["kind"] == 1:
                    # Invalid commands fault even when a context is occupied.
                    candidate = ReceiveAccount(
                        aicore=0,
                        opcode=fields["opcode"],
                        epoch=fields["epoch"],
                        dst=fields["address"],
                        capacity=row["capacity"],
                        exp_val=row["expected"],
                        compiled_exp_val=row["compiled_expected"],
                    )
                    if self.active:
                        if candidate.key == self.account.key and not self.account.complete:
                            raise WseModelError("duplicate incomplete receive")
                        status = 8
                    else:
                        self.account, self.active = candidate, True
                        accepted, status = True, 1
                elif row["kind"] == 2:
                    segment = Segment(
                        identity=SegmentId(fields["source"], fields["seq"]),
                        **{
                            k: fields[k]
                            for k in (
                                "opcode",
                                "epoch",
                                "address",
                                "nbytes",
                                "committed",
                                "self_sourced",
                            )
                        },
                    )
                    if self.active and segment.key == self.account.key:
                        status, accepted = self._deliver(segment), True
                    elif None in self.pending:
                        self.pending[self.pending.index(None)] = segment
                        status, accepted = 6, True
                    else:
                        status = 8
                elif row["kind"] == 3:
                    accepted = True
                    if not self.active or not self.account.complete:
                        raise WseModelError("retire requires an active completed account")
                    self.active = False
                    self.account.seen.clear()
                    status = 7
            except WseModelError:
                accepted, self.fault, status = True, True, 9
        return {
            "accepted": accepted,
            "status": status,
            "fault": self.fault,
            "active": self.active,
            "counted": self.account.counted_bytes if self.account else 0,
            "complete": self.active and self.account.complete,
            "pending": sum(s is not None for s in self.pending),
        }


def expected_trace(case: dict) -> dict:
    reference = ReceiveReference(case["early_slots"], case["seen_slots"])
    values = []
    for cycle, row in enumerate(case["requests"]):
        try:
            values.append(reference.step(row))
        except ModelCapacityError as error:
            return {"results": values, "failure_cycle": cycle, "error": str(error)}
    return {"results": values, "failure_cycle": None}


def boundary_scenarios() -> list[dict]:
    post, deliver, idle, retire = 1, 2, 0, 3
    cases = [
        scenario(
            "early-backpressure",
            [
                request(deliver),
                request(deliver, epoch=2),
                request(deliver, source=2),
                request(post),
                request(deliver, source=2),
                request(deliver, source=2),
                request(retire),
                request(post, epoch=2, expected=4, compiled_expected=4),
                request(idle),
                request(retire),
            ],
        ),
        scenario(
            "identity-priorities",
            [
                request(post),
                request(deliver),
                request(deliver),
                request(deliver, source=2, self_sourced=True),
                request(deliver, source=2, committed=False),
                request(deliver, committed=False, self_sourced=True),
                request(deliver, self_sourced=True),
                request(deliver),
                request(retire),
            ],
            seen=1,
        ),
        scenario(
            "capacity-direct", [request(post), request(deliver), request(deliver, source=2)], seen=1
        ),
        scenario(
            "capacity-pending",
            [
                request(deliver),
                request(deliver, source=2),
                request(post),
                request(idle),
                request(idle),
            ],
            seen=1,
        ),
        scenario(
            "context-backpressure",
            [
                request(post),
                request(post, epoch=2),
                request(deliver),
                request(deliver, source=2),
                request(post, epoch=2),
                request(retire),
                request(post, epoch=2),
                request(deliver),
                request(deliver, opcode=2),
                request(deliver, epoch=3),
            ],
        ),
        scenario(
            "zero-and-reuse",
            [
                request(post, expected=0, compiled_expected=0),
                request(retire),
                request(post, epoch=2, expected=4, compiled_expected=4),
                request(deliver, epoch=2),
                request(retire),
                request(post, epoch=3, expected=4, compiled_expected=4),
                request(deliver, epoch=3),
            ],
            seen=1,
        ),
        scenario(
            "byte-overflow",
            [
                request(post, capacity=6, expected=4, compiled_expected=4),
                request(deliver),
                request(deliver, seq=1, address=102),
                request(idle),
            ],
        ),
        scenario(
            "wide-address",
            [
                request(post, address=(1 << 64) - 4, capacity=8),
                request(deliver, address=(1 << 64) - 4, nbytes=8),
            ],
        ),
        scenario(
            "wide-count-overflow",
            [
                request(post, address=0, capacity=(1 << 32) - 1, expected=1, compiled_expected=1),
                request(deliver, address=0, nbytes=(1 << 32) - 2),
                request(deliver, address=0, nbytes=4, seq=1),
            ],
        ),
        scenario(
            "dedup-three-slots",
            [
                request(post, expected=12, compiled_expected=12),
                request(deliver),
                request(deliver, seq=1),
                request(deliver, seq=2),
                request(deliver, seq=1),
                request(retire),
            ],
            early=3,
            seen=3,
        ),
    ]
    cases.append(
        scenario(
            "buffered-range-fault",
            [
                request(deliver, address=99),
                request(deliver, epoch=2),
                request(post),
                request(idle),
                request(deliver),
            ],
        )
    )
    for name, bad in [
        ("post-reserved", request(post, opcode=0)),
        ("post-epoch-zero", request(post, epoch=0)),
        ("post-exp-capacity", request(post, expected=17, compiled_expected=17)),
        ("post-compiled-mismatch", request(post, compiled_expected=9)),
        ("retire-inactive", request(retire)),
        ("deliver-reserved", request(deliver, opcode=7)),
    ]:
        cases.append(scenario(name, [bad, request(post), request(deliver)]))
    for name, bad in [
        ("address-low", request(deliver, address=99)),
        ("address-high-uncommitted", request(deliver, address=114, committed=False)),
        ("duplicate-incomplete-post", request(post)),
        ("retire-incomplete", request(retire)),
    ]:
        cases.append(scenario(name, [request(post), bad, request(deliver)]))
    return cases
