"""Directional link occupancy bounds using the existing WSE wire model.

These bounds omit scheduling and pipeline delays. Packet boundaries and epoch
width are explicit inputs, because neither is inferred from functional cycles.
"""

from collections import Counter, defaultdict

from wse_model.analysis.bandwidth import flit_header_cost, link_time_seconds
from wse_model.analysis.platform import NOC_CLOCK_HZ, NOC_LINK_WIDTH_BYTES
from wse_model.calendar.route_bits import RouteBits
from wse_model.noc.flit import flit_count
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE


def link_occupancy_bounds(descriptors, *, epoch_tag_bits, packet_boundary):
    """Sum phase bottlenecks; opposite link directions have separate capacity."""
    if packet_boundary not in ("source", "fragment"):
        raise ValueError("packet_boundary must be source or fragment")
    header = flit_header_cost(node_count=40, epoch_tag_bits=epoch_tag_bits)
    runs = defaultdict(int)
    for d in descriptors:
        fragment = d.fragment if packet_boundary == "fragment" else None
        runs[d.phase, d.source, d.route, fragment] += d.nbytes
    phases = {}
    for (phase, source, route, _fragment), nbytes in runs.items():
        stats = phases.setdefault(phase, {"links": Counter(), "injected_flits": 0})
        count = flit_count(nbytes, header)
        stats["injected_flits"] += count
        trace = simulate_multicast(
            CALENDAR_BASELINE.topology, RouteBits.from_int(route, node_count=40), source=source
        )
        if not trace.is_clean_tree_walk:
            raise ValueError("timing input must be a clean multicast tree")
        for hop in trace.hops:
            for peer in hop.egress:
                stats["links"][hop.node, peer] += count
    result = []
    for phase, stats in sorted(phases.items()):
        links = stats["links"]
        bottleneck = max(links.values(), default=0)
        result.append(
            {
                "phase": phase,
                "injected_flits": stats["injected_flits"],
                "wire_byte_hops": sum(links.values()) * NOC_LINK_WIDTH_BYTES,
                "bottleneck_noc_cycles": bottleneck,
                "link_occupancy_lower_bound_seconds": link_time_seconds(
                    bottleneck * NOC_LINK_WIDTH_BYTES
                ),
                "directional_links": [
                    {
                        "source": a,
                        "destination": b,
                        "flits": count,
                        "wire_bytes": count * NOC_LINK_WIDTH_BYTES,
                    }
                    for (a, b), count in sorted(links.items())
                ],
            }
        )
    return {
        "packet_boundary": packet_boundary,
        "header": header.describe(),
        "noc_clock_hz": NOC_CLOCK_HZ,
        "phases": result,
        "sequential_phases_link_lower_bound_seconds": sum(
            p["link_occupancy_lower_bound_seconds"] for p in result
        ),
    }
