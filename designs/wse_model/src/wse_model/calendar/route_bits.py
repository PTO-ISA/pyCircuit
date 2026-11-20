"""The ``routeBits`` path bitmap.

Calendar §2.2: every NoC node owns a two-bit ``{P, L}`` pair, so the bitmap is
``2 x node_count`` bits. Bit order is fixed:

* ``routeBits[2i + 1] = P_i``
* ``routeBits[2i]     = L_i``
* bit ``b`` lives in byte ``b / 8`` at bit position ``b % 8`` (LSB first)

At 40 nodes that is 80 bit = 10 B, packed into one ``CalendarRouteEntry``
(Calendar §2.6). The codec never routes and never validates reachability: it
only maps between the four pair encodings and their bytes. Structure checks
live in :mod:`wse_model.calendar.validate`.
"""

from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from enum import IntEnum

from wse_model.errors import IllegalBitPairError

__all__ = ["BitPair", "RouteBits"]


class BitPair(IntEnum):
    """The four legal-or-illegal ``{P, L}`` encodings (Calendar §2.2).

    The integer value is ``L | (P << 1)``, which is exactly the on-the-wire bit
    order, so ``byte = routeBits[2i] | (routeBits[2i+1] << 1)``.
    """

    ABSENT = 0b00
    """``00`` — unrelated to this flit: neither land nor forward."""

    ILLEGAL = 0b01
    """``01`` — ``L = 1`` with ``P = 0``. The compiler must reject it; hardware faults."""

    PASS = 0b10
    """``10`` — forward only, do not land."""

    LAND = 0b11
    """``11`` — land and forward (a multicast fork or a reduction point)."""

    @property
    def passes(self) -> bool:
        """``P``: the node forwards the flit."""
        return bool(self.value & 0b10)

    @property
    def lands(self) -> bool:
        """``L``: the node commits the payload to its receive range."""
        return bool(self.value & 0b01)

    @classmethod
    def from_flags(
        cls, *, p: bool, l: bool
    ) -> BitPair:  # noqa: E741 - P/L are the document's names
        return cls((0b10 if p else 0) | (0b01 if l else 0))


@dataclass(frozen=True)
class RouteBits:
    """An immutable ``2 x node_count`` bit route bitmap."""

    node_count: int
    _value: int

    # -- construction --------------------------------------------------------

    def __post_init__(self) -> None:
        if self.node_count < 1:
            raise ValueError(f"node_count must be positive, got {self.node_count}")
        if self._value < 0 or self._value >> (2 * self.node_count):
            raise ValueError(
                f"value 0x{self._value:x} does not fit in the "
                f"{2 * self.node_count} bits of a {self.node_count}-node route map"
            )

    @classmethod
    def from_int(cls, value: int, node_count: int) -> RouteBits:
        return cls(node_count=node_count, _value=value)

    @classmethod
    def from_bytes(cls, data: bytes, node_count: int) -> RouteBits:
        """Decode little-endian packed pairs.

        Every pair value is representable, including the illegal ``01``: raw
        decoding must not silently repair bad input. Use
        :meth:`illegal_nodes` or :func:`wse_model.calendar.validate.validate_route_bits`
        to reject it.
        """
        expected = (2 * node_count + 7) // 8
        if len(data) != expected:
            raise ValueError(
                f"routeBits for {node_count} nodes must be {expected} bytes, got {len(data)}"
            )
        return cls(node_count=node_count, _value=int.from_bytes(data, "little"))

    @classmethod
    def from_hex(cls, text: str, node_count: int) -> RouteBits:
        compact = text.replace(" ", "").replace("_", "").replace("\n", "")
        try:
            data = bytes.fromhex(compact)
        except ValueError as exc:
            raise ValueError(f"routeBits is not valid hex: {text!r}") from exc
        return cls.from_bytes(data, node_count)

    @classmethod
    def from_sets(
        cls,
        node_count: int,
        *,
        pass_nodes: Iterable[int],
        land_nodes: Iterable[int],
    ) -> RouteBits:
        """Build from the ``P`` and ``L`` node sets.

        This is the checked constructor: a land node that is not a pass node
        would produce the illegal pair ``01`` (Calendar §2.2), so it is
        rejected here.
        """
        passing = frozenset(pass_nodes)
        landing = frozenset(land_nodes)
        _check_range(node_count, passing, "pass_nodes")
        _check_range(node_count, landing, "land_nodes")
        illegal = sorted(landing - passing)
        if illegal:
            raise IllegalBitPairError(illegal[0], context="from_sets: L=1 requires P=1")
        value = 0
        for node in passing:
            value |= 0b10 << (2 * node)
        for node in landing:
            value |= 0b01 << (2 * node)
        return cls(node_count=node_count, _value=value)

    # -- access --------------------------------------------------------------

    def pair(self, node: int) -> BitPair:
        _check_range(self.node_count, (node,), "node")
        return BitPair((self._value >> (2 * node)) & 0b11)

    def pairs(self) -> tuple[BitPair, ...]:
        """All ``{P, L}`` pairs in node order."""
        return tuple(self.pair(node) for node in range(self.node_count))

    @property
    def pass_nodes(self) -> frozenset[int]:
        return frozenset(n for n in range(self.node_count) if self.pair(n).passes)

    @property
    def land_nodes(self) -> frozenset[int]:
        return frozenset(n for n in range(self.node_count) if self.pair(n).lands)

    @property
    def land_count(self) -> int:
        """``popcount(L)``, carried in the table entry for cross-checking."""
        return len(self.land_nodes)

    def illegal_nodes(self) -> tuple[int, ...]:
        """Nodes whose pair is the illegal ``01``, in ascending order."""
        return tuple(n for n in range(self.node_count) if self.pair(n) is BitPair.ILLEGAL)

    # -- encoding ------------------------------------------------------------

    @property
    def width_bits(self) -> int:
        return 2 * self.node_count

    @property
    def width_bytes(self) -> int:
        return (2 * self.node_count + 7) // 8

    def to_int(self) -> int:
        return self._value

    def to_bytes(self) -> bytes:
        return self._value.to_bytes(self.width_bytes, "little")

    def to_hex(self, *, spaced: bool = True) -> str:
        raw = self.to_bytes().hex().upper()
        if not spaced:
            return raw
        return " ".join(raw[i : i + 2] for i in range(0, len(raw), 2))

    # -- derivation ----------------------------------------------------------

    def with_pair(self, node: int, pair: BitPair) -> RouteBits:
        _check_range(self.node_count, (node,), "node")
        mask = 0b11 << (2 * node)
        return RouteBits(
            node_count=self.node_count,
            _value=(self._value & ~mask) | (int(pair) << (2 * node)),
        )

    def without(self, node: int) -> RouteBits:
        """Clear one node's pair, used to build the "all but self" land set."""
        return self.with_pair(node, BitPair.ABSENT)

    def describe(self) -> dict[str, object]:
        return {
            "node_count": self.node_count,
            "width_bits": self.width_bits,
            "width_bytes": self.width_bytes,
            "hex": self.to_hex(),
            "pass_nodes": sorted(self.pass_nodes),
            "land_nodes": sorted(self.land_nodes),
            "land_count": self.land_count,
            "illegal_nodes": list(self.illegal_nodes()),
        }

    def __str__(self) -> str:  # pragma: no cover - trivial
        return self.to_hex()


def _check_range(node_count: int, nodes: Sequence[int] | frozenset[int], label: str) -> None:
    for node in nodes:
        if not 0 <= node < node_count:
            raise ValueError(f"{label}: node {node} is outside 0..{node_count - 1}")
