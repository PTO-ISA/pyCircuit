"""Error hierarchy for the WSE model.

The model fails closed. Every illegal encoding, structural violation, or
failed consistency check raises rather than warning, because the design
documents treat these conditions as faults (Calendar §2.2, §2.7.2; whitepaper
§14.2).
"""

from __future__ import annotations

__all__ = [
    "CalendarError",
    "ConservationError",
    "IllegalBitPairError",
    "LandSetError",
    "OpenItemError",
    "ReceiveContractError",
    "RouteTreeError",
    "TopologyError",
    "VersionMismatchError",
    "WseModelError",
]


class WseModelError(Exception):
    """Base class for every error raised by the model."""


class TopologyError(WseModelError):
    """A node, coordinate, or link is outside the declared topology."""


class CalendarError(WseModelError):
    """Base class for Calendar encoding and validation faults."""


class IllegalBitPairError(CalendarError):
    """A ``{P, L}`` bit pair took the illegal value ``01``.

    Calendar §2.2 fixes the four encodings: ``00`` irrelevant, ``10`` pass,
    ``11`` pass and land, ``01`` illegal. The compiler must reject ``01``; the
    hardware must fault.
    """

    def __init__(self, node: int, *, context: str = "") -> None:
        detail = f" (in {context})" if context else ""
        super().__init__(
            f"node {node}: illegal {{P, L}} bit pair 01 (L=1 with P=0){detail}; "
            "Calendar §2.2 requires the compiler to reject it"
        )
        self.node = node


class RouteTreeError(CalendarError):
    """The ``{P=1}`` induced subgraph is not a tree containing the source.

    A cyclic or disconnected induced subgraph makes passive forwarding either
    non-terminating or unable to reach a land node (Calendar §2.2, §2.7.2).
    """

    def __init__(self, message: str, *, nodes: tuple[int, ...] = ()) -> None:
        super().__init__(message)
        self.nodes = nodes


class LandSetError(CalendarError):
    """The land set, ``landCount``, or ``selfLand`` bit disagrees with the model."""


class ConservationError(CalendarError):
    """Send/receive byte conservation failed for some destination.

    Calendar §2.7.2: for every destination ``d``,
    ``sum(rowCount * rowBytes over sources s with L_d(s)=1) == expectedRxBytes[d]``.
    This value is the device-side MTE4 ``expVal``.
    """

    def __init__(self, destination: int, expected: int, actual: int) -> None:
        super().__init__(
            f"destination {destination}: expectedRxBytes={expected} but the "
            f"declared land set delivers {actual} bytes; the two must be equal "
            "because the value is the device-side expVal (Calendar §2.7.2)"
        )
        self.destination = destination
        self.expected = expected
        self.actual = actual


class VersionMismatchError(WseModelError):
    """``topologyVersion`` / ``calendarVersion`` / ``routeVersion`` disagree.

    Whitepaper §10.3 and §14.2: the three versions are checked together at load
    time and any mismatch must fault rather than degrade.
    """


class ReceiveContractError(CalendarError):
    """A violation of one of the seven ``expVal`` contracts (Calendar §4.5.2).

    The receive side is the design's dominant correctness risk (HW-5). Every
    violation is a fault: a wrong-opcode or wrong-epoch arrival, an
    out-of-``dstRange`` write, an ``expVal`` above ``capacity``, or a counter
    overflow must never be absorbed silently, because a silently absorbed one
    lets the collective return early.
    """

    def __init__(self, contract: int, message: str) -> None:
        super().__init__(f"expVal contract {contract} violated: {message}")
        self.contract = contract


class OpenItemError(WseModelError):
    """A design value that the sources leave open was requested without a choice.

    Whitepaper Appendix A and Calendar §6.3 leave specific values unresolved.
    The model never substitutes a plausible default for them silently; it
    raises this error instead. See :mod:`wse_model.open_items`.
    """
