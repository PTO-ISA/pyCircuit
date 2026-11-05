"""The receive side: byte accounting against ``expVal``.

Calendar §4.5 defines the one genuinely new instruction of the design,
``MTE4_NOC_RECV_WAIT(dst, capacity, expVal, opcode, epoch)``. It **moves no
bytes**: the payload is written into the core's arena directly by the NoC, and
MTE4 only *accounts*. A receive command retires once the valid payload bytes
that match ``{opcode, epoch, dstRange}`` and are already committed reach
``expVal``.

That turns "the collective is complete" from a distributed protocol problem into
a local counter problem. The price is that the accounting must be exact, so this
module implements the seven P0 contracts of §4.5.2 as explicit, fail-closed
checks:

1. the unit is valid payload **bytes** — no header, CRC, padding, retransmit
   copy, or self-sourced byte counts;
2. only writes inside ``[dst, dst + capacity)`` matching the current
   ``{opcode, epoch}`` count; anything else is discarded **and faults**;
3. accounting happens after the payload is committed and readable — MTE4
   completion is therefore an acquire;
4. legal arrivals that predate the command but belong to the same established
   epoch must not be missed;
5. duplicate packets must not be double counted, including multicast forked
   copies per land point;
6. ``expVal == 0`` completes immediately; ``expVal > capacity``, a mismatch with
   the compiled value, or counter overflow faults — never a silent permanent
   block;
7. because ``opcode`` is a weak discriminator (every AllGather shares
   ``opcode = 1``), ``collectionEpoch`` carries the entire discrimination duty.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass, field
from enum import Enum
from typing import NamedTuple

from wse_model.calendar.collective import OPCODE_RESERVED
from wse_model.errors import ReceiveContractError, WseModelError

__all__ = [
    "CoreIngress",
    "DeliveryOutcome",
    "ReceiveAccount",
    "Segment",
    "SegmentId",
]


class SegmentId(NamedTuple):
    """Identity of one payload segment, used for de-duplication (contract 5).

    The design requires packet/segment identity de-duplication, with multicast
    forked copies de-duplicated **per land point**. Since a receive account is
    per core, the account's seen-set is exactly the per-land-point scope.
    """

    source: int
    seq: int


@dataclass(frozen=True)
class Segment:
    """A unit of payload delivered by the NoC."""

    identity: SegmentId
    opcode: int
    epoch: int
    address: int
    nbytes: int
    #: Set for the source's own contribution, which never counts (contract 1).
    self_sourced: bool = False
    #: Contract 3: accounting happens only once the payload is committed and
    #: readable by this core's successors, which is what makes MTE4 completion an
    #: acquire. A write that has reached the arena but not committed is not
    #: countable yet and must not be dropped either.
    committed: bool = True

    def committed_copy(self) -> Segment:
        """The same segment, marked committed."""
        from dataclasses import replace

        return replace(self, committed=True)

    def __post_init__(self) -> None:
        if self.nbytes < 0:
            raise WseModelError(f"segment {self.identity} has negative length")
        if self.opcode in OPCODE_RESERVED:
            raise WseModelError(
                f"segment {self.identity} carries reserved opcode {self.opcode}; "
                "the receive side must fault rather than count it (Calendar §2.3)"
            )

    @property
    def key(self) -> tuple[int, int]:
        return (self.opcode, self.epoch)

    @property
    def end_address(self) -> int:
        return self.address + self.nbytes


class DeliveryOutcome(str, Enum):
    """What the ingress ledger did with a delivery."""

    COUNTED = "counted"
    DUPLICATE = "duplicate"
    SELF_SOURCED = "self-sourced"
    DEFERRED = "deferred"
    """Buffered because no matching receive command has been posted yet
    (contract 4)."""

    UNCOMMITTED = "uncommitted"
    """Arrived but not yet committed, so not countable (contract 3). The delivery
    must be re-presented after the commit, not silently dropped."""


@dataclass
class ReceiveAccount:
    """One in-flight ``MTE4_NOC_RECV_WAIT`` command."""

    aicore: int
    opcode: int
    epoch: int
    dst: int
    capacity: int
    exp_val: int
    #: The value the compiler derived from geometry, checked for consistency
    #: (Calendar §2.8: user value == geometry value == table expectedRxBytes).
    compiled_exp_val: int | None = None
    seen: set[SegmentId] = field(default_factory=set)
    counted_bytes: int = 0

    def __post_init__(self) -> None:
        if self.opcode in OPCODE_RESERVED:
            raise ReceiveContractError(
                2,
                f"core {self.aicore} posted a receive for reserved opcode "
                f"{self.opcode}; it must fault (Calendar §2.3)",
            )
        if self.epoch < 1:
            raise ReceiveContractError(
                7,
                f"core {self.aicore} posted a receive with epoch {self.epoch}; "
                "collection epochs start at 1 (Calendar §1.5)",
            )
        if self.exp_val < 0 or self.capacity < 0:
            raise ReceiveContractError(
                6, f"core {self.aicore}: expVal and capacity must be non-negative"
            )
        if self.exp_val > self.capacity:
            raise ReceiveContractError(
                6,
                f"core {self.aicore}: expVal {self.exp_val} exceeds capacity "
                f"{self.capacity}; an unsatisfiable wait must fault, not block "
                "forever",
            )
        if self.compiled_exp_val is not None and self.compiled_exp_val != self.exp_val:
            raise ReceiveContractError(
                6,
                f"core {self.aicore}: expVal {self.exp_val} disagrees with the "
                f"compiled value {self.compiled_exp_val} (Calendar §2.8)",
            )

    @property
    def key(self) -> tuple[int, int]:
        return (self.opcode, self.epoch)

    @property
    def dst_range(self) -> tuple[int, int]:
        return (self.dst, self.dst + self.capacity)

    @property
    def remaining(self) -> int:
        return max(0, self.exp_val - self.counted_bytes)

    @property
    def complete(self) -> bool:
        return self.counted_bytes >= self.exp_val

    def _in_range(self, segment: Segment) -> bool:
        return self.dst <= segment.address and segment.end_address <= self.dst + self.capacity

    def deliver(self, segment: Segment) -> DeliveryOutcome:
        """Account one delivered segment against this command."""
        if segment.key != self.key:
            raise ReceiveContractError(
                2,
                f"core {self.aicore}: segment {segment.identity} carries "
                f"{{opcode {segment.opcode}, epoch {segment.epoch}}} but the "
                f"active receive expects {{opcode {self.opcode}, epoch "
                f"{self.epoch}}}; a mismatched arrival is discarded and faults",
            )
        if not self._in_range(segment):
            raise ReceiveContractError(
                2,
                f"core {self.aicore}: segment {segment.identity} spans "
                f"[{segment.address}, {segment.end_address}) which leaves the "
                f"receive range [{self.dst}, {self.dst + self.capacity})",
            )
        if not segment.committed:
            # Contract 3: measure only committed payload. The segment is not
            # counted and not de-duplicated yet, so its commit can count it once.
            return DeliveryOutcome.UNCOMMITTED
        if segment.self_sourced:
            return DeliveryOutcome.SELF_SOURCED
        if segment.identity in self.seen:
            return DeliveryOutcome.DUPLICATE
        counted_bytes = self.counted_bytes + segment.nbytes
        if counted_bytes > self.capacity:
            raise ReceiveContractError(
                6,
                f"core {self.aicore}: counted {counted_bytes} bytes exceeds "
                f"capacity {self.capacity}; the counter overflowed, which must "
                "fault rather than silently satisfy expVal",
            )
        # A rejected segment must neither consume its identity nor change the
        # byte count. Publish both changes only after validating the candidate.
        self.seen.add(segment.identity)
        self.counted_bytes = counted_bytes
        return DeliveryOutcome.COUNTED

    def describe(self) -> dict[str, object]:
        return {
            "aicore": self.aicore,
            "opcode": self.opcode,
            "epoch": self.epoch,
            "dst": self.dst,
            "capacity": self.capacity,
            "expVal": self.exp_val,
            "counted_bytes": self.counted_bytes,
            "remaining": self.remaining,
            "complete": self.complete,
            "segments": len(self.seen),
        }


class CoreIngress:
    """A core's ingress ledger: it makes early arrivals countable.

    Contract 4 requires that a legal arrival which happens before the receive
    command executes is still accounted, because Step3 posts the command after
    the group aligns while the NoC may already be delivering. The ledger buffers
    arrivals per ``{opcode, epoch}`` and adopts them when the matching command is
    posted.
    """

    __slots__ = ("_accounts", "_pending", "aicore")

    def __init__(self, aicore: int) -> None:
        self.aicore = aicore
        self._accounts: dict[tuple[int, int], ReceiveAccount] = {}
        self._pending: dict[tuple[int, int], list[Segment]] = {}

    @property
    def pending(self) -> dict[tuple[int, int], tuple[Segment, ...]]:
        return {key: tuple(items) for key, items in self._pending.items()}

    @property
    def outstanding(self) -> tuple[ReceiveAccount, ...]:
        return tuple(self._accounts.values())

    def post(self, account: ReceiveAccount) -> ReceiveAccount:
        """Post a receive command and adopt any matching early arrivals."""
        if account.aicore != self.aicore:
            raise WseModelError(
                f"account for core {account.aicore} posted to core {self.aicore} ingress"
            )
        existing = self._accounts.get(account.key)
        if existing is not None and not existing.complete:
            raise ReceiveContractError(
                7,
                f"core {self.aicore}: two active receives for opcode "
                f"{account.opcode} epoch {account.epoch}; the epoch must be unique "
                "within its opcode domain",
            )
        self._accounts[account.key] = account
        for segment in self._pending.pop(account.key, []):
            account.deliver(segment)
        return account

    def retire(self, account: ReceiveAccount) -> None:
        """Remove a completed command."""
        if not account.complete:
            raise WseModelError(
                f"core {self.aicore}: cannot retire an incomplete receive for "
                f"opcode {account.opcode} epoch {account.epoch} "
                f"({account.remaining} bytes still expected)"
            )
        self._accounts.pop(account.key, None)

    def deliver(self, segment: Segment) -> DeliveryOutcome:
        """Deliver a segment from the NoC into this core's arena."""
        account = self._accounts.get(segment.key)
        if account is None:
            self._pending.setdefault(segment.key, []).append(segment)
            return DeliveryOutcome.DEFERRED
        return account.deliver(segment)

    def deliver_all(self, segments: Iterable[Segment]) -> list[DeliveryOutcome]:
        return [self.deliver(segment) for segment in segments]

    def commit(self, segment: Segment) -> DeliveryOutcome:
        """Re-present a segment once its write has committed (contract 3).

        Before this the delivery returns :attr:`DeliveryOutcome.UNCOMMITTED` and
        counts nothing; afterwards it counts exactly once, because an uncommitted
        delivery never entered the de-duplication set.
        """
        return self.deliver(segment.committed_copy())

    def describe(self) -> dict[str, object]:
        return {
            "aicore": self.aicore,
            "outstanding": [account.describe() for account in self.outstanding],
            "pending": {
                f"{opcode}:{epoch}": len(items) for (opcode, epoch), items in self._pending.items()
            },
        }
