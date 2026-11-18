"""Independent payload fixtures for the integrated timing executor."""

from dataclasses import replace

from machine_cases import (
    TREE_ROUTE,
    MachineCase,
    _finish,
    _post,
    _setup,
    copy_72_bytes,
    tree_copy_duplicate,
)

from wse_model.programs.abi import Command, Kind, MemoryPatch, MemoryRegion, Program
from wse_model.programs.timing import fixed_calendar, timed_program


def one_hop() -> MachineCase:
    """One packet from node 38 to 39, two flits with the default 52-byte payload."""
    case = copy_72_bytes("timed-one-hop", 256, 1536, trailing_guard=b"\xcc" * 16)
    return replace(case, program=timed_program(case.program), stall_copy_pc=None)


def two_hop_pipeline() -> MachineCase:
    """Send four flits from node 0 through node 1 to node 2."""
    payload = bytes(range(200))
    commands = _setup((0, 2)) + [_post(2, 1536, 200, 200)]
    commands.append(
        Command(
            Kind.COPY,
            opcode=1,
            epoch=1,
            source=0,
            src=256,
            dst=1536,
            nbytes=200,
            route=2 | (2 << 2) | (3 << 4),
        )
    )
    commands += _finish((2,))
    return MachineCase(
        name="timed-two-hop-pipeline",
        program=timed_program(Program(tuple(commands))),
        patches=(MemoryPatch(0, 256, payload),),
        readback=(MemoryRegion(2, 1536, 200),),
        expected_bytes=payload,
        expected_counts={2: 200},
        expected_traffic=(200, 200),
    )


def branching_multicast() -> MachineCase:
    """Node 9 reaches 1 and 10 in one hop, and node 33 in three hops."""
    case = tree_copy_duplicate()
    original = case.program.commands
    broadcast = next(command for command in original if command.kind == Kind.COPY)
    assert broadcast.route == TREE_ROUTE
    commands = tuple(
        command
        for command in original
        if command.kind in (Kind.INSTALL, Kind.NEXT_EPOCH, Kind.POST)
    ) + (broadcast, *_finish((1, 10, 33)))
    # Keep the source's original data, since this fixture sends only the broadcast.
    return replace(
        case,
        name="timed-branching-multicast",
        program=timed_program(Program(commands)),
        expected_bytes=case.expected_bytes[:-24] + case.patches[0].data,
        stall_copy_pc=None,
    )


def disjoint_links() -> MachineCase:
    """Two 72-byte packets take equally long, link-disjoint one-hop paths."""
    sources = (0, 8)
    targets = (1, 9)
    payloads = (bytes(range(72)), bytes(range(128, 200)))
    commands = _setup((0, 1, 8, 9)) + [_post(node, 1536, 72, 72) for node in targets]
    for source, target in zip(sources, targets, strict=True):
        commands.append(
            Command(
                Kind.COPY,
                opcode=1,
                epoch=1,
                source=source,
                src=256,
                dst=1536,
                nbytes=72,
                route=(2 << (2 * source)) | (3 << (2 * target)),
            )
        )
    commands += _finish(targets)
    return MachineCase(
        name="timed-disjoint-links",
        program=timed_program(Program(tuple(commands))),
        patches=tuple(
            MemoryPatch(node, 256, data) for node, data in zip(sources, payloads, strict=True)
        ),
        readback=tuple(MemoryRegion(node, 1536, 72) for node in targets),
        expected_bytes=b"".join(payloads),
        expected_counts=dict.fromkeys(targets, 72),
        expected_traffic=(144, 144),
    )


def shared_link() -> MachineCase:
    """Two 72-byte packets reach node 1 through the same directed link 0 → 1."""
    sources = (0, 8)
    payloads = (bytes(range(72)), bytes(range(128, 200)))
    commands = _setup((0, 1, 8)) + [_post(1, 1536, 144, 144)]
    # 0 → 1 and 8 → 0 → 1. Node 0 only forwards the packet from node 8.
    direct_route = 2 | (3 << 2)
    for source, dst, route in (
        (0, 1536, direct_route),
        (8, 1608, direct_route | (2 << 16)),
    ):
        commands.append(
            Command(
                Kind.COPY,
                opcode=1,
                epoch=1,
                source=source,
                src=256,
                dst=dst,
                nbytes=72,
                route=route,
            )
        )
    commands += _finish((1,))
    return MachineCase(
        name="timed-shared-link",
        program=timed_program(Program(tuple(commands))),
        patches=tuple(
            MemoryPatch(node, 256, data) for node, data in zip(sources, payloads, strict=True)
        ),
        readback=(MemoryRegion(1, 1536, 144),),
        expected_bytes=b"".join(payloads),
        expected_counts={1: 144},
        expected_traffic=(144, 144),
    )


def open_calendar() -> dict[int, int]:
    """Allow any source in every slot of each valid link."""
    masks = fixed_calendar()
    # Vertical slots 5 through 7 are unused in the default config, but they are
    # valid on every link with slot 0 enabled.
    return {address: ((1 << 40) - 1 if masks[address & ~7] else 0) for address in masks}


def check_default_open_events(case: MachineCase, events: list[dict]) -> None:
    """Compare selected model events against hand-derived timing constraints.

    Use the unmodified provisional parameters and an open Calendar. The first
    phase releases after 16 alignment + 8 arm ticks. Endpoints take 200 ticks
    for the first 128-byte beat. Injection starts two ticks after source read,
    and each link adds one tick. At the destination, the last flit is followed
    by a two-tick crossing and the corresponding endpoint write duration.
    """
    if case.name == "timed-shared-link":
        reads = sorted(
            (event["source"], event["noc_cycle"]) for event in events if event["kind"] == 1
        )
        assert reads == [(0, 224), (8, 224)], "both source reads finish at the same time"
        arrivals = [event for event in events if event["kind"] == 4 and event["node"] == 1]
        assert sorted((event["source"], event["flit"]) for event in arrivals) == [
            (0, 0),
            (0, 1),
            (8, 0),
            (8, 1),
        ], "both two-flit packets traverse the shared link"
        # Earliest possible arrival is 24 + 200 + 2 + 1. Four flits on one
        # link need four distinct ticks, independent of arbitration priority.
        cycles = {event["noc_cycle"] for event in arrivals}
        assert (
            len(cycles) == 4 and min(cycles) >= 227 and max(cycles) >= 230
        ), "the shared link carries at most one flit per cycle"
        return

    endpoint_ticks = 200
    if case.name == "timed-one-hop":
        paths = ((38, 39, 1),)
        flits = 2
    elif case.name == "timed-two-hop-pipeline":
        paths = ((0, 2, 2),)
        flits = 4  # ceil(200 B / 52 B per flit)
        endpoint_ticks = 202  # 200 ticks + ceil(10/7 ticks) for the second beat
        # At t=230 the first flit reaches node 2 while the second reaches node 1.
        assert sorted(
            (event["node"], event["flit"], event["noc_cycle"])
            for event in events
            if event["kind"] == 4 and event["source"] == 0
        ) == [
            (node, flit, first_arrival + flit)
            for node, first_arrival in ((1, 229), (2, 230))
            for flit in range(flits)
        ], "the two hops overlap across successive flits"
    elif case.name == "timed-disjoint-links":
        paths = ((0, 1, 1), (8, 9, 1))
        flits = 2
    elif case.name == "timed-branching-multicast":
        paths = ((9, 1, 1), (9, 10, 1), (9, 33, 3))
        flits = 1
    else:
        raise ValueError(f"no independent timing fixture for {case.name}")

    def points(kind: int) -> list[tuple[int, int, int]]:
        return sorted(
            (event["source"], event["node"], event["noc_cycle"])
            for event in events
            if event["kind"] == kind
        )

    sources = sorted({source for source, _, _ in paths})
    read_complete = 24 + endpoint_ticks
    first_injection = read_complete + 2
    assert points(1) == [(source, source, read_complete) for source in sources], "source read"
    assert points(2) == [
        (source, source, first_injection + flit) for source in sources for flit in range(flits)
    ], "flit injection"

    target_pairs = {(source, target) for source, target, _ in paths}
    target_arrivals = [point for point in points(4) if (point[0], point[1]) in target_pairs]
    assert target_arrivals == [
        (source, target, first_injection + hops + flit)
        for source, target, hops in paths
        for flit in range(flits)
    ], "destination flit arrivals"

    last_injection = first_injection + flits - 1
    assert points(5) == [
        (source, target, last_injection + hops + 2 + endpoint_ticks)
        for source, target, hops in paths
    ], "destination commit after final flit"
    assert points(6) == [
        (
            source,
            source,
            last_injection + max(h for s, _, h in paths if s == source) + 2 + endpoint_ticks + 1,
        )
        for source in sources
    ], "guard after last destination commit"
