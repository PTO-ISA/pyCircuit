"""Collective semantics, ``opcode``, and ``redOp``.

Calendar §2.3 fixes ``opcode`` at **3 bit**, with the value determined by the
collective semantics alone. ``opcode`` carries two responsibilities:

1. it indexes the resident timeslot register ``CalReg[opcode]``, and
2. it doubles as the **alignment semaphore id** — ``opcode in 1..6`` is a subset
   of the existing ``0..15`` id space, which is what makes "alignment adds no
   ISA" true (Calendar §1.5, §4.2).

It does **not** carry the reduction operator; ``redOp`` is a separate 3 bit flit
header field (Calendar §2.4).

The numeric codes for :class:`RedOp` are not fixed by the design sources. They
are assigned here in the order the design document lists them, and every use of
a reduction operator is gated on open item ``C-10`` ("complete definition of
reduction semantics under ``{P, L}``"), which also covers the missing element
type field of open item ``C-3``.
"""

from __future__ import annotations

from enum import IntEnum

from wse_model.errors import WseModelError

__all__ = [
    "ALIGNMENT_SEMAPHORE_WIDTH_BITS",
    "OPCODE_RESERVED",
    "OPCODE_WIDTH_BITS",
    "RED_OP_WIDTH_BITS",
    "AlignmentDomain",
    "Collective",
    "RedOp",
    "ReductionElementType",
]

#: ``opcode`` is 3 bit so the six collectives fit with room for illegal codes.
OPCODE_WIDTH_BITS = 3

#: ``redOp`` is an independent 3 bit flit header field (Calendar §2.4).
RED_OP_WIDTH_BITS = 3

#: Calendar §4.2: ``opcode`` is reused as the alignment semaphore id, whose
#: existing field is 4 bit (0..15). HW-7 requires at least 6 bit for a 32-core
#: FFN phase B or a 40-core full-die collective.
ALIGNMENT_SEMAPHORE_WIDTH_BITS = 4

#: Calendar §2.3: codes 0 and 7 are reserved; 0 makes an all-zero flit header fault.
OPCODE_RESERVED = frozenset({0, 7})


class Collective(IntEnum):
    """The six collective semantics and their ``opcode`` values (Calendar §2.3)."""

    INVALID = 0
    """Reserved: entering the Calendar data plane must fault."""

    ALL_GATHER = 1
    REDUCE = 2
    ALL_REDUCE = 3
    REDUCE_SCATTER = 4
    SCATTER = 5
    GATHER = 6

    EXTENSION = 7
    """Reserved for extension: must fault or be dropped and reported."""

    @property
    def is_implemented(self) -> bool:
        """First phase implements ``AllGather`` and ``Reduce`` only."""
        return self in (Collective.ALL_GATHER, Collective.REDUCE)

    @property
    def is_reduction(self) -> bool:
        return self in (
            Collective.REDUCE,
            Collective.ALL_REDUCE,
            Collective.REDUCE_SCATTER,
        )

    @property
    def is_reserved(self) -> bool:
        return self in (Collective.INVALID, Collective.EXTENSION)

    @property
    def opcode(self) -> int:
        return int(self)

    @classmethod
    def from_opcode(cls, opcode: int) -> Collective:
        if not 0 <= opcode < (1 << OPCODE_WIDTH_BITS):
            raise WseModelError(f"opcode {opcode} does not fit in {OPCODE_WIDTH_BITS} bits")
        return cls(opcode)


class RedOp(IntEnum):
    """Reduction operators carried in the flit header (Calendar §2.4).

    ``None`` is the value for the pure-copy collectives, where the field is
    dormant. The numeric assignment follows the order the design document lists
    the operators in, because the document does not fix the codes.
    """

    NONE = 0
    SUM = 1
    MAX = 2
    MIN = 3
    PROD = 4

    @property
    def is_active(self) -> bool:
        return self is not RedOp.NONE


class ReductionElementType:
    """The 3 bit reduction element type required by open item ``C-3``.

    Calendar §2.4: ``redOp`` alone cannot determine the machine operation —
    ``f32`` SUM and ``s32`` SUM are different operations, as are ``f16`` and
    ``b16`` MAX. The header needs a 3 bit element type taking seven of the
    existing ``vtype_t`` values, excluding ``fmix``.

    The design source fixes neither which seven values nor how they are
    numbered, so this type **carries** the raw code without interpreting it.
    Asking for a name resolves through open item ``C-3`` and therefore fails
    until a decision record picks an encoding.
    """

    WIDTH_BITS = RED_OP_WIDTH_BITS

    __slots__ = ("code",)

    def __init__(self, code: int) -> None:
        if not 0 <= code < (1 << RED_OP_WIDTH_BITS):
            raise WseModelError(
                f"reduction element type code {code} does not fit in {RED_OP_WIDTH_BITS} bits"
            )
        self.code = code

    @property
    def name(self) -> str:
        """The ``vtype_t`` name, which requires open item ``C-3`` to be resolved."""
        from wse_model.open_items import require_resolved

        require_resolved("C-3", consumer="ReductionElementType.name")
        raise AssertionError("unreachable: C-3 is unresolved")  # pragma: no cover

    def __eq__(self, other: object) -> bool:
        if isinstance(other, ReductionElementType):
            return self.code == other.code
        if isinstance(other, int):
            return self.code == other
        return NotImplemented

    def __hash__(self) -> int:
        return hash(("ReductionElementType", self.code))

    def __repr__(self) -> str:  # pragma: no cover - trivial
        return f"ReductionElementType(code={self.code})"


class AlignmentDomain:
    """The alignment semaphore domain for one ``opcode``.

    Calendar §1.5 is explicit that the alignment domain is partitioned **by
    ``opcode``, not by group**: every core in a launch that uses the same
    ``opcode`` rendezvouses on the same semaphore, so an FFN phase B is 32
    cores reporting at once, not four independent groups of eight.

    This class only models the arithmetic of the arrival count; it does not
    schedule.
    """

    __slots__ = ("_arrivals", "_width_bits", "opcode")

    def __init__(self, opcode: int, *, width_bits: int = ALIGNMENT_SEMAPHORE_WIDTH_BITS) -> None:
        if opcode in OPCODE_RESERVED:
            raise WseModelError(
                f"opcode {opcode} is reserved and cannot form an alignment domain (Calendar §2.3)"
            )
        self.opcode = opcode
        self._width_bits = width_bits
        self._arrivals = 0

    @property
    def capacity(self) -> int:
        """Largest arrival count the semaphore can represent."""
        return (1 << self._width_bits) - 1

    @property
    def arrivals(self) -> int:
        return self._arrivals

    @property
    def required_width_bits(self) -> int:
        """Minimum semaphore width for the current member count (HW-7)."""
        return self.required_width_for(self._arrivals)

    @staticmethod
    def required_width_for(members: int) -> int:
        """``ceil(log2(members + 1))``: the width needed to count ``members``.

        HW-7: an FFN phase B rendezvous is 32 cores and a full-die collective is
        40, both of which need at least 6 bit against the existing 4 bit field.
        """
        if members < 0:
            raise WseModelError(f"member count must be non-negative, got {members}")
        return max(1, members.bit_length())

    def report(self, count: int) -> None:
        """Record ``count`` arrivals; raises on wraparound (HW-7)."""
        total = self._arrivals + count
        if total > self.capacity:
            raise WseModelError(
                f"opcode {self.opcode}: {total} arrivals exceed the "
                f"{self._width_bits} bit semaphore capacity {self.capacity}; "
                "HW-7 requires at least ceil(log2(members + 1)) bits"
            )
        self._arrivals = total

    def check_width(self, members: int) -> bool:
        """Whether ``members`` fits the configured width (HW-7)."""
        return members <= self.capacity

    def describe(self) -> dict[str, object]:
        return {
            "opcode": self.opcode,
            "semaphore_width_bits": self._width_bits,
            "semaphore_capacity": self.capacity,
            "arrivals": self._arrivals,
            "required_width_bits": self.required_width_bits,
        }
