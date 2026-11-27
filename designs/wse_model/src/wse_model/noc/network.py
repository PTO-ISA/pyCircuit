"""The NoC as a whole: flit injection, passive forwarding, and byte accounting.

This module closes the loop the design documents describe. A send injects flits
carrying one ``routeBits`` bitmap and one ``opcode``; the mesh forwards them
passively; each landing node's ingress ledger counts valid payload bytes against
its ``expVal``. Nothing in that path is advisory — a reserved or uninstalled
``opcode``, a cyclic bitmap, an out-of-range landing, or a duplicate segment all
fault.

Timing is *not* modelled here. Packet release instants come from ``CalReg``,
whose contents are produced by the NoC algorithm under a timing model the model
does not own (Calendar §2.7.1). What this module does model exactly is the
**volume**: header overhead per flit per hop, link load, tree depth, and the
payload bytes that must add up to ``expVal``.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from wse_model.calendar.collective import Collective, RedOp
from wse_model.calendar.epoch import EpochTracker
from wse_model.calendar.geometry import SymmetricArena
from wse_model.calendar.receive import CoreIngress, DeliveryOutcome, Segment, SegmentId
from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.table import CalendarRouteTable, TableLayout
from wse_model.errors import CalendarError, WseModelError
from wse_model.noc.calreg import CalRegBank, CalRegImage, StopTheWorldWindow
from wse_model.noc.flit import (
    DEFAULT_LINK_WIDTH_BYTES,
    FlitHeader,
    flit_count,
)
from wse_model.noc.forward import MulticastTrace, simulate_multicast
from wse_model.topology import MeshTopology

__all__ = ["Noc", "SendOutcome", "SendPlan"]


@dataclass(frozen=True)
class SendPlan:
    """One ``MTE3_NOC_SEND`` worth of work, as the compiler would emit it."""

    source: int
    route_bits: RouteBits
    opcode: int
    epoch: int
    row_count: int
    row_bytes: int
    arena_base: int
    row_stride: int
    #: ``selfOff`` of the *sending* core. Calendar §2.8 makes the landing point a
    #: function of the source, not of the destination.
    source_self_off: int
    red_op: RedOp = RedOp.NONE
    #: Whether the fabric lands the source's own copy; when false, Step4s writes
    #: the local segment with a UB-to-UB copy and those bytes never count.
    self_delivery: bool = False
    #: Geometry the destination must expect, used to cross-check ``expVal``.
    expected_rx_bytes: int | None = None

    def __post_init__(self) -> None:
        if self.row_count < 0 or self.row_bytes < 0:
            raise WseModelError("rowCount and rowBytes must be non-negative")
        if self.row_stride < self.row_bytes:
            raise WseModelError(
                f"recvRowStride {self.row_stride} is smaller than rowBytes {self.row_bytes}"
            )

    @property
    def payload_bytes(self) -> int:
        """The source's own contribution, before multicast replication."""
        return self.row_count * self.row_bytes


@dataclass
class SendOutcome:
    """What one send did to the mesh and to every destination's ledger."""

    source: int
    land_nodes: tuple[int, ...]
    remote_land_nodes: tuple[int, ...]
    flits_injected: int
    flit_hops: int
    wire_bytes: int
    header_bytes_moved: int
    payload_bytes_delivered: int
    header_overhead_fraction: float
    duplicate_nodes: tuple[int, ...] = ()
    max_depth: int = 0
    link_load: dict[tuple[int, int], int] = field(default_factory=dict)
    deliveries: dict[str, int] = field(default_factory=dict)

    @property
    def useful_payload_fraction(self) -> float:
        """Delivered payload bytes over total wire bytes."""
        if self.wire_bytes == 0:
            return 0.0
        return self.payload_bytes_delivered / self.wire_bytes

    def describe(self) -> dict[str, object]:
        return {
            "source": self.source,
            "land_nodes": list(self.land_nodes),
            "remote_land_nodes": list(self.remote_land_nodes),
            "flits_injected": self.flits_injected,
            "flit_hops": self.flit_hops,
            "wire_bytes": self.wire_bytes,
            "header_bytes_moved": self.header_bytes_moved,
            "payload_bytes_delivered": self.payload_bytes_delivered,
            "header_overhead_percent": round(100.0 * self.header_overhead_fraction, 2),
            "useful_payload_fraction": round(self.useful_payload_fraction, 6),
            "duplicate_nodes": list(self.duplicate_nodes),
            "max_depth": self.max_depth,
        }


class Noc:
    """A die's worth of NoC state: register banks, windows, and ingress ledgers."""

    def __init__(
        self,
        topology: MeshTopology,
        *,
        calreg: CalRegImage | None = None,
        epoch_tag_bits: int = 8,
        link_width_bytes: int = DEFAULT_LINK_WIDTH_BYTES,
        self_delivery: bool = False,
    ) -> None:
        self.topology = topology
        self.epoch_tag_bits = epoch_tag_bits
        self.link_width_bytes = link_width_bytes
        self.self_delivery = self_delivery
        self.banks: dict[int, CalRegBank] = {node: CalRegBank(node) for node in topology.nodes}
        self.windows: dict[int, StopTheWorldWindow] = {
            node: StopTheWorldWindow(node) for node in topology.nodes
        }
        self.ingress: dict[int, CoreIngress] = {node: CoreIngress(node) for node in topology.nodes}
        #: ``CalendarNextEpoch(opcode)`` counters. These live on a kernel-local
        #: object, so they belong to the run, not to a single call: two phases
        #: sharing one ``opcode`` must advance the same domain (Calendar §1.5).
        self.epoch_trackers: dict[int, EpochTracker] = {
            node: EpochTracker(width_bits=epoch_tag_bits) for node in topology.nodes
        }
        self._sequence: dict[int, int] = {}
        self.installed_calreg: CalRegImage | None = None
        if calreg is not None:
            self.install_calreg(calreg)

    # -- CalReg --------------------------------------------------------------

    def install_calreg(self, image: CalRegImage) -> None:
        """Atomically install the timeslot image (Calendar §3.9)."""
        for window in self.windows.values():
            window.require_drained(action="install the CalReg image")
        image.install_into(self.banks, node_drained=dict.fromkeys(self.topology.nodes, True))
        self.installed_calreg = image

    def _check_release(self, source: int, opcode: int) -> None:
        """Model the node's ``CalReg[opcode]`` index, which can fault."""
        self.banks[source].lookup(opcode)

    @property
    def drained(self) -> bool:
        return all(window.drained for window in self.windows.values())

    def next_epoch(self, node: int, opcode: int) -> int:
        """Advance this core's ``opcode`` counter and return the round number."""
        return self.epoch_trackers[node].next_epoch(opcode, drained=self.drained)

    # -- send ----------------------------------------------------------------

    def send(self, plan: SendPlan) -> SendOutcome:
        """Inject one send and account every resulting landing."""
        self._check_release(plan.source, plan.opcode)
        header = FlitHeader(
            route_bits=plan.route_bits,
            opcode=plan.opcode,
            red_op=plan.red_op,
            epoch_tag=plan.epoch,
            epoch_tag_bits=self.epoch_tag_bits,
            link_width_bytes=self.link_width_bytes,
        )
        trace = simulate_multicast(self.topology, plan.route_bits, source=plan.source)
        if not trace.is_clean_tree_walk:
            raise CalendarError(
                f"source {plan.source}: the {{P=1}} induced subgraph is not a tree "
                f"(duplicate landings at {list(trace.duplicate_nodes)}, revisits "
                f"{list(trace.unexpanded_revisits)}); forwarding would not "
                "terminate and expVal would be met early (Calendar §2.2)"
            )

        land_nodes = trace.land_nodes
        remote = tuple(node for node in land_nodes if node != plan.source)
        per_row_flits = flit_count(plan.row_bytes, header)
        flits_injected = per_row_flits * plan.row_count
        flit_hops = trace.hop_count * flits_injected
        wire_bytes = flit_hops * self.link_width_bytes

        base = self._sequence.get(plan.source, 0)
        deliveries: dict[str, int] = {}
        payload_delivered = 0

        for row in range(plan.row_count):
            seq = base + row
            address = plan.arena_base + row * plan.row_stride + plan.source_self_off
            for landing in trace.landings:
                self_sourced = landing.node == plan.source
                if self_sourced and not plan.self_delivery:
                    # Step4s replaces the fabric's self-delivery with a local
                    # UB-to-UB copy; those bytes never enter expVal.
                    deliveries[DeliveryOutcome.SELF_SOURCED.value] = (
                        deliveries.get(DeliveryOutcome.SELF_SOURCED.value, 0) + 1
                    )
                    continue
                segment = Segment(
                    identity=SegmentId(source=plan.source, seq=seq),
                    opcode=plan.opcode,
                    epoch=plan.epoch,
                    address=address,
                    nbytes=plan.row_bytes,
                    self_sourced=self_sourced,
                )
                outcome = self.ingress[landing.node].deliver(segment)
                deliveries[outcome.value] = deliveries.get(outcome.value, 0) + 1
                if outcome is DeliveryOutcome.COUNTED:
                    payload_delivered += plan.row_bytes
        self._sequence[plan.source] = base + max(1, plan.row_count)
        for window in self.windows.values():
            window.enter(flit_hops)

        return SendOutcome(
            source=plan.source,
            land_nodes=land_nodes,
            remote_land_nodes=remote,
            flits_injected=flits_injected,
            flit_hops=flit_hops,
            wire_bytes=wire_bytes,
            header_bytes_moved=flit_hops * header.header_bytes,
            payload_bytes_delivered=payload_delivered,
            header_overhead_fraction=header.overhead_fraction,
            duplicate_nodes=trace.duplicate_nodes,
            max_depth=trace.max_depth,
            link_load=trace.link_load(),
            deliveries=deliveries,
        )

    def trace(self, plan: SendPlan) -> MulticastTrace:
        """Compute the forwarding tree for a plan without delivering."""
        return simulate_multicast(self.topology, plan.route_bits, source=plan.source)

    # -- inspection ----------------------------------------------------------

    def describe(self) -> dict[str, object]:
        return {
            "topology": self.topology.describe(),
            "epoch_tag_bits": self.epoch_tag_bits,
            "link_width_bytes": self.link_width_bytes,
            "self_delivery": self.self_delivery,
            "epochs": {
                str(node): tracker.describe()
                for node, tracker in sorted(self.epoch_trackers.items())
                if tracker.describe()
            },
            "calreg": self.installed_calreg.describe() if self.installed_calreg else None,
            "drained": self.drained,
            "ingress": {
                str(node): ledger.describe()
                for node, ledger in sorted(self.ingress.items())
                if ledger.outstanding or ledger.pending
            },
        }


def table_layout_lines(table: CalendarRouteTable, *, layout: TableLayout | None = None) -> int:
    """Per-core D-cache residency for a table under a layout (open item ``C-9``).

    ``layout`` overrides the table's own layout, which is what makes the
    key-major versus node-major comparison possible without re-emitting: the
    entries are the same 16 B objects, only their order changes.
    """
    chosen = layout or table.layout
    return chosen.d_cache_lines_per_core(table.key_count)


def allgather_geometry_check(
    *,
    collective: Collective,
    geometry_rows: int,
    arena: SymmetricArena,
) -> None:
    """Refuse to model a non-AllGather closure before its semantics are frozen."""
    if collective is not Collective.ALL_GATHER:
        raise CalendarError(
            f"the closure models AllGather only; {collective.name} requires the "
            "reduction semantics of open item C-10 (merge buffering, out-of-order "
            "merge timing, overflow behaviour) to be defined first"
        )
    if geometry_rows != arena.row_count:
        raise CalendarError(
            f"payload rowCount {geometry_rows} disagrees with arena rowCount {arena.row_count}"
        )
