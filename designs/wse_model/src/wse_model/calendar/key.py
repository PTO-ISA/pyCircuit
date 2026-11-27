"""Logical identity and the inseparable ``CalendarKeyRef`` constant pair.

Calendar §2.5 defines the logical identity::

    RouteKey = { programId, kernelId, phaseId, collective, groupRole }

and requires ``keyId`` and ``opcode`` to be resolved from that one identity and
packaged as a single compile-time constant that call sites cannot split::

    struct CalendarKeyRef { keyId, opcode, redOp, routeVersion };

This module enforces that discipline in the model. Ordering invariant **O1** —
"the ``routeBits`` and the ``opcode`` of one send must come from the same
logical identity" — has no hardware anchor, because the bitmap travels in
``.rodata``/D-cache and the ``opcode`` travels in ``.text``/I-cache. The
registry is the model's substitute for that missing anchor.

``groupRole`` is a *role*, not a specific group: under SPMD one kernel text can
only say "my ROW group". PyPTO resolves the role against every core coordinate
at compile time and emits all rows, so the runtime index is a single
``blockId`` lookup.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.calendar.collective import Collective, RedOp
from wse_model.errors import CalendarError, VersionMismatchError, WseModelError

__all__ = [
    "CalendarKeyRef",
    "CalendarKeyRegistry",
    "GroupRole",
    "RouteKey",
]


@dataclass(frozen=True)
class GroupRole:
    """A parameterized group role such as ``ROW(my_row)``.

    ``kind`` is the role family (``ROW``, ``COL``, ``EP``, ...) and ``axis`` is
    the compile-time index expression the compiler substitutes per core. A
    concrete role has an integer ``axis``; a symbolic one carries the expression
    text, because under SPMD the kernel cannot name the concrete group.
    """

    kind: str
    axis: int | str

    def __post_init__(self) -> None:
        if not self.kind:
            raise WseModelError("GroupRole.kind must be non-empty")
        if isinstance(self.axis, int) and self.axis < 0:
            raise WseModelError(f"GroupRole.axis must be non-negative, got {self.axis}")

    def concretize(self, axis: int) -> GroupRole:
        """Return the role with a concrete axis, i.e. after per-core resolution."""
        return GroupRole(kind=self.kind, axis=axis)

    def __str__(self) -> str:
        return f"{self.kind}({self.axis})"


@dataclass(frozen=True)
class RouteKey:
    """The stable compile-time identity of one Calendar route/timeslot combination."""

    program_id: str
    kernel_id: str
    phase_id: str
    collective: Collective
    group_role: GroupRole

    def __post_init__(self) -> None:
        for field_name in ("program_id", "kernel_id", "phase_id"):
            if not getattr(self, field_name):
                raise WseModelError(f"RouteKey.{field_name} must be non-empty")
        if self.collective.is_reserved:
            raise CalendarError(
                f"collective {self.collective.name} is reserved and cannot carry "
                "a logical identity (Calendar §2.3)"
            )

    @property
    def opcode(self) -> int:
        """The identity determines the ``opcode`` outright (Calendar §2.5)."""
        return self.collective.opcode

    def describe(self) -> dict[str, object]:
        return {
            "program_id": self.program_id,
            "kernel_id": self.kernel_id,
            "phase_id": self.phase_id,
            "collective": self.collective.name,
            "opcode": self.opcode,
            "group_role": str(self.group_role),
        }


@dataclass(frozen=True)
class CalendarKeyRef:
    """The value a call site passes: an indivisible compile-time constant pair.

    The wrapper layer must pass this **by value or as a non-type template
    parameter** and must never take its address (Calendar §3.10, prohibition
    F2). Taking the address demotes ``opcode`` from an I-cache immediate to a
    D-cache load and breaks the "alignment adds no ISA" property.
    """

    key_id: int
    opcode: Collective
    red_op: RedOp = RedOp.NONE
    route_version: int = 0

    def __post_init__(self) -> None:
        if not 0 <= self.key_id < 0xFFFF:
            raise WseModelError(f"keyId {self.key_id} does not fit the uint16 key dimension")
        if self.opcode.is_reserved:
            raise CalendarError(
                f"CalendarKeyRef.opcode {self.opcode.name} is reserved; opcode 0 "
                "must fault and opcode 7 is extension-only (Calendar §2.3)"
            )
        if self.opcode.is_reduction and self.red_op is RedOp.NONE:
            raise CalendarError(
                f"{self.opcode.name} requires a redOp; only the copy collectives "
                "may carry None (Calendar §2.4)"
            )
        if not self.opcode.is_reduction and self.red_op is not RedOp.NONE:
            raise CalendarError(
                f"{self.opcode.name} is a copy collective, so redOp must be None "
                f"(got {self.red_op.name}); the field is dormant (Calendar §2.4)"
            )
        if not 0 <= self.route_version <= 0xFFFF_FFFF:
            raise WseModelError(f"routeVersion {self.route_version} does not fit uint32")

    @property
    def opcode_value(self) -> int:
        return int(self.opcode)

    def require_version(self, *, route_version: int) -> None:
        """Check the route version against a loaded table (whitepaper §10.3)."""
        if self.route_version != route_version:
            raise VersionMismatchError(
                f"CalendarKeyRef(keyId={self.key_id}) carries routeVersion "
                f"{self.route_version} but the loaded route table is version "
                f"{route_version}; load must fault rather than degrade"
            )

    def describe(self) -> dict[str, object]:
        return {
            "key_id": self.key_id,
            "opcode": self.opcode.name,
            "opcode_value": self.opcode_value,
            "red_op": self.red_op.name,
            "route_version": self.route_version,
        }


class CalendarKeyRegistry:
    """Assigns ``keyId`` values and defends invariant O1.

    The registry is the compile-time bookkeeping the design document describes:
    one logical identity maps to exactly one ``(keyId, opcode)`` pair, and a
    ``keyId`` never maps to two identities.
    """

    MAX_KEYS = 16  #: Calendar §2.6 budget: keyCount <= 16.

    __slots__ = ("_by_id", "_by_key")

    def __init__(self) -> None:
        self._by_id: dict[int, RouteKey] = {}
        self._by_key: dict[RouteKey, int] = {}

    def __len__(self) -> int:
        return len(self._by_id)

    @property
    def key_count(self) -> int:
        return len(self._by_id)

    def items(self) -> tuple[tuple[int, RouteKey], ...]:
        return tuple(sorted(self._by_id.items()))

    def register(self, key: RouteKey, *, key_id: int | None = None) -> int:
        """Register ``key`` and return its ``keyId``.

        Re-registering the same identity is idempotent. Registering a different
        identity under an occupied ``keyId`` is rejected: that is precisely the
        O1 failure mode, where a route bitmap and a timeslot would disagree.
        """
        existing = self._by_key.get(key)
        if existing is not None:
            if key_id is not None and key_id != existing:
                raise CalendarError(
                    f"identity {key.describe()} is already registered as keyId "
                    f"{existing}, but keyId {key_id} was requested; one identity "
                    "must resolve to exactly one (keyId, opcode) pair (invariant O1)"
                )
            return existing

        if key_id is None:
            candidates = [i for i in range(self.MAX_KEYS) if i not in self._by_id]
            if not candidates:
                raise CalendarError(
                    f"no free keyId: the Calendar table is limited to "
                    f"{self.MAX_KEYS} keys (Calendar §2.6, §3.8.4)"
                )
            key_id = candidates[0]

        if not 0 <= key_id < self.MAX_KEYS:
            raise CalendarError(
                f"keyId {key_id} is outside the 0..{self.MAX_KEYS - 1} budget (Calendar §2.6, C-12)"
            )
        occupant = self._by_id.get(key_id)
        if occupant is not None and occupant != key:
            raise CalendarError(
                f"keyId {key_id} is already bound to {occupant.describe()}, so it "
                f"cannot also bind {key.describe()}; the route table is per keyId "
                "(Calendar §2.5, invariant O1)"
            )

        self._by_id[key_id] = key
        self._by_key[key] = key_id
        return key_id

    def key_ref(
        self,
        key: RouteKey,
        *,
        route_version: int,
        red_op: RedOp | None = None,
    ) -> CalendarKeyRef:
        """Resolve an identity to its inseparable constant pair."""
        key_id = self._by_key.get(key)
        if key_id is None:
            raise CalendarError(
                f"identity {key.describe()} is not registered; call register() "
                "before resolving a CalendarKeyRef"
            )
        if red_op is None:
            red_op = RedOp.SUM if key.collective.is_reduction else RedOp.NONE
        return CalendarKeyRef(
            key_id=key_id,
            opcode=key.collective,
            red_op=red_op,
            route_version=route_version,
        )

    def describe(self) -> dict[str, object]:
        return {
            "key_count": self.key_count,
            "keys": {str(key_id): key.describe() for key_id, key in self.items()},
        }
