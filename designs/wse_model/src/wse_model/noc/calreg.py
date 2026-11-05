"""The resident Calendar timeslot register, ``CalReg``.

Calendar §3.9: the *value* of the timeslot information is computed at compile
time and installed once into a resident register on each NoC node. The core never
sees that value — it only supplies a 3 bit ``opcode``, and the node performs one
register index ``CalReg[opcode]`` to decide when to release the packet.

Properties the model enforces:

* **Resident, read-only during kernel execution.** Installation happens in one
  atomic write inside a stop-the-world window; while any Calendar traffic is in
  flight a rewrite must be refused.
* **Independent queue per ``opcode``.** A node keeps a separate waiting queue per
  ``opcode`` so that one domain cannot head-of-line block another.
* **Illegal or uninstalled codes fault.** ``opcode in {0, 7}`` and any code with
  no installed content must fault or be dropped and reported; they must never be
  released by a default entry.
* **Shared constraint.** Every collective using one ``opcode`` shares one
  ``CalReg[opcode]`` entry, so the "no conflict" and "no concurrency" guarantees
  are the NoC algorithm's responsibility (Calendar §2.7.1). The model records the
  algorithm's ``conflictProof`` and never re-derives the timing guarantee.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from wse_model.calendar.collective import OPCODE_RESERVED
from wse_model.errors import CalendarError, WseModelError

__all__ = [
    "CalRegBank",
    "CalRegImage",
    "CalRegSlot",
    "StopTheWorldWindow",
]


@dataclass(frozen=True)
class CalRegSlot:
    """One ``CalReg[opcode]`` entry on one node."""

    opcode: int
    #: Opaque register image; its internal format is defined by the NoC and the
    #: program never interprets it (Calendar §2.3).
    content: bytes
    #: The window the NoC/BSP reserves after alignment for the receive command
    #: and every send descriptor to be enqueued (Calendar §1.5).
    arm_lead_cycles: int = 0
    #: Which logical identities share this slot, for the concurrency audit.
    identities: tuple[str, ...] = ()

    def __post_init__(self) -> None:
        if self.opcode in OPCODE_RESERVED:
            raise CalendarError(
                f"CalReg[{self.opcode}] may not be installed: opcode {self.opcode} "
                "is reserved, so the node must fault rather than release "
                "(Calendar §2.3, §3.9)"
            )
        if self.arm_lead_cycles < 0:
            raise WseModelError(f"CalReg[{self.opcode}]: negative armLeadCycles")

    def describe(self) -> dict[str, object]:
        return {
            "opcode": self.opcode,
            "content_bytes": len(self.content),
            "arm_lead_cycles": self.arm_lead_cycles,
            "identities": list(self.identities),
        }


class CalRegBank:
    """The per-node register file: one slot per installed ``opcode``."""

    __slots__ = ("_slots", "node")

    def __init__(self, node: int) -> None:
        self.node = node
        self._slots: dict[int, CalRegSlot] = {}

    @property
    def installed(self) -> tuple[int, ...]:
        return tuple(sorted(self._slots))

    def install(self, slot: CalRegSlot) -> None:
        if slot.opcode in self._slots:
            raise CalendarError(
                f"node {self.node}: CalReg[{slot.opcode}] is already installed; a "
                "rewrite requires draining all Calendar traffic first "
                "(Calendar §3.9)"
            )
        self._slots[slot.opcode] = slot

    def clear(self) -> None:
        self._slots.clear()

    def lookup(self, opcode: int) -> CalRegSlot:
        """Index ``CalReg[opcode]``; an uninstalled or reserved code faults."""
        if opcode in OPCODE_RESERVED:
            raise CalendarError(
                f"node {self.node}: flit carries reserved opcode {opcode}; the node "
                "must fault rather than release (Calendar §2.3, §3.9)"
            )
        try:
            return self._slots[opcode]
        except KeyError:
            raise CalendarError(
                f"node {self.node}: CalReg[{opcode}] is not installed; an "
                "uninstalled code must fault rather than fall through to a "
                "default entry (Calendar §3.9)"
            ) from None

    def describe(self) -> dict[str, object]:
        return {
            "node": self.node,
            "installed": [self._slots[opcode].describe() for opcode in self.installed],
        }


class StopTheWorldWindow:
    """Tracks Calendar traffic so ``CalReg`` installation stays atomic.

    Calendar §3.9 and runtime constraint 3 of whitepaper §12.3: the register
    image is installed once, inside a stop-the-world window, and no Calendar
    traffic may be in flight while it happens. The same drain property is what
    makes cross-launch epoch isolation valid (``SW-1``), so it is modelled rather
    than assumed.
    """

    __slots__ = ("_in_flight", "node")

    def __init__(self, node: int) -> None:
        self.node = node
        self._in_flight = 0

    @property
    def in_flight(self) -> int:
        return self._in_flight

    @property
    def drained(self) -> bool:
        return self._in_flight == 0

    def enter(self, count: int = 1) -> None:
        self._in_flight += count

    def drain(self, count: int = 1) -> None:
        self._in_flight = max(0, self._in_flight - count)

    def require_drained(self, *, action: str) -> None:
        if not self.drained:
            raise CalendarError(
                f"node {self.node}: cannot {action} while {self._in_flight} "
                "Calendar transfer(s) are in flight; CalReg writes and launch "
                "boundaries require a drained die (Calendar §3.9, whitepaper "
                "§12.3, SW-1)"
            )


@dataclass
class CalRegImage:
    """The die-wide ``CalReg`` mirror handed to the loader.

    All nodes receive a byte-identical image; only the node-side index matters,
    not any per-core difference (Calendar §2.7.3 step 5).
    """

    slots: tuple[CalRegSlot, ...] = ()
    calendar_version: int = 0
    _by_opcode: dict[int, CalRegSlot] = field(init=False, repr=False, default_factory=dict)

    def __post_init__(self) -> None:
        for slot in self.slots:
            if slot.opcode in self._by_opcode:
                raise CalendarError(f"CalReg image lists opcode {slot.opcode} twice")
            self._by_opcode[slot.opcode] = slot

    @property
    def opcodes(self) -> tuple[int, ...]:
        return tuple(sorted(self._by_opcode))

    def slot(self, opcode: int) -> CalRegSlot:
        try:
            return self._by_opcode[opcode]
        except KeyError:
            raise CalendarError(f"the CalReg image has no entry for opcode {opcode}") from None

    def install_into(self, banks: dict[int, CalRegBank], *, node_drained: dict[int, bool]) -> None:
        """Install into every node, refusing if any node is not drained."""
        for node in sorted(banks):
            if not node_drained.get(node, False):
                raise CalendarError(
                    f"node {node}: CalReg installation requires a drained die; "
                    "there is Calendar traffic in flight (Calendar §3.9)"
                )
        for bank in banks.values():
            bank.clear()
        for slot in self.slots:
            for bank in banks.values():
                bank.install(slot)

    def describe(self) -> dict[str, object]:
        return {
            "calendarVersion": self.calendar_version,
            "opcodes": list(self.opcodes),
            "bytes": sum(len(slot.content) for slot in self.slots),
            "slots": [self._by_opcode[opcode].describe() for opcode in self.opcodes],
        }
