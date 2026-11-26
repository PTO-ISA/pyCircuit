"""Conservative fixed-Calendar packet timing with explicit provisional inputs.

Source windows are configuration, never calculated from packet size. All groups
within each phase are disjoint and run in parallel; their identical worst-path
budgets permit one group projection. This is not a queue/port contention model.
"""

from collections import Counter
from fractions import Fraction
from math import ceil

from wse_model.analysis.bandwidth import flit_header_cost
from wse_model.analysis.platform import AICORE_CLOCK_HZ, NOC_CLOCK_HZ, NOC_LINK_WIDTH_BYTES
from wse_model.calendar.route_bits import RouteBits
from wse_model.noc.flit import flit_count
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE

ABI_FIELDS = (
    "row_read",
    "row_write",
    "row_flits",
    "column_read",
    "column_write",
    "column_flits",
    "hop",
    "tx",
    "rx",
    "alignment",
    "arm",
    "guard",
    "slot_cycles",
)


def values(profile):
    if profile["schema"] != "wse-allgather-timing/1":
        raise ValueError("unsupported timing profile")
    parameters = profile["parameters"]
    for name, entry in parameters.items():
        if entry["status"] not in ("documented", "pending") or not entry["source"]:
            raise ValueError(f"parameter must retain its status and provenance: {name}")
    result = {key: entry["value"] for key, entry in parameters.items()}
    fixed = {
        "noc_clock_hz": NOC_CLOCK_HZ,
        "core_clock_hz": AICORE_CLOCK_HZ,
        "link_bytes_per_noc_cycle": NOC_LINK_WIDTH_BYTES,
        "storage": "UB",
        "independent_ub_ports": True,
        "packet_boundary": "source",
        "header_policy": "per-flit",
        "endpoint_buffering": "whole-packet",
        "injection_flits_per_noc_cycle": 1,
        "ejection_flits_per_noc_cycle": 1,
        "multicast_branch_issue": "parallel",
        "slots_per_period": 8,
    }
    for key, expected in fixed.items():
        if result[key] != expected:
            raise ValueError(f"unsupported timing policy {key}={result[key]}")
    for key, value in result.items():
        if key not in fixed and (type(value) is not int or not 0 < value <= 1000000):
            raise ValueError(f"timing parameter must be a positive bounded integer: {key}")
    return result


def ub_cycles(nbytes, width, latency_ns, p):
    # First beat COMPLETES at latency; only subsequent beats add service cycles.
    beats = (nbytes + width - 1) // width
    duration = Fraction(latency_ns, 10**9) + Fraction(beats - 1, p["core_clock_hz"])
    return ceil(duration * p["noc_clock_hz"])


def prepare_timing(program, descriptors, profile):
    p = values(profile)
    if program["topology"] != [5, 8] or program["members"] != list(range(32)):
        raise ValueError("timing projection requires the declared 40-node/32-member workload")
    header = flit_header_cost(node_count=40, epoch_tag_bits=p["epoch_tag_bits"])
    phases, abi = [], []
    absolute_start = 0
    for phase_id, phase in enumerate(program["phases"]):
        ranks = phase["ranks"]
        if (phase_id, ranks) not in ((0, 8), (1, 4)):
            raise ValueError("unsupported phase/group symmetry")
        groups = phase["groups"]
        expected_groups = (
            [list(range(row * 8, row * 8 + 8)) for row in range(4)]
            if phase_id == 0
            else [list(range(column, 32, 8)) for column in range(8)]
        )
        if groups != expected_groups:
            raise ValueError("group rank order must match fixed Calendar coordinate ownership")
        amounts = Counter()
        traces = {}
        for d in descriptors:
            if d.phase == phase_id:
                amounts[d.source] += d.nbytes
                if d.source in traces and traces[d.source].route_bits.to_int() != d.route:
                    raise ValueError("source fragments must follow one fixed route")
                traces[d.source] = simulate_multicast(
                    CALENDAR_BASELINE.topology, RouteBits.from_int(d.route, 40), source=d.source
                )
        nbytes = phase["words"] * 4
        if amounts != Counter(dict.fromkeys(range(32), nbytes)):
            raise ValueError("packet projection must preserve all descriptor payload bytes")
        links_by_group = []
        for group in groups:
            links = set()
            for source in group:
                trace = traces[source]
                if not trace.is_clean_tree_walk or set(trace.land_nodes) != set(group) - {source}:
                    raise ValueError("unproved multicast destinations")
                if trace.max_depth > ranks - 1:
                    raise ValueError("route exceeds reserved worst-path budget")
                links.update((hop.node, peer) for hop in trace.hops for peer in hop.egress)
            if any(links & previous for previous in links_by_group):
                raise ValueError("parallel phase groups share directional links")
            links_by_group.append(links)
        read = ub_cycles(nbytes, p["ub_read_bytes_per_core_cycle"], p["ub_read_first_beat_ns"], p)
        write = ub_cycles(
            nbytes, p["ub_write_bytes_per_core_cycle"], p["ub_write_first_beat_ns"], p
        )
        flits = flit_count(nbytes, header)
        abi.extend((read, write, flits))
        stages = {
            "ub_read": read,
            "tx_cdc": p["tx_cdc_noc_cycles"],
            "network": flits - 1 + (ranks - 1) * p["hop_noc_cycles"],
            "rx_cdc": p["rx_cdc_noc_cycles"],
            "ub_write": write,
            "guard": p["guard_noc_cycles"],
        }
        service = sum(stages.values())
        slot = p["slot_noc_cycles"]
        if service > slot:
            raise ValueError(
                f"{phase['name']} packet needs {service} cycles, exceeds fixed Calendar slot {slot}"
            )
        setup = p["alignment_noc_cycles"] + p["arm_noc_cycles"]
        duration = setup + (ranks - 1) * slot + service
        windows = []
        for rank in range(ranks):
            start = absolute_start + setup + rank * slot
            cursor = start
            intervals = {}
            for name, count in stages.items():
                intervals[name] = {"start_cycle": cursor, "end_cycle": cursor + count}
                cursor += count
            windows.append(
                {
                    "source_rank": rank,
                    "sources": [group[rank] for group in groups],
                    "slot_start_cycle": start,
                    "slot_limit_cycle": start + slot,
                    "completion_cycle": cursor,
                    "stages": intervals,
                }
            )
        phases.append(
            {
                "name": phase["name"],
                "ranks": ranks,
                "parallel_groups": len(groups),
                "packet_bytes_per_source": nbytes,
                "logical_traffic": {
                    "source_read_bytes": 32 * nbytes,
                    "destination_write_bytes": 32 * ranks * nbytes,
                    "remote_delivery_bytes": 32 * (ranks - 1) * nbytes,
                    "self_copy_bytes": 32 * nbytes,
                },
                "flits_per_source": flits,
                "stage_durations": stages,
                "source_service_cycles": service,
                "phase_start_cycle": absolute_start,
                "phase_cycles": duration,
                "phase_end_cycle": absolute_start + duration,
                "slot_idle_cycles": (ranks - 1) * (slot - service),
                "window_trace": windows,
                "self_copy_policy": "source UB write overlaps tx/network/remote write; remote worst path dominates",
            }
        )
        absolute_start += duration
    if len(phases) != 2:
        raise ValueError("two phases required")
    abi.extend(
        p[key]
        for key in (
            "hop_noc_cycles",
            "tx_cdc_noc_cycles",
            "rx_cdc_noc_cycles",
            "alignment_noc_cycles",
            "arm_noc_cycles",
            "guard_noc_cycles",
            "slot_noc_cycles",
        )
    )
    return {
        "status": "PROVISIONAL_TIMING_MODEL",
        "hardware_calibrated": False,
        "parameters": profile["parameters"],
        "abi_fields": list(ABI_FIELDS),
        "abi_values": abi,
        "calendar": {
            "opcode": 1,
            "period_noc_cycles": slot * p["slots_per_period"],
            "slot_noc_cycles": slot,
            "slots_per_period": p["slots_per_period"],
            "origin": "after per-phase alignment and arm; same table for ROW/COL",
            "owner": "source column rank for horizontal links, source row rank for vertical links",
        },
        "phases": phases,
        "total_noc_cycles": absolute_start,
        "predicted_seconds": absolute_start / p["noc_clock_hz"],
        "conditions": "whole-packet endpoints; fixed source windows; disjoint groups parallel; phases sequential; NoC-grid service rounding; clock phase abstracted into CDC budgets",
    }
