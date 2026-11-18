"""AllGather programs; group geometry belongs here, never in machine state."""

from __future__ import annotations

from collections.abc import Mapping, Sequence

from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.validate import validate_route_bits
from wse_model.fixtures import group_allgather_bitmap
from wse_model.programs.abi import (
    DEFAULT_PROFILE,
    Command,
    DeviceProfile,
    Kind,
    MemoryRegion,
    Program,
)
from wse_model.topology import CALENDAR_BASELINE


def _groups(groups: Sequence[Sequence[int]], profile: DeviceProfile) -> tuple[tuple[int, ...], ...]:
    result = tuple(tuple(group) for group in groups)
    members = [node for group in result for node in group]
    if not result or any(not group for group in result):
        raise ValueError("AllGather groups must be nonempty")
    if len(members) != len(set(members)):
        raise ValueError("a participant can have only one group per phase")
    if any(not isinstance(node, int) or not 0 <= node < profile.nodes for node in members):
        raise ValueError("participant exceeds the declared node-bank profile")
    return result


def _route(group: tuple[int, ...], source: int, routes: Mapping[int, int] | None) -> int:
    topology = CALENDAR_BASELINE.topology
    route = (
        RouteBits.from_int(routes[source], topology.node_count)
        if routes is not None
        else group_allgather_bitmap(topology, group, source)
    )
    validation = validate_route_bits(
        topology,
        route,
        source=source,
        expected_land_nodes=frozenset(group) - {source},
        self_delivery=False,
    )
    if validation.errors:
        raise ValueError(
            f"invalid supplied AllGather route for source {source}: {validation.errors}"
        )
    return route.to_int()


def allgather_program(
    groups: Sequence[Sequence[int]],
    *,
    source_address: int,
    destination_address: int,
    bytes_per_source: int,
    fragment_bytes: int,
    opcode: int = 1,
    epoch: int = 1,
    profile: DeviceProfile = DEFAULT_PROFILE,
    routes: Mapping[int, int] | None = None,
) -> Program:
    """Build a complete one-phase program in the caller's group/rank order.

    The default bitmap builder is the existing documented group fixture, not a
    production routing algorithm. Groups requiring transit nodes supply their
    route images explicitly. Every image is checked against the Python contract.
    """
    profile.validate()
    groups = _groups(groups, profile)
    if not 1 <= opcode <= 6 or not 1 <= epoch <= 255:
        raise ValueError("invalid opcode or epoch")
    if (
        bytes_per_source <= 0
        or fragment_bytes <= 0
        or fragment_bytes % profile.beat_bytes
        or bytes_per_source % fragment_bytes
    ):
        raise ValueError("fragments must partition the source into whole aligned beats")
    fragments = bytes_per_source // fragment_bytes
    if fragments > 65536:
        raise ValueError("fragment identities exceed the sequence carrier")
    members = sorted(node for group in groups for node in group)
    commands = []
    for node in members:
        commands.append(Command(Kind.INSTALL, node=node, opcode=opcode))
        for _ in range(epoch):
            commands.append(Command(Kind.NEXT_EPOCH, node=node, opcode=opcode))
    for group in groups:
        capacity = len(group) * bytes_per_source
        expected = (len(group) - 1) * bytes_per_source
        for node in group:
            MemoryRegion(node, source_address, bytes_per_source).validate(profile)
            MemoryRegion(node, destination_address, capacity).validate(profile)
            if max(source_address, destination_address) < min(
                source_address + bytes_per_source, destination_address + capacity
            ):
                raise ValueError("AllGather source and destination storage must not overlap")
            commands.append(
                Command(
                    Kind.POST,
                    node=node,
                    opcode=opcode,
                    epoch=epoch,
                    dst=destination_address,
                    capacity=capacity,
                    expected=expected,
                    compiled_expected=expected,
                )
            )
    for source in members:
        group = next(group for group in groups if source in group)
        rank = group.index(source)
        route = _route(group, source, routes)
        for fragment in range(fragments):
            offset = fragment * fragment_bytes
            commands.append(
                Command(
                    Kind.COPY,
                    opcode=opcode,
                    epoch=epoch,
                    seq=fragment,
                    source=source,
                    src=source_address + offset,
                    dst=destination_address + rank * bytes_per_source + offset,
                    nbytes=fragment_bytes,
                    route=route,
                    self_copy=True,
                )
            )
    for node in members:
        commands.append(Command(Kind.WAIT, node=node, opcode=opcode, epoch=epoch))
    for node in members:
        commands.append(Command(Kind.RETIRE, node=node))
    commands.append(Command(Kind.END))
    program = Program(tuple(commands), profile)
    program.pack()
    return program


def compile_allgather(
    manifest: dict, descriptors: Sequence, profile: DeviceProfile | None = None
) -> Program:
    """Turn proved PTO transfers into a runtime program, retaining phase barriers.

    Descriptors are algorithm-side transport fragments, not instructions accepted
    directly by hardware. The PTO adapter has already checked the original IR.
    """
    profile = profile or DeviceProfile(bank_bytes=manifest["bank_bytes"])
    profile.validate()
    commands = []
    installed: set[tuple[int, int]] = set()
    epochs: dict[tuple[int, int], int] = {}
    for phase_id, phase in enumerate(manifest["phases"]):
        groups = _groups(phase["groups"], profile)
        members = sorted(node for group in groups for node in group)
        opcode = phase.get("opcode", 1)
        epoch = phase_id + 1
        if opcode not in range(1, 7) or epoch > 255:
            raise ValueError("program exceeds the opcode/epoch profile")
        for node in members:
            key = node, opcode
            if key not in installed:
                commands.append(Command(Kind.INSTALL, node=node, opcode=opcode))
                installed.add(key)
            current = epochs.get(key, 0)
            while current < epoch:
                commands.append(Command(Kind.NEXT_EPOCH, node=node, opcode=opcode))
                current += 1
            epochs[key] = current
        logical_bytes = phase["words"] * 4
        for group in groups:
            capacity = len(group) * logical_bytes
            expected = (len(group) - 1) * logical_bytes
            for node in group:
                MemoryRegion(node, phase["source_base"], logical_bytes).validate(profile)
                MemoryRegion(node, phase["target_base"], capacity).validate(profile)
                commands.append(
                    Command(
                        Kind.POST,
                        node=node,
                        opcode=opcode,
                        epoch=epoch,
                        dst=phase["target_base"],
                        capacity=capacity,
                        expected=expected,
                        compiled_expected=expected,
                    )
                )
        fragments = [descriptor for descriptor in descriptors if descriptor.phase == phase_id]
        if not fragments:
            raise ValueError("an AllGather phase has no proved transfers")
        for descriptor in fragments:
            commands.append(
                Command(
                    Kind.COPY,
                    opcode=opcode,
                    epoch=epoch,
                    seq=descriptor.fragment,
                    source=descriptor.source,
                    src=descriptor.source_address,
                    dst=descriptor.destination_address,
                    nbytes=descriptor.nbytes,
                    route=descriptor.route,
                    self_copy=True,
                )
            )
        for node in members:
            commands.append(Command(Kind.WAIT, node=node, opcode=opcode, epoch=epoch))
        for node in members:
            commands.append(Command(Kind.RETIRE, node=node))
    commands.append(Command(Kind.END))
    result = Program(tuple(commands), profile)
    result.pack()
    return result
