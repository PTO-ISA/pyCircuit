"""The 16 B ``CalendarRouteEntry`` and its register-pair view.

Calendar §2.6 fixes the ``.rodata`` entry at exactly **16 B**, so that two
ordinary 64 bit scalar loads fill one GPR pair:

.. code-block:: text

    struct alignas(16) CalendarRouteEntry {   // 16 B
        uint8_t  routeBits[10];   // 40 x {P,L}: bit(2i+1)=P_i, bit(2i)=L_i
        uint8_t  landCount;       // popcount(L), for cross-checking
        uint8_t  flags;           // bit0 isMember, bit1 isRoot, bit2 selfLand
        uint32_t expectedRxBytes; // this core's expVal as a destination
    };

Register pair layout::

    rbLo = bytes 0..7   -> nodes 00..31
    rbHi = bytes 8..15  -> [15:0]  nodes 32..39
                           [23:16] landCount
                           [31:24] flags
                           [63:32] expectedRxBytes

The send instruction takes only ``rbHi[15:0]`` as routing bits; the upper 48 bit
must be ignored and must not be checked (HW-4). That is what lets ``landCount``,
``flags``, and ``expectedRxBytes`` ride along at zero extra load cost.

A consequence of open item ``Q1`` is worth stating explicitly: the 16 B layout
only holds a 40-node bitmap. At 48 nodes ``routeBits`` alone is 12 B, so the
entry would need 18 B and could no longer be one GPR pair.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntFlag

from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import LandSetError, WseModelError

__all__ = [
    "ENTRY_LAYOUT_OVERHEAD_BYTES",
    "ENTRY_SIZE_BYTES",
    "ROUTE_BITS_NODE_CAPACITY",
    "RouteEntryFlags",
    "CalendarRouteRegs",
    "CalendarRouteEntry",
    "entry_layout_size_bytes",
]

#: ``routeBits[10] + landCount(1) + flags(1) + expectedRxBytes(4)``.
ENTRY_LAYOUT_OVERHEAD_BYTES = 6

#: The frozen entry size: one GPR pair (Calendar §2.6).
ENTRY_SIZE_BYTES = 16

#: Largest node count whose bitmap plus the fixed tail still fits 16 B.
ROUTE_BITS_NODE_CAPACITY = (ENTRY_SIZE_BYTES - ENTRY_LAYOUT_OVERHEAD_BYTES) * 4


def entry_layout_size_bytes(node_count: int) -> int:
    """Bytes an entry needs for ``node_count`` nodes, before any alignment."""
    route_bytes = (2 * node_count + 7) // 8
    return route_bytes + ENTRY_LAYOUT_OVERHEAD_BYTES


class RouteEntryFlags(IntFlag):
    """The ``flags`` byte of a route entry (Calendar §2.6).

    * bit0 ``isMember`` — this core belongs to the collective's member set;
    * bit1 ``isRoot`` — this core is the routing tree root, where the semantics
      require one;
    * bit2 ``selfLand`` — whether the fabric self-delivers, which must equal
      ``CalendarPlatformTraits::FabricSelfDelivery`` for the source's own ``L``
      bit to be consistent (Calendar §2.7.2, HW-10).
    """

    NONE = 0
    IS_MEMBER = 1 << 0
    IS_ROOT = 1 << 1
    SELF_LAND = 1 << 2


@dataclass(frozen=True)
class CalendarRouteRegs:
    """The two 64 bit registers a route entry is loaded into.

    The field boundaries mirror Calendar §1.7 exactly; they are not
    re-derived, so a layout change is immediately visible here.
    """

    rb_lo: int
    rb_hi: int

    @property
    def route_bits_value(self) -> int:
        """``routeBits80 = GR_rb0[63:0] ++ GR_rb1[15:0]``."""
        return (self.rb_lo & 0xFFFF_FFFF_FFFF_FFFF) | ((self.rb_hi & 0xFFFF) << 64)

    @property
    def land_count(self) -> int:
        return (self.rb_hi >> 16) & 0xFF

    @property
    def flags(self) -> RouteEntryFlags:
        return RouteEntryFlags((self.rb_hi >> 24) & 0xFF)

    @property
    def expected_rx_bytes(self) -> int:
        return (self.rb_hi >> 32) & 0xFFFF_FFFF

    def describe(self) -> dict[str, object]:
        return {
            "rbLo": f"0x{self.rb_lo:016X}",
            "rbHi": f"0x{self.rb_hi:016X}",
            "route_bits": f"0x{self.route_bits_value:0{16}X}",
            "land_count": self.land_count,
            "flags": int(self.flags),
            "expected_rx_bytes": self.expected_rx_bytes,
        }


@dataclass(frozen=True)
class CalendarRouteEntry:
    """One ``kCalendarRoute[keyId][node]`` table entry."""

    route_bits: RouteBits
    expected_rx_bytes: int = 0
    flags: RouteEntryFlags = RouteEntryFlags.NONE
    #: Overrides ``popcount(L)`` only to *detect* a mismatch; ``None`` derives it.
    declared_land_count: int | None = None

    def __post_init__(self) -> None:
        if self.expected_rx_bytes < 0 or self.expected_rx_bytes > 0xFFFF_FFFF:
            raise WseModelError(
                f"expectedRxBytes {self.expected_rx_bytes} does not fit the "
                "entry's uint32 field (Calendar §2.6)"
            )
        actual = self.route_bits.land_count
        if self.declared_land_count is not None and self.declared_land_count != actual:
            raise LandSetError(
                f"declared landCount={self.declared_land_count} but popcount(L)="
                f"{actual}; the entry carries landCount only for cross-checking "
                "(Calendar §2.6)"
            )

    # -- derived -------------------------------------------------------------

    @property
    def node_count(self) -> int:
        return self.route_bits.node_count

    @property
    def land_count(self) -> int:
        return self.route_bits.land_count

    @property
    def is_member(self) -> bool:
        return bool(self.flags & RouteEntryFlags.IS_MEMBER)

    @property
    def is_root(self) -> bool:
        return bool(self.flags & RouteEntryFlags.IS_ROOT)

    @property
    def self_land(self) -> bool:
        return bool(self.flags & RouteEntryFlags.SELF_LAND)

    # -- encoding ------------------------------------------------------------

    def to_bytes(self) -> bytes:
        route = self.route_bits.to_bytes()
        tail = bytes([self.land_count, int(self.flags) & 0xFF])
        payload = route + tail + self.expected_rx_bytes.to_bytes(4, "little")
        expected = entry_layout_size_bytes(self.node_count)
        if expected != ENTRY_SIZE_BYTES:
            raise WseModelError(
                f"a {self.node_count}-node entry needs {expected} B and cannot be "
                f"packed into the {ENTRY_SIZE_BYTES} B one-GPR-pair layout of "
                "Calendar §2.6; at 48 nodes routeBits alone is 12 B (open item "
                "Q1). Resolve Q1 before emitting a table for this topology."
            )
        return payload

    @classmethod
    def from_bytes(cls, data: bytes, node_count: int) -> CalendarRouteEntry:
        expected = entry_layout_size_bytes(node_count)
        if len(data) != expected or len(data) != ENTRY_SIZE_BYTES:
            raise WseModelError(
                f"a {node_count}-node entry is {expected} B, not the required "
                f"{ENTRY_SIZE_BYTES} B (open item Q1)"
            )
        route_bytes = (2 * node_count + 7) // 8
        route = RouteBits.from_bytes(data[:route_bytes], node_count)
        land_count = data[route_bytes]
        flags = RouteEntryFlags(data[route_bytes + 1])
        expected_rx = int.from_bytes(data[route_bytes + 2 : route_bytes + 6], "little")
        return cls(
            route_bits=route,
            expected_rx_bytes=expected_rx,
            flags=flags,
            declared_land_count=land_count,
        )

    @classmethod
    def from_regs(cls, regs: CalendarRouteRegs, node_count: int) -> CalendarRouteEntry:
        """Decode from the register pair, ignoring ``rbHi`` above bit 15+48.

        Hardware is required to ignore the high 48 bit of ``rbHi`` when sending
        (HW-4), but the tail fields are exactly those bits, so this decoder reads
        them deliberately rather than ignoring them.
        """
        raw = regs.route_bits_value.to_bytes((2 * node_count + 7) // 8, "little")
        route = RouteBits.from_bytes(raw, node_count)
        return cls(
            route_bits=route,
            expected_rx_bytes=regs.expected_rx_bytes,
            flags=regs.flags,
            declared_land_count=regs.land_count,
        )

    def to_regs(self) -> CalendarRouteRegs:
        payload = self.to_bytes()
        lo = int.from_bytes(payload[0:8], "little")
        hi = int.from_bytes(payload[8:16], "little")
        return CalendarRouteRegs(rb_lo=lo, rb_hi=hi)

    def describe(self) -> dict[str, object]:
        return {
            "route_bits": self.route_bits.describe(),
            "land_count": self.land_count,
            "is_member": self.is_member,
            "is_root": self.is_root,
            "self_land": self.self_land,
            "expected_rx_bytes": self.expected_rx_bytes,
            "regs": self.to_regs().describe(),
        }
