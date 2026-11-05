"""Stateless per-hop forwarding.

Calendar §2.2 gives the NoC exactly two rules, both passive::

    1. if L_self == 1:  commit the payload locally (and continue forwarding)
    2. out = { neighbour j : P_j == 1 } - ingress
       replicate one flit per egress port

There is no routing table, no lookup path, and no state. The whole path is the
``{P, L}`` bitmap the flit carries.

Because rule 2 copies to *all* physical neighbours with ``P = 1``, the routes the
algorithm "intended" are irrelevant: what must be a tree is the subgraph
**induced** by ``{P_i = 1}``. This module therefore simulates the induced
subgraph rather than any planned path.

A cyclic induced subgraph would replicate a flit without bound. The simulator
refuses to diverge: it expands each node at most once and records every repeat
arrival as a duplicate landing, which the validator reports as a fault.
"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass, field

from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import CalendarError
from wse_model.topology import MeshTopology

__all__ = [
    "Hop",
    "Landing",
    "MulticastTrace",
    "delivery_order",
    "link_load",
    "simulate_multicast",
]


@dataclass(frozen=True)
class Hop:
    """One node's forwarding decision for one arriving flit."""

    node: int
    ingress: int | None
    egress: tuple[int, ...]
    lands: bool

    def describe(self) -> dict[str, object]:
        return {
            "node": self.node,
            "ingress": self.ingress,
            "egress": list(self.egress),
            "lands": self.lands,
        }


@dataclass(frozen=True)
class Landing:
    """A commit of the payload into a node's receive range."""

    node: int
    ingress: int | None
    order: int
    #: True when the same node was reached again — only possible on a cyclic
    #: ``{P=1}`` subgraph, and fatal for ``expVal`` accounting.
    duplicate: bool = False


@dataclass
class MulticastTrace:
    """The result of forwarding one flit's worth of bitmap across the mesh."""

    source: int
    route_bits: RouteBits
    hops: tuple[Hop, ...]
    landings: tuple[Landing, ...]
    unexpanded_revisits: tuple[int, ...] = field(default_factory=tuple)

    @property
    def land_nodes(self) -> tuple[int, ...]:
        """Nodes that received the payload, in delivery order, deduplicated."""
        seen: list[int] = []
        for landing in self.landings:
            if landing.node not in seen:
                seen.append(landing.node)
        return tuple(seen)

    @property
    def duplicate_nodes(self) -> tuple[int, ...]:
        """Nodes that received the payload more than once (Calendar S-6/HW-15)."""
        counts: dict[int, int] = {}
        for landing in self.landings:
            counts[landing.node] = counts.get(landing.node, 0) + 1
        return tuple(sorted(node for node, count in counts.items() if count > 1))

    @property
    def is_clean_tree_walk(self) -> bool:
        return not self.duplicate_nodes and not self.unexpanded_revisits

    @property
    def hop_count(self) -> int:
        """Number of link traversals, i.e. flit-hops across the whole tree."""
        return sum(len(hop.egress) for hop in self.hops)

    @property
    def max_depth(self) -> int:
        """Longest source-to-land path in hops."""
        depth: dict[int, int] = {self.source: 0}
        for hop in self.hops:
            base = depth.get(hop.node, 0)
            for peer in hop.egress:
                depth[peer] = max(depth.get(peer, 0), base + 1)
        return max(depth.values(), default=0)

    def link_load(self) -> dict[tuple[int, int], int]:
        """Flits traversing each undirected physical link."""
        load: dict[tuple[int, int], int] = {}
        for hop in self.hops:
            for peer in hop.egress:
                key = (min(hop.node, peer), max(hop.node, peer))
                load[key] = load.get(key, 0) + 1
        return load

    def describe(self) -> dict[str, object]:
        return {
            "source": self.source,
            "land_nodes": list(self.land_nodes),
            "land_count": len(self.land_nodes),
            "duplicate_nodes": list(self.duplicate_nodes),
            "hops": self.hop_count,
            "max_depth": self.max_depth,
            "clean": self.is_clean_tree_walk,
        }


def simulate_multicast(
    topology: MeshTopology, route_bits: RouteBits, *, source: int
) -> MulticastTrace:
    """Forward a flit from ``source`` under the two Calendar rules."""
    if not 0 <= source < topology.node_count:
        raise CalendarError(f"source {source} is outside topology {topology.name}")
    if route_bits.node_count != topology.node_count:
        raise CalendarError(
            f"routeBits covers {route_bits.node_count} nodes but topology "
            f"{topology.name} has {topology.node_count}"
        )
    if not route_bits.pair(source).passes:
        raise CalendarError(
            f"source {source} has P=0; the flit cannot start on its own path (Calendar §2.7.2)"
        )

    hops: list[Hop] = []
    landings: list[Landing] = []
    revisits: list[int] = []
    expanded: set[int] = set()
    order = 0

    stack: list[tuple[int, int | None]] = [(source, None)]
    while stack:
        node, ingress = stack.pop()
        lands = route_bits.pair(node).lands
        repeat = node in expanded
        if lands:
            landings.append(Landing(node=node, ingress=ingress, order=order, duplicate=repeat))
            order += 1
        if repeat:
            # Expanding again would not terminate on a cyclic induced subgraph.
            revisits.append(node)
            hops.append(Hop(node=node, ingress=ingress, egress=(), lands=lands))
            continue
        expanded.add(node)

        egress = tuple(
            peer
            for peer in topology.neighbours(node)
            if route_bits.pair(peer).passes and peer != ingress
        )
        hops.append(Hop(node=node, ingress=ingress, egress=egress, lands=lands))
        for peer in reversed(egress):
            stack.append((peer, node))

    return MulticastTrace(
        source=source,
        route_bits=route_bits,
        hops=tuple(hops),
        landings=tuple(landings),
        unexpanded_revisits=tuple(sorted(set(revisits))),
    )


def delivery_order(
    topology: MeshTopology, route_bits: RouteBits, *, source: int
) -> tuple[int, ...]:
    """Convenience wrapper returning the deduplicated land order."""
    return simulate_multicast(topology, route_bits, source=source).land_nodes


def link_load(trace: MulticastTrace) -> Iterator[tuple[tuple[int, int], int]]:
    """Iterate the trace's per-link flit counts in a deterministic order."""
    load = trace.link_load()
    for key in sorted(load):
        yield key, load[key]
