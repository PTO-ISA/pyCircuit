"""The AICORE pipe set and the ordering rules Calendar depends on.

Whitepaper §3 keeps Davinci's pipeline organisation. Calendar §4.7 adds two
requirements that are correctness, not performance:

* ``PIPE_MTE3`` (send) and ``PIPE_MTE4`` (receive accounting) must dispatch,
  back-pressure, and barrier **independently**. If an unsatisfied ``MTE4`` wait
  could stall AICORE issue or block ``MTE3``, every member would sit waiting for
  its own receive while nobody sends — a group-wide deadlock (invariant O3).
* ``pipe_barrier(PIPE_MTE3)`` must wait for the packets to have actually been
  sent. It must not return early merely because ``CalReg[opcode]`` has not
  released the packets yet.

The model expresses those as a small ordering machine so a scenario can prove it
does not deadlock, rather than asserting it in prose.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

from wse_model.errors import WseModelError

__all__ = [
    "INDEPENDENT_FROM_MTE4",
    "PIPE_ORDER_NOTE",
    "Pipe",
    "PipeBarrier",
    "PipeTracker",
    "StepRequirement",
    "calendar_step_pipes",
]


class Pipe(str, Enum):
    """The pipes the design names (whitepaper §9.1, Calendar §4.1)."""

    SCALAR = "PIPE_S"
    MTE1 = "PIPE_MTE1"
    MTE2 = "PIPE_MTE2"
    MTE3 = "PIPE_MTE3"
    MTE4 = "PIPE_MTE4"
    CUBE = "PIPE_M"
    VECTOR = "PIPE_V"
    FIXPIPE = "PIPE_FIX"

    @property
    def is_memory(self) -> bool:
        return self in (Pipe.MTE1, Pipe.MTE2, Pipe.MTE3, Pipe.MTE4)

    @property
    def is_compute(self) -> bool:
        return self in (Pipe.CUBE, Pipe.VECTOR, Pipe.FIXPIPE)


#: Pipes that must keep making progress while ``PIPE_MTE4`` is unsatisfied.
INDEPENDENT_FROM_MTE4 = (Pipe.SCALAR, Pipe.MTE3)

PIPE_ORDER_NOTE = (
    "Calendar §4.7: MTE3 is the existing inter-core write path and MTE4 only "
    "accounts. They must be independently dispatchable, back-pressurable, and "
    "barrier-able, or the collective deadlocks."
)


@dataclass(frozen=True)
class StepRequirement:
    """The pipes one Calendar step needs, and what it waits for."""

    step: str
    requires: tuple[Pipe, ...]
    blocks: tuple[Pipe, ...] = ()
    note: str = ""

    def __post_init__(self) -> None:
        if not self.step:
            raise WseModelError("StepRequirement.step must be non-empty")

    def describe(self) -> dict[str, object]:
        return {
            "step": self.step,
            "requires": [pipe.value for pipe in self.requires],
            "blocks": [pipe.value for pipe in self.blocks],
            "note": self.note,
        }


#: Steps Step0..Step6 mapped onto pipes, from Calendar §1.2 and §1.7.
_CALENDAR_STEPS: tuple[StepRequirement, ...] = (
    StepRequirement(
        "Step0 MatMul",
        requires=(Pipe.CUBE, Pipe.VECTOR),
        note="the caller's producer, not part of the collective",
    ),
    StepRequirement(
        "Step1 Barrier",
        requires=(Pipe.SCALAR,),
        blocks=(Pipe.CUBE, Pipe.VECTOR, Pipe.MTE1, Pipe.MTE3, Pipe.MTE4),
        note="TSYNC(events) + pipe_barrier(PIPE_ALL) + dsb(DSB_DDR)",
    ),
    StepRequirement(
        "Step2 Align",
        requires=(Pipe.SCALAR,),
        blocks=(Pipe.SCALAR,),
        note=(
            "SET_CROSS_CORE(calGroup, opcode) + WAIT_FLAG_DEV(opcode); reuses the "
            "existing ISA, so the control plane adds zero instructions"
        ),
    ),
    StepRequirement(
        "Step3 MTE4 RECV",
        requires=(Pipe.MTE4,),
        blocks=(),
        note=(
            "MTE4_NOC_RECV_WAIT: establishes the round's {opcode, epoch} context "
            "and accounts bytes; it must not block issue or MTE3"
        ),
    ),
    StepRequirement(
        "Step4 MTE3 SEND",
        requires=(Pipe.MTE3,),
        blocks=(),
        note="MTE3_NOC_SEND carrying {routeBits80, opcode}; released by CalReg[opcode]",
    ),
    StepRequirement(
        "Step5 Join",
        requires=(Pipe.MTE3, Pipe.MTE4),
        blocks=(Pipe.MTE3, Pipe.MTE4),
        note="drain the sends first, then wait for the MTE4 account to retire (O3)",
    ),
    StepRequirement(
        "Step6 next",
        requires=(Pipe.SCALAR,),
        note="return RecordEvent; the instruction body issues no set_flag",
    ),
)


def calendar_step_pipes() -> tuple[StepRequirement, ...]:
    """The Step0..Step6 pipe requirements."""
    return _CALENDAR_STEPS


@dataclass
class PipeTracker:
    """A crude per-pipe progress model, enough to detect the O3 deadlock.

    Each pipe holds a count of outstanding commands. ``issue`` refuses when the
    pipe is not independently dispatchable, and :meth:`check_no_deadlock`
    verifies that an unsatisfied ``MTE4`` wait leaves ``SCALAR`` and ``MTE3``
    free.
    """

    outstanding: dict[Pipe, int] = field(default_factory=dict)
    mte4_waiting: bool = False

    def issue(self, pipe: Pipe, count: int = 1) -> None:
        if count < 0:
            raise WseModelError("count must be non-negative")
        if self.mte4_waiting and pipe not in INDEPENDENT_FROM_MTE4:
            raise WseModelError(
                f"{pipe.value} cannot dispatch while PIPE_MTE4 is unsatisfied; "
                "only SCALAR and MTE3 may, or the collective deadlocks "
                "(Calendar §4.7, invariant O3)"
            )
        self.outstanding[pipe] = self.outstanding.get(pipe, 0) + count

    def retire(self, pipe: Pipe, count: int = 1) -> None:
        current = self.outstanding.get(pipe, 0)
        if count > current:
            raise WseModelError(
                f"{pipe.value}: cannot retire {count} of {current} outstanding commands"
            )
        self.outstanding[pipe] = current - count

    def begin_mte4_wait(self) -> None:
        self.mte4_waiting = True

    def end_mte4_wait(self) -> None:
        self.mte4_waiting = False

    @property
    def drained(self) -> bool:
        return all(count == 0 for count in self.outstanding.values())

    def dispatchable(self) -> tuple[Pipe, ...]:
        """The pipes that may dispatch in the current state.

        While an ``MTE4`` wait is outstanding, only the pipes the design exempts
        remain dispatchable. That set must never be empty, or the wait could
        deadlock the collective (Calendar §4.7, invariant O3).
        """
        if not self.mte4_waiting:
            return tuple(Pipe)
        return INDEPENDENT_FROM_MTE4

    def check_no_deadlock(self) -> None:
        """Verify the MTE4 wait leaves a live send path."""
        if not self.mte4_waiting:
            return
        allowed = self.dispatchable()
        if Pipe.MTE3 not in allowed:
            raise WseModelError(
                "an unsatisfied PIPE_MTE4 wait blocked PIPE_MTE3; every member "
                "would wait for its own receive while nobody sends, so the "
                "collective deadlocks (Calendar §4.7, invariant O3)"
            )
        if Pipe.SCALAR not in allowed:
            raise WseModelError(
                "an unsatisfied PIPE_MTE4 wait blocked scalar issue, so the core "
                "could not even enqueue the send (Calendar §4.7, invariant O3)"
            )

    def describe(self) -> dict[str, object]:
        return {
            "outstanding": {
                pipe.value: count
                for pipe, count in sorted(self.outstanding.items(), key=lambda item: item[0].value)
            },
            "mte4_waiting": self.mte4_waiting,
            "drained": self.drained,
            "independent_from_mte4": [pipe.value for pipe in INDEPENDENT_FROM_MTE4],
        }


@dataclass(frozen=True)
class PipeBarrier:
    """A barrier over one pipe, with the "actually sent" requirement.

    Calendar §4.7 item 4: the Step5 barrier on ``PIPE_MTE3`` must wait until the
    queued sends have physically been emitted. Returning early because the NoC
    has not released them yet would let the core observe an incomplete collective
    as complete.
    """

    pipe: Pipe
    waits_for_actual_send: bool = True

    def __post_init__(self) -> None:
        if self.pipe is not Pipe.MTE3 and self.waits_for_actual_send:
            raise WseModelError(
                "only a PIPE_MTE3 barrier has the 'must wait for the actual send' "
                "requirement (Calendar §4.7 item 4)"
            )

    def admits_release(self, *, sent: int, queued: int) -> bool:
        """Whether the barrier may return, given sent and still-queued sends."""
        if queued < sent:
            raise WseModelError("queued sends cannot be fewer than sent sends")
        return sent == queued

    def describe(self) -> dict[str, object]:
        return {
            "pipe": self.pipe.value,
            "waits_for_actual_send": self.waits_for_actual_send,
        }
