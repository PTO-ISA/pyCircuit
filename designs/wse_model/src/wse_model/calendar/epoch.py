"""Per-round ``collectionEpoch`` accounting.

Calendar §1.5: the report instruction is a pure side effect with no return
value, so the round's epoch cannot come back through alignment. Instead each
core keeps a counter per ``opcode`` domain::

    epoch = CalendarNextEpoch(opcode)   // pure scalar increment, starts at 1

Under SPMD every member of an ``opcode`` domain runs the same ``.text`` and just
rendezvoused at Step2, so the n-th collective in that domain necessarily
produces the same ``n`` on every member — which is exactly why invariant **E1**
("branch participation must agree") exists.

The counter lives on a kernel-local object, so it resets every launch. Isolation
across launches therefore depends entirely on "{SW-1} all Calendar traffic on
the die has drained before the launch boundary", not on the epoch value being
monotonic. Both the width/wraparound policy and the cross-launch initial value
are open items (``C-5``); the model follows the Calendar baseline and makes the
width an explicit parameter.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from wse_model.calendar.collective import OPCODE_RESERVED
from wse_model.errors import WseModelError

__all__ = [
    "EPOCH_TAG_MAX_BITS",
    "EPOCH_TAG_MIN_BITS",
    "EpochCounter",
    "EpochTracker",
    "RecvContext",
]

#: Calendar §1.8 / §6.3 C-5: the header field is proposed at 8-16 bit.
EPOCH_TAG_MIN_BITS = 8
EPOCH_TAG_MAX_BITS = 16


class EpochCounter:
    """The per-core, per-``opcode`` round counter.

    Wraparound is only legal once the previous epoch's traffic has drained
    (``C-5``, contract 7 of §4.5.2). :meth:`next` takes that as an explicit
    precondition rather than assuming it.
    """

    __slots__ = ("_value", "opcode", "width_bits")

    def __init__(self, opcode: int, *, width_bits: int = EPOCH_TAG_MIN_BITS) -> None:
        if opcode in OPCODE_RESERVED:
            raise WseModelError(
                f"opcode {opcode} is reserved and cannot carry a collection epoch (Calendar §2.3)"
            )
        if not EPOCH_TAG_MIN_BITS <= width_bits <= EPOCH_TAG_MAX_BITS:
            raise WseModelError(
                f"epochTag width {width_bits} is outside the proposed "
                f"{EPOCH_TAG_MIN_BITS}-{EPOCH_TAG_MAX_BITS} bit range (C-5)"
            )
        self.opcode = opcode
        self.width_bits = width_bits
        self._value = 0

    @property
    def capacity(self) -> int:
        return (1 << self.width_bits) - 1

    @property
    def current(self) -> int:
        """The current round, or 0 before the first round."""
        return self._value

    def next(self, *, drained: bool = True) -> int:
        """Advance and return the round number, starting at 1."""
        if self._value == 0:
            self._value = 1
            return 1
        if self._value >= self.capacity:
            if not drained:
                raise WseModelError(
                    f"opcode {self.opcode}: epoch counter reached the "
                    f"{self.width_bits} bit capacity {self.capacity} while older "
                    "traffic is still in flight; wraparound requires the previous "
                    "epoch to be drained (Calendar §4.5.2 contract 7, C-5)"
                )
            self._value = 1
            return 1
        self._value += 1
        return self._value

    def describe(self) -> dict[str, object]:
        return {
            "opcode": self.opcode,
            "width_bits": self.width_bits,
            "capacity": self.capacity,
            "value": self._value,
        }


class EpochTracker:
    """All epoch counters of one core, keyed by ``opcode``."""

    __slots__ = ("_counters", "width_bits")

    def __init__(self, *, width_bits: int = EPOCH_TAG_MIN_BITS) -> None:
        self.width_bits = width_bits
        self._counters: dict[int, EpochCounter] = {}

    def counter(self, opcode: int) -> EpochCounter:
        counter = self._counters.get(opcode)
        if counter is None:
            counter = EpochCounter(opcode, width_bits=self.width_bits)
            self._counters[opcode] = counter
        return counter

    def next_epoch(self, opcode: int, *, drained: bool = True) -> int:
        """``CalendarNextEpoch(opcode)``."""
        return self.counter(opcode).next(drained=drained)

    def assert_spmd_consistent(self, opcode: int, peers: dict[int, EpochTracker]) -> int:
        """Verify every peer reports the same epoch for ``opcode``.

        Models invariant E1's consequence: a divergent counter makes receivers
        drop packets under the wrong epoch.
        """
        local = self.counter(opcode).current
        for peer_id, tracker in peers.items():
            remote = tracker.counter(opcode).current
            if remote != local:
                raise WseModelError(
                    f"epoch divergence for opcode {opcode}: core {peer_id} is at "
                    f"epoch {remote} while this core is at {local}; members must "
                    "call the collective the same number of times in the same "
                    "order (invariant E1)"
                )
        return local

    def describe(self) -> dict[str, object]:
        return {key: counter.describe() for key, counter in sorted(self._counters.items())}


@dataclass
class RecvContext:
    """The ``{opcode, epoch}`` context a receive command establishes.

    Calendar §4.5: ``MTE4_NOC_RECV_WAIT`` carries ``opcode`` as a 3 bit immediate
    and ``epoch`` from the core's own counter; the flit ``epochTag`` is taken
    from that context rather than consuming a send encoding slot. The context is
    a per-core, per-round implicit state and must be isolated per AICORE /
    kernel / stream (HW-5), which this dataclass makes explicit.
    """

    opcode: int
    epoch: int
    aicore: int
    kernel_id: str
    stream_id: int = 0
    labels: dict[str, int] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if self.opcode in OPCODE_RESERVED:
            raise WseModelError(
                f"opcode {self.opcode} is reserved and cannot form a receive "
                "context (Calendar §2.3)"
            )
        if self.epoch < 1:
            raise WseModelError(
                f"collection epoch must start at 1, got {self.epoch} (Calendar §1.5)"
            )

    @property
    def key(self) -> tuple[int, int]:
        return (self.opcode, self.epoch)

    def matches(self, other: RecvContext) -> bool:
        """Whether two contexts share the same discrimination key."""
        return self.key == other.key

    def describe(self) -> dict[str, object]:
        return {
            "opcode": self.opcode,
            "epoch": self.epoch,
            "aicore": self.aicore,
            "kernel_id": self.kernel_id,
            "stream_id": self.stream_id,
        }
