"""Calendar lowering for the bounded rank-major PTO AllGather profile.

The PTO frontend supplies verified transfers. This module owns WSE layout,
routing and fragmentation, not the semantics of arbitrary PTO instructions.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

from wse_model.calendar.geometry import PayloadGeometry, SymmetricArena
from wse_model.fixtures import group_allgather_bitmap
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE


@dataclass(frozen=True)
class Descriptor:
    phase: int
    source: int
    fragment: int
    nbytes: int
    source_address: int
    destination_address: int
    route: int
    expected: int
    arena_base: int
    arena_capacity: int

    def pack(self) -> bytes:
        return struct.pack(
            "<12I",
            self.phase,
            self.source,
            self.fragment,
            self.nbytes,
            self.source_address,
            self.destination_address,
            self.route & 0xFFFFFFFF,
            (self.route >> 32) & 0xFFFFFFFF,
            self.route >> 64,
            self.expected,
            self.arena_base,
            self.arena_capacity,
        )


def lower_calendar(program: dict, transfers: list[list[dict]]) -> list[Descriptor]:
    """Lower proved all-to-all copies into one multicast per source fragment.

    Each transfer set was extracted from the post-PTOAS SSA for every local
    rank. Exact source/destination coverage is checked before multicast fusion.
    """
    if program["schema"] != "wse-pypto-allgather/1":
        raise ValueError("unsupported program schema")
    if program["topology"] != [5, 8]:
        raise ValueError("this route encoding uses the 40-node carrier")
    members = program["members"]
    if (
        not members
        or members != sorted(set(members))
        or any(not 0 <= node < 40 for node in members)
    ):
        raise ValueError("members must be distinct nodes in the 40-node route space")
    if not 0 < program["bank_bytes"] <= 65536 or len(program["phases"]) != 2:
        raise ValueError("unsupported memory or phase profile")
    phases = program["phases"]
    if phases[1]["source_base"] != phases[0]["target_base"]:
        raise ValueError("column must read the actual row output")
    if phases[1]["words"] != phases[0]["ranks"] * phases[0]["words"]:
        raise ValueError("column flatten view has the wrong size")
    expected_dependency = {
        "producer": "row.target",
        "consumer": "column.source",
        "view": "contiguous-flatten",
        "bytes": phases[0]["ranks"] * phases[0]["words"] * 4,
    }
    if program["dependency"] != expected_dependency:
        raise ValueError("unsupported phase dependency")
    topology = CALENDAR_BASELINE.topology
    descriptors = []
    for phase_id, (phase, copies) in enumerate(zip(phases, transfers, strict=True)):
        ranks, words = phase["ranks"], phase["words"]
        fragments = phase["fragments"]
        if not 1 <= ranks <= 40 or words <= 0 or fragments <= 0:
            raise ValueError("unsupported executable geometry")
        if words * 4 % fragments or (words * 4 // fragments) % 8:
            raise ValueError("fragments must partition the source into aligned 8B beats")
        if phase["dtype"] != "f32" or phase["layout"] != "rank-major-contiguous":
            raise ValueError("expected contiguous FP32 rank-major storage")
        if phase["input_shape"] != [1, words] or phase["output_shape"] != [ranks, words]:
            raise ValueError("shape metadata disagrees with the contiguous kernel ABI")
        expected_copies = [
            {
                "source_rank": source,
                "target_rank": target,
                "source_offset": 0,
                "target_offset": source * words * 4,
                "nbytes": words * 4,
            }
            for source in range(ranks)
            for target in range(ranks)
        ]
        if copies != expected_copies:
            raise ValueError("PTOAS transfers do not implement the declared rank-ordered AllGather")
        groups = phase["groups"]
        if any(len(group) != ranks for group in groups):
            raise ValueError("group size disagrees with the PTO rank count")
        if sorted(node for group in groups for node in group) != members:
            raise ValueError("each phase must partition the declared participants")
        logical_bytes = words * 4
        geometry = PayloadGeometry(1, logical_bytes, logical_bytes, 1, ranks * logical_bytes)
        geometry.check_source_alignment()
        arena = SymmetricArena.packed(
            recv_sym_base=phase["target_base"],
            row_count=1,
            row_bytes=logical_bytes,
            member_ranks=tuple(range(ranks)),
        )
        arena.validate_fill()
        src_start, dst_start = phase["source_base"], phase["target_base"]
        if src_start % 8 or dst_start % 8:
            raise ValueError("source and target addresses must be 8B aligned")
        if not (0 <= src_start < src_start + logical_bytes <= program["bank_bytes"]):
            raise ValueError("source range exceeds the modeled bank")
        if not (0 <= dst_start < dst_start + geometry.capacity <= program["bank_bytes"]):
            raise ValueError("target range exceeds the modeled bank")
        if max(src_start, dst_start) < min(
            src_start + logical_bytes, dst_start + geometry.capacity
        ):
            raise ValueError("source and target buffers overlap")
        for source in members:
            group = next(tuple(group) for group in groups if source in group)
            rank = group.index(source)
            route = group_allgather_bitmap(topology, group, source)
            trace = simulate_multicast(topology, route, source=source)
            if not trace.is_clean_tree_walk or set(trace.land_nodes) != set(group) - {source}:
                raise ValueError("Calendar route does not reach the transfer destinations")
            fragment_bytes = logical_bytes // fragments
            for fragment in range(fragments):
                descriptors.append(
                    Descriptor(
                        phase_id,
                        source,
                        fragment,
                        fragment_bytes,
                        src_start + fragment * fragment_bytes,
                        arena.dst_address(rank, 0) + fragment * fragment_bytes,
                        route.to_int(),
                        geometry.exp_val(ranks - 1),
                        dst_start,
                        geometry.capacity,
                    )
                )
    return descriptors
